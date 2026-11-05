# Memory management

Bourmada uses deterministic ownership and scope-based reclamation. Its current
value types contain no dynamically allocated payloads, so runtime values can
be copied and discarded without a tracing garbage collector.

## Automatic variable lifetime

Variables are stored in a fixed-size array. Entering a block or function
records the current end of that array. When the scope exits, all variables
created since that marker are removed from the variable index and the slots
are made available for reuse. This applies to normal completion, `return`,
and `break`; the evaluator leaves scopes on each corresponding control-flow
path.

Top-level variables are intentionally retained across REPL submissions and
remain available until the process exits.

The variable hash table uses deleted-slot markers so removing a local variable
does not require rebuilding the entire table on every loop iteration. When
enough deleted entries accumulate, the table is compacted from the active
variables.

## Allocation ownership

| Allocation | Owner | Release point |
|---|---|---|
| Token nodes | One submitted input | `free_tokens` after evaluation or on lexer failure |
| Parsed AST nodes | One submitted input | `free_ast` after evaluation |
| Retained function AST | Evaluator function registry | Replaced definition or `evaluator_cleanup` at process exit |
| Table row buffers | Table subsystem | `table_cleanup` at process exit |
| Variable and scope storage | Static interpreter storage | Local entries are reclaimed at scope exit; global entries last until process exit |

Strings and other current value data are stored inline, not allocated
separately. New dynamically allocated value types will need an explicit
ownership policy and cleanup path before they are added.

## Cleanup on errors

The lexer releases any tokens already created when tokenization fails. The
execution path releases both the parsed tree and token list after handling a
program. Function replacement frees the old retained definition, and process
shutdown frees retained function trees and table row buffers.

These are deterministic lifetime rules, not a tracing garbage collector.
