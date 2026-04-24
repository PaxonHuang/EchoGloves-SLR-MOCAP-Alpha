# -*- coding: utf-8 -*-
"""
glove_relay.src.models.stgcn_model — Spatial-Temporal Graph ConvNet (L2).

Architecture (≈ 280 K parameters)
==================================
1. **Pseudo-skeleton projection**: Linear(21 → 42) maps 21 sensor features
   to 21 pseudo 2-D keypoint coordinates (x, y).
2. **Spatial graph convolution**: Operates on a hand-skeleton graph with
   21 nodes following MediaPipe / OpenPose hand topology.
3. **Temporal convolution (TCN)**: 1-D conv along the time axis with
   residual connections.
4. **ST-Conv Block**: Spatial → BN → ReLU → Temporal → BN → ReLU
   with a skip/residual connection.
5. **Attention pooling**: Learnable channel-attention for classification.

Input shape: ``(B, T, 21)`` where T = 30 (sliding window).
Output shape: ``(B, 46)`` logits.
"""

from __future__ import annotations

from typing import Any, List, Optional

import torch
import torch.nn as nn
import torch.nn.functional as F

from src.models.base_model import BaseModel
from src.utils.logger import get_logger

logger = get_logger(__name__)


# ---------------------------------------------------------------------------
# Hand skeleton adjacency (21 nodes)
# ---------------------------------------------------------------------------
# MediaPipe hand landmark topology:
#   0(wrist) → 1-4(thumb), 5-8(index), 9-12(middle), 13-16(ring), 17-20(pinky)
_ADJACENCY: List[tuple[int, int]] = [
    (0, 1), (1, 2), (2, 3), (3, 4),           # thumb
    (0, 5), (5, 6), (6, 7), (7, 8),           # index
    (0, 9), (9, 10), (10, 11), (11, 12),      # middle
    (0, 13), (13, 14), (14, 15), (15, 16),    # ring
    (0, 17), (17, 18), (18, 19), (19, 20),    # pinky
    (5, 9), (9, 13), (13, 17),                 # palm cross-links
]

NUM_NODES = 21


def _build_adj_matrix(num_nodes: int = NUM_NODES) -> torch.Tensor:
    """Build a normalised adjacency matrix ``(N, N)`` with self-loops."""
    adj = torch.zeros(num_nodes, num_nodes)
    for u, v in _ADJACENCY:
        adj[u, v] = 1.0
        adj[v, u] = 1.0
    # Self-loops
    adj += torch.eye(num_nodes)
    # Normalise: D^{-1/2} A D^{-1/2}
    deg = adj.sum(dim=-1, keepdim=True).clamp(min=1)
    deg_inv_sqrt = torch.pow(deg, -0.5)
    adj_norm = deg_inv_sqrt * adj * deg_inv_sqrt.T
    return adj_norm


# ---------------------------------------------------------------------------
# Graph Convolution layer
# ---------------------------------------------------------------------------
class GraphConv(nn.Module):
    """
    Single spatial graph convolution layer.

    Parameters
    ----------
    in_channels : int
        Input feature dim per node.
    out_channels : int
        Output feature dim per node.
    num_nodes : int
        Number of graph nodes (default 21).
    """

    def __init__(self, in_channels: int, out_channels: int, num_nodes: int = NUM_NODES) -> None:
        super().__init__()
        self.num_nodes = num_nodes
        # Learnable adjacency bias
        self.A = nn.Parameter(_build_adj_matrix(num_nodes))
        self.weight = nn.Parameter(torch.randn(in_channels, out_channels) * 0.02)
        self.bias = nn.Parameter(torch.zeros(out_channels))

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Parameters
        ----------
        x : torch.Tensor
            ``(B, N, C_in)``

        Returns
        -------
        torch.Tensor
            ``(B, N, C_out)``
        """
        # A: (N, N), x: (B, N, C_in)
        # support = A @ x: (B, N, C_in)
        support = torch.einsum("nm,bmc->bnc", self.A, x)
        out = support @ self.weight + self.bias  # (B, N, C_out)
        return out


# ---------------------------------------------------------------------------
# Temporal Convolution layer (TCN-style)
# ---------------------------------------------------------------------------
class TemporalConv(nn.Module):
    """
    1-D temporal convolution along the time axis.

    Parameters
    ----------
    in_channels : int
        Feature dim per node per timestep.
    out_channels : int
        Output feature dim.
    kernel_size : int
        Temporal kernel size (default 3).
    stride : int
        Temporal stride (default 1).
    dilation : int
        Temporal dilation (default 1).
    """

    def __init__(
        self,
        in_channels: int,
        out_channels: int,
        kernel_size: int = 3,
        stride: int = 1,
        dilation: int = 1,
    ) -> None:
        super().__init__()
        padding = (kernel_size - 1) * dilation // 2
        self.conv = nn.Conv1d(
            in_channels,
            out_channels,
            kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
        )
        self.bn = nn.BatchNorm1d(out_channels)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Parameters
        ----------
        x : torch.Tensor
            ``(B, C, T)``

        Returns
        -------
        torch.Tensor
            ``(B, C_out, T)``
        """
        return F.relu(self.bn(self.conv(x)))


