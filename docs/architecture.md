# Architecture

Bourmada is a small C interpreter. A submitted program moves through a
pipeline of input collection, lexing, parsing, evaluation, and cleanup.

## Execution pipeline

1. `main.c` reads source from a file, standard input, or the interactive
   prompt. The REPL groups lines until a block is complete.
2. The lexer converts source characters into a linked list of tokens. Tokens
   retain their source line for diagnostics.
3. The recursive-descent parser consumes the tokens and builds an AST. Nodes
   use `left` and `right` links for child nodes and linked statement lists.
4. The evaluator walks the AST and performs variable operations, arithmetic,
   control flow, function calls, and table operations.
5. The submitted AST and token list are released. Functions and tables that
   persist beyond the submission remain alive until they are replaced or the
   interpreter exits.

## Source modules

| Path | Responsibility |
|---|---|
| `src/main.c` | REPL/file input, block collection, execution, and top-level cleanup |
| `src/lexer/` | Tokenize source text and report lexical errors |
| `src/tokens/` | Token representation and token-list operations |
| `src/parser/` | Parse tokens into expression and statement AST nodes |
| `src/ast/` | AST representation, node-kind tagging, and tree allocation |
| `src/evaluator/` | Evaluate AST nodes and manage retained function definitions |
| `src/variables/` | Variable storage, lexical scopes, and hashed name lookup |
| `tables/` | Typed table schemas, dynamically sized row storage, and table operations |

## Core representations

- **Values** are copied by value. Integers and booleans use an integer field;
  strings use a fixed-size inline buffer.
- **AST nodes** keep their readable type and value strings, plus a cached
  `astnode_kind` tag. The evaluator dispatches on the tag rather than
  repeatedly comparing type strings in hot loops.
- **Variables** live in a fixed-size store and are indexed by an open-addressed
  hash table keyed by name and scope. Active scope frames define the lookup
  chain; function frames begin a new lexical chain rooted at globals.
- **Tables** store schemas inline and grow their row buffers as needed, up to
  the configured row limit.
- **Functions** retain a copy of their declaration AST so the source AST can
  be freed after the current submission.

## Build layout

`make` compiles C files from `src/` and `tables/`, places intermediate object
files in `dir/`, and writes the executable to `bin/bourmada`. Build options
are set in the [makefile](../makefile).
