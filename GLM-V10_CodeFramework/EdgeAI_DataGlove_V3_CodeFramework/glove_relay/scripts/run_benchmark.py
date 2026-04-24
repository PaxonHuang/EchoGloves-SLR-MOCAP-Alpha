# -*- coding: utf-8 -*-
"""
glove_relay.scripts.run_benchmark — Model benchmarking tool.

Compares all registered L1 models on:
  - Classification accuracy (5-fold cross-validation)
  - FLOPs (estimated)
  - Inference latency (mean ± std over N runs)
  - Parameter count

Outputs a comparison table in Markdown format.

Usage
-----
    python -m scripts.run_benchmark --data_dir data/train --folds 5 --warmup 10 --runs 50

    # Compare specific models only
    python -m scripts.run_benchmark --models cnn_attention_v2 ms_tcn_v1
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from src.utils.logger import get_logger

logger = get_logger(__name__)


# ---------------------------------------------------------------------------
# FLOPs estimation (lightweight — no external library dependency)
# ---------------------------------------------------------------------------
def _estimate_flops_conv1d(
    in_channels: int,
    out_channels: int,
    kernel_size: int,
    length: int,
) -> int:
    """Estimate multiply-accumulate operations for a Conv1d layer."""
    return in_channels * out_channels * kernel_size * length


def _estimate_flops_linear(in_features: int, out_features: int) -> int:
    return in_features * out_features


def estimate_model_flops(model_info: Dict[str, Any], input_shape: Tuple[int, ...]) -> int:
    """
    Rough FLOPs estimation based on architecture metadata.

    This is an approximation; for exact FLOPs use ``fvcore`` or
    ``ptflops``.
    """
    total = 0
    arch = model_info.get("architecture", "")

    if arch == "cnn_attention_v2":
        # Conv1d(1→32, k=3, L=21) + Conv1d(32→64, k=3, L=11) + Conv1d(64→128, k=3, L=5)
        total += _estimate_flops_conv1d(1, 32, 3, 21)
        total += _estimate_flops_conv1d(32, 64, 3, 21)
        total += _estimate_flops_conv1d(64, 128, 3, 10)
        total += _estimate_flops_linear(128, 46)
    elif arch == "ms_tcn_v1":
        # 3 residual stages with dilated convs
        total += _estimate_flops_conv1d(21, 32, 1, 10)
        total += _estimate_flops_conv1d(32, 32, 3, 10)
        total += _estimate_flops_conv1d(32, 64, 3, 10)
        total += _estimate_flops_conv1d(64, 64, 3, 10)
        total += _estimate_flops_linear(64, 46)
    elif arch == "stgcn_v1":
        # Graph conv + temporal conv — rough estimate
        total += 21 * 21 * 2 * 64  # spatial
        total += 64 * 64 * 3 * 30  # temporal
        total += 64 * 64 * 3 * 30
        total += 128 * 128 * 3 * 30
        total += _estimate_flops_linear(128, 46)

    return total


# ---------------------------------------------------------------------------
# Latency measurement
# ---------------------------------------------------------------------------
def measure_latency(model: Any, input_tensor: Any, warmup: int = 10, runs: int = 50) -> Dict[str, float]:
    """
    Measure inference latency over multiple runs.

    Returns
    -------
    dict with keys: ``mean_ms``, ``std_ms``, ``min_ms``, ``max_ms``.
    """
    import torch

    device = next(model.parameters()).device
    input_tensor = input_tensor.to(device)

    # Warm-up
    with torch.no_grad():
        for _ in range(warmup):
            _ = model(input_tensor)

    if device.type == "cuda":
        torch.cuda.synchronize()

    timings: List[float] = []
    with torch.no_grad():
        for _ in range(runs):
            t0 = time.perf_counter()
            _ = model(input_tensor)
            if device.type == "cuda":
                torch.cuda.synchronize()
            timings.append((time.perf_counter() - t0) * 1000.0)  # ms

    arr = np.array(timings)
    return {
        "mean_ms": float(arr.mean()),
        "std_ms": float(arr.std()),
        "min_ms": float(arr.min()),
        "max_ms": float(arr.max()),
    }


# ---------------------------------------------------------------------------
# Cross-validation
# ---------------------------------------------------------------------------
def cross_validate(
    model_class: Any,
    X: np.ndarray,
    y: np.ndarray,
    num_folds: int = 5,
    epochs: int = 20,
    batch_size: int = 32,
    lr: float = 0.001,
) -> Dict[str, float]:
    """
    Run k-fold cross-validation and return accuracy metrics.

    Parameters
    ----------
    model_class : type
        Model class to instantiate.
    X : np.ndarray
        Feature array ``(N, T, 21)`` or ``(N, 21)``.
    y : np.ndarray
        Label array ``(N,)``.
    num_folds : int
        Number of folds.
    epochs : int
        Training epochs per fold.
    batch_size : int
        Batch size.
    lr : float
        Learning rate.

    Returns
    -------
    dict with ``mean_accuracy``, ``std_accuracy``, ``per_fold``.
    """
    import torch
    from torch.utils.data import DataLoader, Subset, TensorDataset
    from sklearn.model_selection import KFold

    N = len(X)
    indices = np.arange(N)
    np.random.shuffle(indices)

    kf = KFold(n_splits=num_folds, shuffle=False)
    fold_accuracies: List[float] = []

    for fold_idx, (train_idx, val_idx) in enumerate(kf.split(indices)):
        logger.info("  Fold %d/%d — train=%d val=%d", fold_idx + 1, num_folds, len(train_idx), len(val_idx))

        X_train, y_train = X[train_idx], y[train_idx]
        X_val, y_val = X[val_idx], y[val_idx]

        train_ds = TensorDataset(
            torch.FloatTensor(X_train), torch.LongTensor(y_train)
        )
        val_ds = TensorDataset(
            torch.FloatTensor(X_val), torch.LongTensor(y_val)
        )
        train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True)
        val_loader = DataLoader(val_ds, batch_size=batch_size)

        model = model_class()
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        model = model.to(device)

        criterion = torch.nn.CrossEntropyLoss()
        optimizer = torch.optim.Adam(model.parameters(), lr=lr)

        # Train
        model.train()
        for epoch in range(epochs):
            for xb, yb in train_loader:
                xb, yb = xb.to(device), yb.to(device)
                optimizer.zero_grad()
                logits = model(xb)
                loss = criterion(logits, yb)
                loss.backward()
                optimizer.step()

        # Evaluate
        model.eval()
        correct = 0
        total = 0
        with torch.no_grad():
            for xb, yb in val_loader:
                xb, yb = xb.to(device), yb.to(device)
                logits = model(xb)
                preds = logits.argmax(dim=-1)
                correct += (preds == yb).sum().item()
                total += len(yb)

        acc = correct / max(total, 1)
        fold_accuracies.append(acc)
        logger.info("  Fold %d accuracy: %.4f", fold_idx + 1, acc)

        del model
        if device.type == "cuda":
            torch.cuda.empty_cache()

    arr = np.array(fold_accuracies)
    return {
        "mean_accuracy": float(arr.mean()),
        "std_accuracy": float(arr.std()),
        "per_fold": fold_accuracies,
    }


# ---------------------------------------------------------------------------
# Markdown report generator
# ---------------------------------------------------------------------------
def generate_report(results: List[Dict[str, Any]]) -> str:
    """Format benchmark results as a Markdown table."""
    lines = [
        "# Model Benchmark Report",
        "",
        f"| {'Model':<25s} | {'Params':>8s} | {'FLOPs':>10s} | "
        f"| {'Accuracy':>10s} | {'Latency (ms)':>14s} |",
        f"|:{'—' * 25}:|:{'—' * 8}:|:{'—' * 10}:|"
        f":|:{'—' * 10}:|:{'—' * 14}:|",
    ]

    for r in results:
        name = r.get("name", "?")
        params = r.get("params", 0)
        flops = r.get("flops", 0)
        acc = r.get("mean_accuracy", 0.0)
        acc_std = r.get("std_accuracy", 0.0)
        lat = r.get("mean_latency_ms", 0.0)
        lat_std = r.get("std_latency_ms", 0.0)

        lines.append(
            f"| {name:<25s} | {params:>8,d} | {flops:>10,d} | "
            f"| {acc * 100:>6.2f}±{acc_std * 100:<4.2f} | {lat:>6.2f}±{lat_std:<4.2f} |"
        )

    lines.append("")
    lines.append(f"*Generated at {time.strftime('%Y-%m-%d %H:%M:%S')}*")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> None:
    """CLI entry point for the benchmark tool."""
    parser = argparse.ArgumentParser(description="Benchmark L1/L2 gesture models")
    parser.add_argument("--data_dir", type=str, default="data/train", help="Training data directory")
    parser.add_argument("--models", nargs="+", default=None, help="Models to benchmark (default: all)")
    parser.add_argument("--folds", type=int, default=5, help="Cross-validation folds")
    parser.add_argument("--epochs", type=int, default=20, help="Training epochs per fold")
    parser.add_argument("--batch_size", type=int, default=32, help="Batch size")
    parser.add_argument("--lr", type=float, default=0.001, help="Learning rate")
    parser.add_argument("--warmup", type=int, default=10, help="Latency warm-up runs")
    parser.add_argument("--runs", type=int, default=50, help="Latency measurement runs")
    parser.add_argument("--output", type=str, default="data/benchmark_report.md", help="Output report path")
    args = parser.parse_args()

    print("=" * 60)
    print("  EdgeAI DataGlove V3 — Model Benchmark")
    print("=" * 60)

    # Model catalogue
    from src.models.l1_cnn_attention import L1EdgeModel
    from src.models.l1_ms_tcn import MSTCNModel
    from src.models.stgcn_model import STGCNModel

    all_models = {
        "cnn_attention_v2": L1EdgeModel,
        "ms_tcn_v1": MSTCNModel,
        "stgcn_v1": STGCNModel,
    }

    target_models = args.models or list(all_models.keys())

    # Load or generate dummy data
    data_dir = Path(args.data_dir)
    if data_dir.exists():
        # Try to load NPY files
        npy_files = sorted(data_dir.glob("*.npy"))
        if npy_files:
            samples = [np.load(f) for f in npy_files[:500]]
            X = np.array([s.reshape(-1) for s in samples], dtype=np.float32)
            y = np.random.randint(0, 46, size=len(X))
            print(f"  Loaded {len(X)} samples from {data_dir}")
        else:
            X, y = None, None
    else:
        X, y = None, None

    if X is None:
        print("  ⚠ No training data found — using dummy data for demonstration")
        X = np.random.randn(200, 21).astype(np.float32)
        y = np.random.randint(0, 46, size=200)

    results: List[Dict[str, Any]] = []

    for model_name in target_models:
        if model_name not in all_models:
            print(f"  ⚠ Unknown model: {model_name}")
            continue

        model_cls = all_models[model_name]
        print(f"\n--- Benchmarking {model_name} ---")

        # Instantiate
        model = model_cls()
        info = model.get_model_info()
        print(f"  Parameters: {info['params']:,}")

        # FLOPs
        if model_name == "stgcn_v1":
            input_shape = (1, 30, 21)
        else:
            input_shape = (1, 21)
        flops = estimate_model_flops(model.get_config(), input_shape)
        print(f"  FLOPs:     {flops:,}")

        # Latency
        import torch
        dummy = torch.randn(input_shape)
        latency = measure_latency(model, dummy, warmup=args.warmup, runs=args.runs)
        print(f"  Latency:   {latency['mean_ms']:.2f} ± {latency['std_ms']:.2f} ms")

        # Cross-validation (use CPU to avoid GPU memory issues)
        cv = cross_validate(
            model_cls, X, y,
            num_folds=args.folds,
            epochs=args.epochs,
            batch_size=args.batch_size,
            lr=args.lr,
        )
        print(f"  Accuracy:  {cv['mean_accuracy'] * 100:.2f} ± {cv['std_accuracy'] * 100:.2f}%")

        results.append({
            "name": model_name,
            "params": info["params"],
            "flops": flops,
            "mean_accuracy": cv["mean_accuracy"],
            "std_accuracy": cv["std_accuracy"],
            "mean_latency_ms": latency["mean_ms"],
            "std_latency_ms": latency["std_ms"],
        })

    # Generate report
    report = generate_report(results)
    report_path = Path(args.output)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(report, encoding="utf-8")
    print(f"\n📄 Report saved to: {report_path}")
    print("\n" + report)


if __name__ == "__main__":
    main()
