# Performance and benchmarks

The interpreter's benchmark runner measures wall-clock time, including process
startup, lexing, parsing, and evaluation. It verifies each workload's printed
result before recording a report.

## Running benchmarks

Build and run all workloads:

```sh
make benchmark
```

Choose the number of repetitions:

```sh
python3 tests/benchmark.py --runs 10
```

The runner writes one report per workload under `benchmarkresults/`. Results
vary with the machine and system load; compare repeated runs on the same
machine and build.

## Optimized hot paths

- AST nodes cache an enum kind when they are created. Evaluation branches on
  this tag instead of repeatedly comparing node-type strings for every
  expression in a loop.
- Scope exit removes only variables declared in that scope. Deleted hash
  slots preserve probe chains, avoiding a full variable-table rebuild on each
  block exit. Accumulated deleted slots are periodically compacted.
- Scope identifiers are based on active stack depth and are reused after a
  scope exits, avoiding monotonically increasing IDs during long-running
  loops.

## Current measurements

The checked-in loop report describes a 10,000,000-iteration workload with a
median of about 4.00 seconds. A local run of the optimized interpreter in this
environment completed the same workload in about 1.5–1.9 seconds across
several runs. This is a single-machine observation, not a performance
guarantee; use the benchmark commands above to measure a particular build and
environment.

Other workloads and their reports are linked from the
[project home page](../readme.md).
