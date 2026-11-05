# Bourmada

![Bourmada logo](./icon/bourmada.svg)

**Bourmada is an experimental programming language implemented in C.** This
repository contains its interpreter, language reference, runnable programs,
and performance benchmarks.

## Wiki pages

| Page | Description |
|---|---|
| [Language manual](./manual.md) | Syntax, types, control flow, functions, tables, scope, and limits |
| [Architecture](./docs/architecture.md) | Interpreter pipeline and the responsibilities of each module |
| [Memory management](./docs/memory-management.md) | Allocation ownership and automatic lifetime management |
| [Performance and benchmarks](./docs/performance.md) | Hot-path optimizations and how to measure workloads |
| [Syntax quick reference](./examples/syn.txt) | Compact examples of the language syntax |
| [Runnable examples and tests](./tests/) | Example programs and benchmark workloads |

### Benchmark reports

- [While loop](./benchmarkresults/loop.txt)
- [For loop](./benchmarkresults/for_loop.txt)
- [Function calls](./benchmarkresults/function_calls.txt)
- [Recursive factorial](./benchmarkresults/factorial_recursive.txt)
- [Recursive Fibonacci](./benchmarkresults/fibonacci_recursive.txt)
- [Variable access](./benchmarkresults/variables.txt)

## Quick start

Build and start the interactive interpreter:

```sh
make
./bin/bourmada
```

Run a source file:

```sh
./bin/bourmada tests/example.bmd
```

Run the performance suite:

```sh
make benchmark
```

The REPL collects multiple lines while a `{ ... }` block is open. An `else`
can follow the closing brace on the same line or on the next line.

## At a glance

- **Values:** `int`, `str`, and `bool`.
- **Expressions:** arithmetic, comparisons, and short-circuit logical
  operators.
- **Control flow:** `if`/`else`, `while`, `for`, and `break`.
- **Functions:** typed return values, optional parameter types, and recursion.
- **Tables:** typed columns, named-field inserts, updates, deletes, and reads.
- **Scope:** nested blocks can shadow names; function calls cannot access
  variables local to their caller.
- **Memory:** local variable slots are reclaimed when their scopes end;
  temporary tokens and syntax trees are freed after each submitted program.

The interpreter is a learning project and its language and implementation
limits may change. See the [manual](./manual.md) for the full language
reference and current limits.

## License

Bourmada is licensed under the [MIT License](./LICENSE).
