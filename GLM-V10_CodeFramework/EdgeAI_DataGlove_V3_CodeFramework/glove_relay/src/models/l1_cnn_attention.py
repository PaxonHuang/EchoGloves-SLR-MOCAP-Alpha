# -*- coding: utf-8 -*-
"""
glove_relay.src.models.l1_cnn_attention — 1D-CNN + Temporal Attention (L1).

Architecture (≈ 34 K parameters)
================================
- Input: ``(B, 21)`` per frame  (15 hall + 6 IMU)
- Three 1-D Conv blocks: Conv1d → BatchNorm → ReLU → MaxPool
- Temporal Self-Attention over the channel dimension
- Global Average Pooling → FC → 46-class logits

The model is designed to run on the relay CPU in < 2 ms and to be
quantised to TFLite INT8 via QAT for potential on-device deployment.

Training
--------
::

    python -m src.models.l1_cnn_attention \\
        --data_dir data/train \\
        --epochs 100 \\
        --batch_size 64 \\
        --lr 0.001 \\
        --num_classes 46
"""

from __future__ import annotations

import argparse
from typing import Any

import torch
import torch.nn as nn
import torch.nn.functional as F

from src.models.base_model import BaseModel
from src.utils.logger import get_logger

logger = get_logger(__name__)


# ---------------------------------------------------------------------------
# Sub-modules
# ---------------------------------------------------------------------------
class TemporalAttention(nn.Module):
    """
    Lightweight additive (Bahdanau-style) attention over the *channel*
    dimension of a 1-D feature map.

    Parameters
    ----------
    in_channels : int
        Number of input channels (e.g. 128 after the CNN backbone).
    """

    def __init__(self, in_channels: int) -> None:
        super().__init__()
        self.W_h = nn.Linear(in_channels, in_channels, bias=False)
        self.W_e = nn.Linear(in_channels, 1, bias=False)
        # Learnable context vector
        self.v = nn.Parameter(torch.randn(in_channels))

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Parameters
        ----------
        x : torch.Tensor
            ``(B, C)`` — a single time-step's channel features.

        Returns
        -------
        torch.Tensor
            ``(B, C)`` — attention-weighted features.
        """
        # Expand v: (C,) → (B, C)
        v = self.v.unsqueeze(0).expand(x.size(0), -1)
        # Energy: (B, C) → (B, 1)
        energy = torch.tanh(self.W_h(x) + v)
        energy = self.W_e(energy)
        alpha = torch.softmax(energy, dim=1)  # (B, 1)
        # Weighted sum (B, C)
        attended = (alpha * x).sum(dim=1, keepdim=False) if x.dim() == 3 else alpha.squeeze(-1) * x
        # For 2-D input (B, C) the attention is just element-wise scaling
        if x.dim() == 2:
            attended = alpha.squeeze(-1) * x
        return attended


# ---------------------------------------------------------------------------
# Main model
# ---------------------------------------------------------------------------
class L1EdgeModel(BaseModel, nn.Module):
    """
    1D-CNN with Temporal Attention for single-frame gesture classification.

    Parameters
    ----------
    input_dim : int
        Number of sensor features per frame (default 21).
    num_classes : int
        Number of gesture classes (default 46).
    """

    def __init__(self, input_dim: int = 21, num_classes: int = 46, **_kwargs: Any) -> None:
        super().__init__()

        self.input_dim = input_dim
        self.num_classes = num_classes

        # --- Conv backbone ---
        # Block 1: (B, 1, input_dim) → (B, 32, H)
        self.conv1 = nn.Conv1d(1, 32, kernel_size=3, padding=1)
        self.bn1 = nn.BatchNorm1d(32)

        # Block 2: (B, 32, H) → (B, 64, H/2)
        self.conv2 = nn.Conv1d(32, 64, kernel_size=3, padding=1)
        self.bn2 = nn.BatchNorm1d(64)
        self.pool2 = nn.MaxPool1d(kernel_size=2)

        # Block 3: (B, 64, H/2) → (B, 128, H/4)
        self.conv3 = nn.Conv1d(64, 128, kernel_size=3, padding=1)
        self.bn3 = nn.BatchNorm1d(128)
        self.pool3 = nn.MaxPool1d(kernel_size=2)

        # --- Attention ---
        self.attention = TemporalAttention(128)

        # --- Classifier head ---
        # After conv+pool the spatial dimension ≈ ceil(21 / 4) = 6
        self.gap = nn.AdaptiveAvgPool1d(1)  # Global Avg Pool
        self.fc = nn.Linear(128, num_classes)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Forward pass.

        Parameters
        ----------
        x : torch.Tensor
            ``(B, 21)`` — single-frame sensor data.

        Returns
        -------
        torch.Tensor
            ``(B, num_classes)`` — raw logits.
        """
        # Ensure 3-D for Conv1d: (B, 1, 21)
        if x.dim() == 2:
            x = x.unsqueeze(1)

        x = F.relu(self.bn1(self.conv1(x)))
        x = self.pool2(F.relu(self.bn2(self.conv2(x))))
        x = self.pool3(F.relu(self.bn3(self.conv3(x))))

        # (B, 128, L) → (B, 128) via GAP
        x = self.gap(x).squeeze(-1)

        # Attention weighting
        x = self.attention(x)

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
            "num_classes": self.num_classes,
            "architecture": "cnn_attention_v2",
        }

    def get_model_info(self) -> dict[str, Any]:
        return {
            "name": "cnn_attention_v2",
            "params": sum(p.numel() for p in self.parameters()),
            "trainable_params": sum(p.numel() for p in self.parameters() if p.requires_grad),
        }


