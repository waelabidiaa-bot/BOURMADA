# Bourmada Language Manual

Bourmada is a small, experimental programming language interpreted by the C
program in this repository. This manual describes the syntax and behavior
currently implemented.

## Getting started

Build the interpreter and start the interactive prompt:

```sh
make
./bin/bourmada
```

Run a source file instead:

```sh
./bin/bourmada tests/example.bmd
```

The REPL accepts expressions and statements one line at a time. It collects
multiple lines when a block is open. Statements do not require semicolons;
semicolons may separate statements.

## Comments and values

Use `//` for a comment through the end of the line. Comments can follow code.
Inside a quoted string, `//` is ordinary text.

The supported value types and literal forms are:

| Type | Examples |
|---|---|
| `int` | `0`, `42`, `-7` |
| `str` | `"hello"`, `"path://name"` |
| `bool` | `true`, `false` |

Strings use double quotes and are limited to 24 characters. Escape sequences
and multiline strings are not supported.

## Variables and scope

Declare a variable with `let`. Its type may be written explicitly or inferred
from the initial value:

```bourmada
let int count = 10
let str name = "Ada"
let bool ready = true
let next_count = count + 1
```

Variables need an initial value. Assignments use an existing variable and
must preserve its type:

```bourmada
count = count + 1
ready = false
```

Names are case-sensitive. Redeclaring a name in the same scope is an error.
A nested block can shadow a variable in an outer scope:

```bourmada
let int value = 1
if (true) {
    let int value = 2
    print(value) // 2
}
print(value) // 1
```

`if`/`else` branches and loop bodies have local scopes. Variables declared in
those scopes disappear when the block exits. Function parameters and local
variables exist only during that call. Functions can access global variables,
but not variables local to their caller. Top-level variables remain available
for later input in the same REPL session. Local variable slots are reclaimed
automatically when a block or function exits, including when execution returns
early or breaks from a loop.

The interpreter also frees each submitted program's tokens and syntax tree
after evaluation. Function definitions are retained for the REPL session, and
table storage and retained functions are released when the interpreter exits.

## Expressions and operators

Integer arithmetic supports:

| Operators | Meaning |
|---|---|
| `+`, `-` | Addition and subtraction |
| `*`, `/` | Multiplication and integer division |
| `-value` | Unary negation |

Multiplication and division bind more tightly than addition and subtraction.
Parentheses can group expressions. Division by zero and integer overflow are
reported as evaluation errors. There is no modulo operator or string
concatenation.

Comparisons are `==`, `!=`, `<`, `>`, `<=`, and `>=`. Equality and inequality
compare values of the same type; values with different types are unequal.
Relational comparisons require integers and produce booleans.

Logical operators are `&&`, `||`, and `!`. `&&` and `||` short-circuit.
Conditions and logical operators accept booleans or integers; integer zero is
false and non-zero is true. Unary `!` requires a boolean.

## Output

Use `print` to write a value followed by a newline:

```bourmada
print(2 + 3)
print("hello")
print(true)
```

Output:

```text
5
hello
true
```

In the REPL, successful expressions and declarations may also display their
result with a `>>>` prefix. `print(...)` does not add that result echo. The
prompt may appear when input is redirected from a file.

## Conditionals

Conditions are enclosed in parentheses. `else` and `else if` are optional:

```bourmada
let int temperature = 18
if (temperature < 10) {
    print("cold")
} else if (temperature < 20) {
    print("mild")
} else {
    print("warm")
}
```

The selected branch runs in its own scope.

## Loops

### `while`

The `do` keyword is required before a `while` body. `break` exits the
innermost loop:

```bourmada
let int count = 0
while (count < 5) do {
    count = count + 1
    if (count == 3) {
        break
    }
}
print(count) // 3
```

### `for`

A condition-only `for` loop behaves like `while`; update the condition inside
the body:

```bourmada
let int count = 0
for (count < 3) do {
    count = count + 1
}
```

A loop may declare an integer counter in its header. The counter starts at
zero and increases by one after each iteration:

```bourmada
let int length = 4
for (int i; i < length) do {
    print(i)
}
```

The counter is local to the loop. An explicit starting value is also
supported:

```bourmada
for (let int i = 3; i < 6) do {
    print(i)
}
```

`break` works in both loop types. There is no `continue` statement.

## Tables

Tables have a fixed schema made of `int`, `str`, and `bool` columns:

```bourmada
table users {
    int id
    str name
    int age
    bool active
}
```

Insert rows by naming their values. Every schema column must be supplied
exactly once, in any order, with a value of the declared type:

```bourmada
insert users {
    id: 1,
    name: "Adam",
    age: 30,
    active: true
}

insert users {
    active: false,
    age: 25,
    name: "Sam",
    id: 2
}
```

Read a column using a zero-based row index:

```bourmada
print(users[0].name) // Adam
print(users[1].age)  // 25
```

The row index is an integer expression. Update one or more columns by
specifying the fields to change; unspecified columns stay as they were:

```bourmada
update users[0] {
    age: 31,
    active: false
}
```

Delete a row with its zero-based index:

```bourmada
delete users[1]
```

After deletion, later rows shift down by one index. Table names must be
unique. Inserts reject missing, repeated, unknown, or wrongly typed fields.
Updates reject repeated, unknown, or wrongly typed fields. Invalid table
names, columns, and row indexes produce table errors.

Limits are 32 tables per interpreter process, 32 columns per table, and 256
rows per table.

## Functions

Functions declare a return type, name, optional parameters, and a body.
Parameters can be typed or left untyped. Untyped parameters accept any
supported value type. Calls must provide the right number of arguments;
typed parameters check argument types. A function must return a value matching
its declared type.

```bourmada
fn int add_two(int value) {
    return value + 2
}

print(add_two(10)) // 12
```

Functions can call themselves recursively:

```bourmada
fn int factorial(int n) {
    if (n <= 1) {
        return 1
    }
    return n * factorial(n - 1)
}

print(factorial(5)) // 120
```

Define a function before calling it. Void functions, prototypes, default
arguments, and named arguments are not supported.

## Errors and execution

The interpreter reports lexer, parser, variable, scope, type, function,
evaluation, and table errors. Examples of runtime errors include undefined
variables, type mismatches, division by zero, and integer overflow. Variable
and scope errors include a source line and a friendly suggestion. Table errors
include a source line when available; other error categories may not include
source locations.

A lexer error rejects the submitted input. A parse error prevents that input
from running. A runtime error stops the remaining statements in the current
execution sequence.

## Limits and unsupported features

Current implementation limits include:

- 24-character identifiers and string values
- 512 tokens per submitted input
- 200 variables
- 32 tables, 32 columns per table, and 256 rows per table
- 64 functions and 64 arguments per function call
- 128 nested scopes
- 4096-byte REPL line buffer and 16384-byte accumulated program buffer

Only `int`, `str`, and `bool` values are implemented. There are no floats,
arrays, general objects, imports, or user-defined types. There is no static
type-checking pass; most type errors are found while evaluating a statement.

## Development and examples

See [examples/syn.txt](./examples/syn.txt) for the concise syntax guide and
[tests/](./tests/) for runnable programs, including [tables.bmd](./tests/tables.bmd),
[for_loop.bmd](./tests/for_loop.bmd), and [recursion.bmd](./tests/recursion.bmd).
