#!/usr/bin/env python3
"""Run repeatable Bourmada benchmarks and report wall-clock timings."""

import argparse
import statistics
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INTERPRETER = ROOT / "bin" / "bourmada"
WORKLOADS = (
    ("loop (1,000,000 iterations)", ROOT / "tests" / "performance.bmd", 1_000_000, 1_000_000),
    ("function calls (10,000 calls)", ROOT / "tests" / "benchmark_functions.bmd", 10_000, 10_000),
    ("factorial_recursive (10)", ROOT / "tests" / "recursion.bmd", 3628800, 10),
    ("fibonacci_recursive (30)", ROOT / "tests" / "fibonacci.bmd", 832040, 30),
    ("variable benchmark (1,000,000 iterations)", ROOT / "tests" / "variables_benchmark.bmd", 150, 1_000_000),
)


def run_benchmark(source_path: Path, expected_result: int, runs: int) -> list[float]:
    source = source_path.read_text(encoding="utf-8")
    timings = []

    for _ in range(runs):
        started = time.perf_counter()
        result = subprocess.run(
            [str(INTERPRETER)],
            input=source,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        elapsed = time.perf_counter() - started

        if result.returncode != 0:
            raise RuntimeError(
                f"{source_path.name} exited with status {result.returncode}:\n"
                f"{result.stdout}"
            )

        output_lines = (line.lstrip("~").strip() for line in result.stdout.splitlines())
        if str(expected_result) not in output_lines:
            raise RuntimeError(
                f"{source_path.name} did not print expected result "
                f"{expected_result}:\n{result.stdout}"
            )
        timings.append(elapsed)

    return timings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--runs",
        type=int,
        default=5,
        help="number of timed runs per workload (default: 5)",
    )
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("--runs must be at least 1")
    if not INTERPRETER.is_file():
        print("Bourmada executable not found; build it first with `make`.", file=sys.stderr)
        return 1

    print(f"Bourmada: {INTERPRETER.relative_to(ROOT)}")
    print(f"Runs per workload: {args.runs}")
    print("Times include process startup, lexing, parsing, and evaluation.\n")

    try:
        for label, source_path, expected_result, workload_iterations in WORKLOADS:
            timings = run_benchmark(source_path, expected_result, args.runs)
            median = statistics.median(timings)
            minimum = min(timings)
            rate = workload_iterations / median if median > 0 else float("inf")
            print(
                f"{label}: median {median:.6f}s, "
                f"best {minimum:.6f}s, {rate:,.0f} workload iterations/s"
            )
    except (OSError, RuntimeError) as error:
        print(f"benchmark error: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