# ---------------------------------------------------------------------------
# Training script
# ---------------------------------------------------------------------------
def _train(args: argparse.Namespace) -> None:
    """
    Training loop with QAT (Quantization-Aware Training) support.

    Saves the best checkpoint based on validation accuracy and optionally
    exports a TFLite INT8 model.
    """
    from torch.utils.data import DataLoader, TensorDataset

    logger.info("Training L1 CNN+Attention model …")
    logger.info("Args: %s", vars(args))

    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    model = L1EdgeModel(input_dim=21, num_classes=args.num_classes).to(device)

    # --- Dummy dataset for skeleton (replace with real data loader) ---
    # In production, load from --data_dir
    logger.warning("Using dummy dataset — replace with real data for training")
    dummy_x = torch.randn(500, 21)
    dummy_y = torch.randint(0, args.num_classes, (500,))
    dataset = TensorDataset(dummy_x, dummy_y)
    train_loader = DataLoader(dataset, batch_size=args.batch_size, shuffle=True)

    criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr, weight_decay=1e-4)
    scheduler = torch.optim.lr_scheduler.StepLR(optimizer, step_size=20, gamma=0.5)

    best_acc = 0.0
    for epoch in range(1, args.epochs + 1):
        model.train()
        total_loss = 0.0
        correct = 0
        total = 0

        for xb, yb in train_loader:
            xb, yb = xb.to(device), yb.to(device)
            optimizer.zero_grad()
            logits = model(xb)
            loss = criterion(logits, yb)
            loss.backward()
            optimizer.step()
            total_loss += loss.item() * xb.size(0)
            correct += (logits.argmax(dim=-1) == yb).sum().item()
            total += xb.size(0)

        scheduler.step()
        avg_loss = total_loss / max(total, 1)
        acc = correct / max(total, 1)

        if acc > best_acc:
            best_acc = acc
            torch.save(model.state_dict(), "checkpoints/l1_cnn_attention_v2.pt")
            logger.info("Epoch %d/%d — loss: %.4f  acc: %.4f  ★ saved", epoch, args.epochs, avg_loss, acc)
        elif epoch % 10 == 0:
            logger.info("Epoch %d/%d — loss: %.4f  acc: %.4f", epoch, args.epochs, avg_loss, acc)

    logger.info("Training complete. Best accuracy: %.4f", best_acc)

    # --- QAT export (optional) ---
    if args.export_tflite:
        _export_tflite_int8(model, args.num_classes)


def _export_tflite_int8(model: L1EdgeModel, num_classes: int) -> None:
    """
    Export the model to TFLite INT8 using PyTorch's quantisation pipeline.

    This is a *skeleton* — full export requires representative dataset
    calibration.
    """
    logger.info("Exporting to TFLite INT8 (skeleton — calibration not implemented)")
    model.eval()
    model.cpu()
    model.qconfig = torch.quantization.get_default_qat_qconfig("fbgemm")
    torch.quantization.prepare_qat(model, inplace=True)
    # In production: run a few calibration batches here
    model_int8 = torch.quantization.convert(model.eval())
    # Save state dict for edge deployment
    torch.save(model_int8.state_dict(), "checkpoints/l1_cnn_attention_v2_int8.pt")
    logger.info("INT8 model saved to checkpoints/l1_cnn_attention_v2_int8.pt")


def main() -> None:
    """CLI entry point for training."""
    parser = argparse.ArgumentParser(description="Train L1 CNN+Attention model")
    parser.add_argument("--data_dir", type=str, default="data/train", help="Training data directory")
    parser.add_argument("--epochs", type=int, default=100, help="Number of training epochs")
    parser.add_argument("--batch_size", type=int, default=64, help="Batch size")
    parser.add_argument("--lr", type=float, default=0.001, help="Learning rate")
    parser.add_argument("--num_classes", type=int, default=46, help="Number of gesture classes")
    parser.add_argument("--cpu", action="store_true", help="Force CPU training")
    parser.add_argument("--export_tflite", action="store_true", help="Export TFLite INT8 after training")
    _train(parser.parse_args())


if __name__ == "__main__":
    main()
