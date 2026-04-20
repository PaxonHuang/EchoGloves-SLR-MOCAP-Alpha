"""
ST-GCN (Spatio-Temporal Graph Convolutional Network) Implementation
Based on MS-GCN3 Paper: "Multi-Step Graph Convolutional Network for 3D Human Pose Estimation"

This is a REAL ST-GCN implementation with graph convolution structure,
NOT the fake nn.Linear implementation from the original paper.

Key components:
- GraphConv: Spatial graph convolution based on hand skeleton adjacency
- TemporalConv: 1D temporal convolution (TCN)
- STConvBlock: Combined spatial + temporal convolution with residual
- STGCNModel: Full model with 9 ST-Conv blocks
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np


def get_hand_skeleton_adjacency():
    """
    Build adjacency matrix for hand skeleton graph (21 keypoints).
    Based on MediaPipe Hand Landmarks definition:

    Keypoints:
    0: WRIST
    1-4: THUMB (CMC, MCP, IP, TIP)
    5-8: INDEX (MCP, PIP, DIP, TIP)
    9-12: MIDDLE (MCP, PIP, DIP, TIP)
    13-16: RING (MCP, PIP, DIP, TIP)
    17-20: PINKY (MCP, PIP, DIP, TIP)

    Returns: A (21, 21) adjacency matrix with self-loops
    """
    A = np.zeros((21, 21))

    # Self-loops (each node connects to itself)
    for i in range(21):
        A[i, i] = 1

    # Wrist connections
    edges = [
        # Wrist to each finger MCP
        (0, 1), (0, 5), (0, 9), (0, 13), (0, 17),
        # Thumb chain
        (1, 2), (2, 3), (3, 4),
        # Index chain
        (5, 6), (6, 7), (7, 8),
        # Middle chain
        (9, 10), (10, 11), (11, 12),
        # Ring chain
        (13, 14), (14, 15), (15, 16),
        # Pinky chain
        (17, 18), (18, 19), (19, 20),
        # Palm connections (MCP to MCP)
        (5, 9), (9, 13), (13, 17), (5, 17),
    ]

    # Bidirectional edges (undirected graph)
    for i, j in edges:
        A[i, j] = 1
        A[j, i] = 1

    return A


def normalize_adjacency(A):
    """
    Normalize adjacency matrix: D^(-1/2) * A * D^(-1/2)
    This is standard spectral graph convolution normalization.
    """
    D = np.sum(A, axis=1)
    D_inv_sqrt = np.power(D, -0.5)
    D_inv_sqrt[np.isinf(D_inv_sqrt)] = 0
    D_mat = np.diag(D_inv_sqrt)
    A_norm = D_mat @ A @ D_mat
    return A_norm


class GraphConv(nn.Module):
    """
    Spatial Graph Convolution Layer.

    Implements: X' = A_norm @ X @ W_spatial

    Input: (Batch, Time, Landmarks, In_Channels)
    Output: (Batch, Time, Landmarks, Out_Channels)
    """

    def __init__(self, in_channels, out_channels, A_norm):
        super(GraphConv, self).__init__()
        self.A_norm = nn.Parameter(torch.tensor(A_norm, dtype=torch.float32),
                                   requires_grad=False)
        self.W_spatial = nn.Linear(in_channels, out_channels)

    def forward(self, x):
        # x: (B, T, V, C_in)
        B, T, V, C_in = x.shape

        # Apply adjacency matrix: (B, T, V, C_in) -> (B, T, V, C_in)
        # A_norm @ x for each batch and time
        x = torch.einsum('vw,btvc->btwc', self.A_norm, x)

        # Apply spatial weights: (B, T, V, C_in) -> (B, T, V, C_out)
        x = self.W_spatial(x)

        return x


class TemporalConv(nn.Module):
    """
    Temporal Convolution Layer (TCN).

    Uses 1D convolution over time dimension with optional dilation.
    kernel_size=9, padding=4 for receptive field coverage.
    """

    def __init__(self, in_channels, out_channels, kernel_size=9, dilation=1):
        super(TemporalConv, self).__init__()
        self.conv = nn.Conv2d(
            in_channels, out_channels,
            kernel_size=(kernel_size, 1),
            padding=(kernel_size // 2 * dilation, 0),
            dilation=(dilation, 1)
        )
        self.bn = nn.BatchNorm2d(out_channels)

    def forward(self, x):
        # x: (B, T, V, C) -> (B, C, T, V) for Conv2d
        x = x.permute(0, 3, 1, 2)
        x = self.conv(x)
        x = self.bn(x)
        x = x.permute(0, 2, 3, 1)  # Back to (B, T, V, C)
        return x


class STConvBlock(nn.Module):
    """
    ST-Conv Block: Spatial + Temporal convolution with residual connection.

    Structure: BN -> ReLU -> SpatialGraphConv -> BN -> ReLU -> TemporalConv -> BN -> + Residual
    """

    def __init__(self, in_channels, out_channels, A_norm, stride=1, residual=True):
        super(STConvBlock, self).__init__()

        self.bn1 = nn.BatchNorm2d(in_channels)
        self.graph_conv = GraphConv(in_channels, out_channels, A_norm)
        self.bn2 = nn.BatchNorm2d(out_channels)
        self.temporal_conv = TemporalConv(out_channels, out_channels)
        self.bn3 = nn.BatchNorm2d(out_channels)

        # Residual connection
        if residual:
            if in_channels != out_channels:
                self.residual = nn.Sequential(
                    nn.Conv2d(in_channels, out_channels, kernel_size=1),
                    nn.BatchNorm2d(out_channels)
                )
            else:
                self.residual = nn.Identity()
        else:
            self.residual = None

        self.stride = stride

    def forward(self, x):
        # x: (B, T, V, C)
        residual = x

        # BN -> ReLU (applied per channel)
        x = x.permute(0, 3, 1, 2)  # (B, C, T, V)
        x = self.bn1(x)
        x = F.relu(x)
        x = x.permute(0, 2, 3, 1)  # (B, T, V, C)

        # Spatial Graph Conv
        x = self.graph_conv(x)

        # BN -> ReLU
        x = x.permute(0, 3, 1, 2)
        x = self.bn2(x)
        x = F.relu(x)
        x = x.permute(0, 2, 3, 1)

        # Temporal Conv
        x = self.temporal_conv(x)

        # BN
        x = x.permute(0, 3, 1, 2)
        x = self.bn3(x)
        x = x.permute(0, 2, 3, 1)

        # Residual
        if self.residual is not None:
            residual = residual.permute(0, 3, 1, 2)
            residual = self.residual(residual)
            residual = residual.permute(0, 2, 3, 1)
            x = x + residual

        x = F.relu(x)

        return x


class AttentionPooling(nn.Module):
    """
    Channel Attention Pooling (SE-Net style).

    Aggregates temporal and spatial features with attention weights.
    """

    def __init__(self, in_channels, reduction=4):
        super(AttentionPooling, self).__init__()
        self.global_pool = nn.AdaptiveAvgPool2d(1)
        self.fc1 = nn.Linear(in_channels, in_channels // reduction)
        self.fc2 = nn.Linear(in_channels // reduction, in_channels)

    def forward(self, x):
        # x: (B, T, V, C)
        B, T, V, C = x.shape

        # Global average pooling over T and V
        x_pool = x.mean(dim=(1, 2))  # (B, C)

        # Attention weights
        attn = F.relu(self.fc1(x_pool))
        attn = torch.sigmoid(self.fc2(attn))  # (B, C)

        # Apply attention
        x = x * attn.unsqueeze(1).unsqueeze(2)  # (B, T, V, C)

        # Final pooling
        x = x.mean(dim=(1, 2))  # (B, C)

        return x


class STGCNModel(nn.Module):
    """
    Full ST-GCN Model for Sign Language Gesture Classification.

    Architecture:
    - 9 ST-Conv Blocks with channel progression [64,64,64,64,128,128,128,256,256]
    - Attention Pooling
    - Classification Head

    Input: (Batch, Time=30, Landmarks=21, Coords=2)
    Output: (Batch, num_classes=46)
    """

    def __init__(self, num_classes=46, in_channels=2):
        super(STGCNModel, self).__init__()

        # Build normalized adjacency matrix
        A = get_hand_skeleton_adjacency()
        A_norm = normalize_adjacency(A)

        # Channel progression
        channels = [in_channels, 64, 64, 64, 64, 128, 128, 128, 256, 256]

        # ST-Conv Blocks (9 blocks)
        self.st_blocks = nn.ModuleList()
        for i in range(len(channels) - 1):
            self.st_blocks.append(
                STConvBlock(channels[i], channels[i+1], A_norm)
            )

        # Attention Pooling
        self.attention_pool = AttentionPooling(channels[-1])

        # Classification Head
        self.fc = nn.Linear(channels[-1], num_classes)

    def forward(self, x):
        # x: (B, T=30, V=21, C=2)

        # Apply ST-Conv Blocks
        for block in self.st_blocks:
            x = block(x)

        # Attention Pooling
        x = self.attention_pool(x)  # (B, C)

        # Classification
        x = self.fc(x)  # (B, num_classes)

        return x

    def get_model_info(self):
        """Print model parameter count and structure info."""
        total_params = sum(p.numel() for p in self.parameters())
        trainable_params = sum(p.numel() for p in self.parameters() if p.requires_grad)
        print(f"Total Parameters: {total_params:,}")
        print(f"Trainable Parameters: {trainable_params:,}")
        return total_params


class PseudoSkeletonMapper(nn.Module):
    """
    Pseudo-Skeleton Mapping Layer (Eq.16).

    Maps 21-dimensional sensor features (15 Hall + 6 IMU) to
    2D coordinates of 21 hand keypoints.

    Input: 21 features
    Output: 42 values (21 keypoints × 2D coordinates)
    """

    def __init__(self):
        super(PseudoSkeletonMapper, self).__init__()
        self.mapping = nn.Linear(21, 42)

    def forward(self, x):
        # x: (B, 21) or (B, T, 21)
        return self.mapping(x)


def test_stgcn():
    """Test ST-GCN forward pass."""
    model = STGCNModel(num_classes=46, in_channels=2)
    model.get_model_info()

    # Test input: (B=2, T=30, V=21, C=2)
    x = torch.randn(2, 30, 21, 2)

    # Forward pass
    output = model(x)
    print(f"Input shape: {x.shape}")
    print(f"Output shape: {output.shape}")

    # Verify output
    assert output.shape == (2, 46), f"Expected (2, 46), got {output.shape}"
    print("✓ Forward pass successful!")

    return model


if __name__ == "__main__":
    test_stgcn()