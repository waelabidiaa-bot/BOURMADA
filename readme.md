# C Interpreter

A small C interpreter for learning language implementation. It tokenizes
input, parses it into an AST, evaluates expressions and statements, and keeps
top-level variables and function definitions available in the REPL.

## Build and run

```sh
make
./bin/interpter_inC
```

The program reads from standard input. Run a source file by redirecting it:

```sh
./bin/interpter_inC < tests/example.lang
```

## Benchmarks

Build and run the benchmark workloads with:

```sh
make benchmark
```

Or choose the number of timed repetitions:

```sh
python3 tests/benchmark.py --runs 10
```

The suite measures a one-million-iteration loop and 10,000 function calls.
It checks each workload's result before reporting median and best wall-clock
times. Timings include process startup, lexing, parsing, and evaluation; use
the same machine/build and compare repeated runs for useful comparisons.

The REPL collects multiple lines while a `{ ... }` block is open. An `else`
can follow the closing brace on the same line or on the next line.
When input is redirected from a file, REPL prompts still appear in the output.

## Language features

- **Comments:** `//` to end of line, including inline comments. `//` inside a
  quoted string is ordinary text.
- **Values and variables:** `int`, `str`, and `bool`; explicit declarations
  (`let int count = 1`) and inferred declarations (`let count = 1`); assignment
  to existing variables.
- **Expressions:** integer `+`, `-`, `*`, `/`; parentheses and unary minus;
  comparisons `==`, `!=`, `<`, `>`, `<=`, `>=`; logical `&&`, `||`, and `!`.
- **Output:** `print(expression)`.
- **Control flow:** `if`, `else if`, `else`, `while (condition) do { ... }`,
  and `break`.
- **Functions:** `fn return_type name(parameters) { ... }`, optional parameter
  types, return values, function calls, and recursion.
- **Scope:** block-local variables in branches and loop bodies; function-local
  parameters and variables; nested declarations can shadow outer names.
- **Diagnostics:** lexer, parser, type, variable, scope, function, and
  evaluation errors; division-by-zero and integer-overflow checks.

For syntax and runnable examples, see [examples/syn.txt](./examples/syn.txt).
The [tests/](./tests/) folder contains example programs, including
[recursion.lang](./tests/recursion.lang).

## Current limitations and known flaws

- Only `int`, `str`, and `bool` are implemented. There are no floats, arrays,
  objects, or user-defined types.
- Strings are limited to 24 characters. Escape sequences, multiline strings,
  and string concatenation are not supported.
- Arithmetic is integer-only; there is no modulo operator.
- Control flow has `if` branches, `while`, and `break`, but no `for` loop or
  `continue`.
- Variables need an initializer. There are no constants, default arguments,
  named arguments, void functions, or prototypes/forward declarations.
- There is no static type-checking pass; most type errors are reported at
  runtime.
- Functions must be defined before they are called.
- Conditions accept booleans and integers (zero is false, non-zero is true).
  `!` requires a boolean; comparisons like `<` and `>` require integers.
- Diagnostics do not include source line or column numbers. A lexer error
  rejects the submitted input; a parse error prevents it from executing; a
  runtime error stops the rest of the current execution sequence.
- Current implementation limits: 24-character identifiers and string values,
  512 tokens per submitted input, 200 variables, 64 functions, 64 arguments per
  function call, 128 nested scopes, a 4096-byte REPL line buffer, and a
  16384-byte accumulated program buffer.
- Successful expressions may be echoed by the REPL using `>>>`. The REPL
  prompt remains visible when running a redirected source file, so output is
  not a clean batch-mode transcript.
- There is no dedicated unit/regression test runner yet. The benchmark runner
  under `tests/` is for performance comparisons and checks workload results.

These are current project limitations, not promises about future versions.