# ---------------------------------------------------------------------------
# ST-Conv Block (Spatial → Temporal with residual)
# ---------------------------------------------------------------------------
class STConvBlock(nn.Module):
    """
    Spatial-Temporal convolution block with residual connection.

    Parameters
    ----------
    in_channels : int
    out_channels : int
    num_nodes : int
    temporal_kernel : int
    temporal_dilation : int
    """

    def __init__(
        self,
        in_channels: int,
        out_channels: int,
        num_nodes: int = NUM_NODES,
        temporal_kernel: int = 3,
        temporal_dilation: int = 1,
    ) -> None:
        super().__init__()

        self.graph_conv = GraphConv(in_channels, out_channels, num_nodes)
        self.bn_s = nn.BatchNorm1d(out_channels)

        self.temp_conv = TemporalConv(out_channels, out_channels, temporal_kernel, dilation=temporal_dilation)

        # Residual projection
        self.residual: Optional[nn.Module] = None
        if in_channels != out_channels:
            self.residual = nn.Conv1d(in_channels, out_channels, kernel_size=1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Parameters
        ----------
        x : torch.Tensor
            ``(B, T, N, C_in)``

        Returns
        -------
        torch.Tensor
            ``(B, T, N, C_out)``
        """
        B, T, N, C = x.shape
        identity = x

        # --- Spatial: operate per-timestep ---
        # Reshape to (B*T, N, C) for graph conv
        x_spatial = x.reshape(B * T, N, C)
        x_spatial = self.graph_conv(x_spatial)  # (B*T, N, C_out)
        C_out = x_spatial.shape[-1]
        x_spatial = F.relu(self.bn_s(x_spatial.permute(0, 2, 1)).permute(0, 2, 1))
        x_spatial = x_spatial.reshape(B, T, N, C_out)

        # --- Temporal: reshape to (B*N, C_out, T) ---
        x_temporal = x_spatial.permute(0, 2, 3, 1).reshape(B * N, C_out, T)
        x_temporal = self.temp_conv(x_temporal)  # (B*N, C_out, T)
        out = x_temporal.reshape(B, N, C_out, T).permute(0, 3, 1, 2)  # (B, T, N, C_out)

        # --- Residual ---
        if self.residual is not None:
            identity = identity.permute(0, 3, 2, 1).reshape(B * N, C, T)
            identity = self.residual(identity)
            identity = identity.reshape(B, N, C_out, T).permute(0, 3, 1, 2)

        return F.relu(out + identity)


# ---------------------------------------------------------------------------
# Attention Pooling
# ---------------------------------------------------------------------------
class AttentionPooling(nn.Module):
    """
    Channel-attention pooling: ``(B, T, N, C) → (B, C)``.
    """

    def __init__(self, in_channels: int) -> None:
        super().__init__()
        self.att = nn.Sequential(
            nn.Linear(in_channels, in_channels // 4),
            nn.Tanh(),
            nn.Linear(in_channels // 4, 1),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Parameters
        ----------
        x : torch.Tensor
            ``(B, T, N, C)``

        Returns
        -------
        torch.Tensor
            ``(B, C)``
        """
        B, T, N, C = x.shape
        x_flat = x.reshape(B * T * N, C)
        weights = self.att(x_flat).reshape(B, T, N)  # (B, T, N)
        weights = torch.softmax(weights.reshape(B, -1), dim=-1).reshape(B, T, N, 1)
        pooled = (x * weights).sum(dim=(1, 2))  # (B, C)
        return pooled


# ---------------------------------------------------------------------------
# Main ST-GCN model
# ---------------------------------------------------------------------------
class STGCNModel(BaseModel, nn.Module):
    """
    Spatial-Temporal Graph Convolutional Network for gesture recognition.

    Uses a pseudo-skeleton projection to convert 21 sensor features into
    21 × 2 pseudo-keypoint coordinates, then applies ST-Conv blocks on the
    hand skeleton graph.

    Parameters
    ----------
    input_dim : int
        Number of sensor features per frame (default 21).
    hidden_dim : int
        Hidden feature dimension (default 64).
    num_classes : int
        Number of gesture classes (default 46).
    num_frames : int
        Temporal window size (default 30).
    num_nodes : int
        Number of graph nodes (default 21).
    """

    def __init__(
        self,
        input_dim: int = 21,
        hidden_dim: int = 64,
        num_classes: int = 46,
        num_frames: int = 30,
        num_nodes: int = NUM_NODES,
        **_kwargs: Any,
    ) -> None:
        super().__init__()

        self.input_dim = input_dim
        self.hidden_dim = hidden_dim
        self.num_classes = num_classes
        self.num_frames = num_frames
        self.num_nodes = num_nodes

        # --- Pseudo-skeleton projection ---
        # Map 21 sensor features → 21 nodes × 2 coords (x, y) = 42
        self.skeleton_proj = nn.Linear(input_dim, num_nodes * 2)
        self.coord_norm = nn.LayerNorm(num_nodes * 2)

        # --- ST-Conv blocks ---
        self.st_blocks = nn.ModuleList([
            STConvBlock(2, hidden_dim, num_nodes, temporal_dilation=1),
            STConvBlock(hidden_dim, hidden_dim, num_nodes, temporal_dilation=2),
            STConvBlock(hidden_dim, hidden_dim * 2, num_nodes, temporal_dilation=1),
        ])

        # --- Attention pooling ---
        self.attn_pool = AttentionPooling(hidden_dim * 2)

        # --- Classification head ---
        self.dropout = nn.Dropout(0.4)
        self.fc = nn.Linear(hidden_dim * 2, num_classes)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Forward pass.

        Parameters
        ----------
        x : torch.Tensor
            ``(B, T, 21)`` — temporal window of sensor data.

        Returns
        -------
        torch.Tensor
            ``(B, num_classes)`` — raw logits.
        """
        B, T, _ = x.shape

        # --- Pseudo-skeleton ---
        x = self.skeleton_proj(x)                # (B, T, 42)
        x = self.coord_norm(x)
        x = x.reshape(B, T, self.num_nodes, 2)  # (B, T, N, 2)

        # --- ST-Conv blocks ---
        for block in self.st_blocks:
            x = block(x)  # (B, T, N, C)

        # --- Attention pooling ---
        x = self.attn_pool(x)  # (B, C)

        # --- Classifier ---
        x = self.dropout(x)
        return self.fc(x)

    @torch.no_grad()
    def predict(self, x: torch.Tensor) -> tuple[int, float]:
        """
        Inference helper.

        Returns
        -------
        tuple[int, float]
            ``(gesture_id, confidence)`` for a single sample.
        """
        self.eval()
        logits = self.forward(x)
        if logits.dim() > 1 and logits.size(0) == 1:
            logits = logits.squeeze(0)
        probs = F.softmax(logits, dim=-1)
        confidence, idx = probs.max(dim=-1)
        return int(idx.item()), float(confidence.item())

    def get_config(self) -> dict[str, Any]:
        return {
            "input_dim": self.input_dim,
            "hidden_dim": self.hidden_dim,
            "num_classes": self.num_classes,
            "num_frames": self.num_frames,
            "num_nodes": self.num_nodes,
            "architecture": "stgcn_v1",
        }

    def get_model_info(self) -> dict[str, Any]:
        return {
            "name": "stgcn_v1",
            "params": sum(p.numel() for p in self.parameters()),
            "trainable_params": sum(p.numel() for p in self.parameters() if p.requires_grad),
        }
