

#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "tokens/token.h"
#include "ast/astnode.h"
#include "parser/parser.h"
#include "evaluator/evaluator.h"
#include "lexer/lex.h"
#include "../tables/table.h"

#define MAX_SOURCE_LENGTH 16384

static int input_line_number;

static int brace_depth(const char *source) {
    int depth = 0;
    bool in_string = false;
    bool in_comment = false;
    for (size_t i = 0; source[i] != '\0'; i++) {
        if (in_comment) {
            if (source[i] == '\n') {
                in_comment = false;
            }
            continue;
        }
        if (source[i] == '"' && (i == 0 || source[i - 1] != '\\')) {
            in_string = !in_string;
        } else if (!in_string && source[i] == '/' && source[i + 1] == '/') {
            in_comment = true;
            i++;
        } else if (!in_string && source[i] == '{') {
            depth++;
        } else if (!in_string && source[i] == '}') {
            depth--;
        }
    }
    return depth;
}

static bool begins_with_keyword(const char *source, const char *keyword) {
    while (isspace((unsigned char)*source)) {
        source++;
    }
    size_t length = strlen(keyword);
    return strncmp(source, keyword, length) == 0 &&
           (source[length] == '\0' ||
            (!isalnum((unsigned char)source[length]) && source[length] != '_'));
}

static bool append_line(char *source, size_t capacity, const char *line) {
    size_t used = strlen(source);
    size_t remaining = capacity - used;
    if (strlen(line) + 1 > remaining) {
        printf("input error: program block is too large\n");
        source[0] = '\0';
        return false;
    }
    snprintf(source + used, remaining, "%s", line);
    return true;
}

static int read_input_line(FILE *input, char *line, size_t capacity) {
    if (fgets(line, (int)capacity, input) == NULL) {
        return -1;
    }
    input_line_number++;

    if (strchr(line, '\n') == NULL && !feof(input)) {
        int character;

        while ((character = fgetc(input)) != '\n' && character != EOF) {
        }

        printf("input error: line exceeds the maximum length\n");
        return 0;
    }

    return 1;
}

static bool should_echo_result(astnode *program) {
    astnode *last = program;
    while (last != NULL && last->kind == AST_NODE_SEQUENCE &&
           last->right != NULL) {
        last = last->right;
    }
    if (last == NULL || last->kind != AST_NODE_SEQUENCE ||
        last->left == NULL) {
        return false;
    }
    astnode_kind kind = last->left->kind;
    return kind != AST_NODE_PRINT &&
           kind != AST_NODE_FUNCTION &&
           kind != AST_NODE_IF &&
           kind != AST_NODE_WHILE &&
           kind != AST_NODE_RETURN &&
           kind != AST_NODE_BREAK;
}

static void execute_source(char *source, int starting_line) {
    Token *tokens = lex_input(source, starting_line);
    if (tokens == NULL) {
        return;
    }
    Token *current = tokens;
    astnode *program = parse_program(&current);
    if (program != NULL && current == NULL) {
        variable_value result = evaluation(program);
        if (result.valid && should_echo_result(program)) {
            printf(">>> ");
            print_value(result);
            printf("\n");
        }
        if (result.break_loop) {
            printf("runtime error: 'break' used outside a loop\n");
        }
        if (result.returned) {
            printf("runtime error: 'return' used outside a function\n");
        }
    } else if (current != NULL) {
        printf("parser error: unexpected token '%s'\n", current->value);
    }
    free_ast(program);
    free_tokens(tokens);
}

int main(int argc, char *argv[]) {

    FILE *input = stdin;

    // Check command-line arguments
    if (argc > 2) {
        printf("Usage: bourmada [filename.bmd]\n");
        return 1;
    }

    // Open source file if provided
    if (argc == 2) {
        input = fopen(argv[1], "r");

        if (input == NULL) {
            perror("bourmada");
            return 1;
        }
    }

    bool interactive = (argc == 1 && isatty(fileno(stdin)));

    char line[MAX_LENGTH];
    char pending_line[MAX_LENGTH];
    char source[MAX_SOURCE_LENGTH] = "";
    int source_starting_line = 1;
    int pending_line_number = 0;

    bool has_pending_line = false;

    if (interactive) {
        printf("starting.....\n");
    }

    for (;;) {
        int current_line_number;

        if (interactive) printf("~");

        if (has_pending_line) {
            snprintf(line, sizeof(line), "%s", pending_line);
            current_line_number = pending_line_number;
            has_pending_line = false;
        } else {
            int read_status = read_input_line(input, line, sizeof(line));

            if (read_status < 0) break;
            current_line_number = input_line_number;

            if (read_status == 0) {
                source[0] = '\0';
                continue;
            }
        }

        if (source[0] == '\0') {
            source_starting_line = current_line_number;
        }
        if (!append_line(source, sizeof(source), line)) {
            continue;
        }

        int depth = brace_depth(source);

        if (depth < 0) {
            printf("parser error: unexpected '}'\n");
            source[0] = '\0';
            continue;
        }

        while (depth > 0) {

            if (interactive) printf("~");

            int read_status = read_input_line(input, line, sizeof(line));

            if (read_status <= 0) {
                if (read_status < 0) {
                    printf("parser error: incomplete block at end of input\n");
                }

                source[0] = '\0';
                break;
            }

            if (!append_line(source, sizeof(source), line)) {
                source[0] = '\0';
                depth = 0;
                break;
            }

            depth = brace_depth(source);

            if (depth < 0) {
                printf("parser error: unexpected '}'\n");
                source[0] = '\0';
                break;
            }
        }

        if (source[0] == '\0' || depth != 0) {
            continue;
        }

        if (begins_with_keyword(source, "if")) {

            for (;;) {

                if (interactive) printf("~");

                int read_status = read_input_line(input, line, sizeof(line));

                if (read_status < 0) break;

                if (read_status == 0) {
                    source[0] = '\0';
                    break;
                }

                if (!begins_with_keyword(line, "else")) {
                    snprintf(pending_line, sizeof(pending_line), "%s", line);
                    pending_line_number = input_line_number;
                    has_pending_line = true;
                    break;
                }

                if (!append_line(source, sizeof(source), line)) {
                    source[0] = '\0';
                    break;
                }

                depth = brace_depth(source);

                if (depth < 0) {
                    printf("parser error: unexpected '}'\n");
                    source[0] = '\0';
                    break;
                }

                while (depth > 0) {

                    if (interactive) printf("~");

                    read_status = read_input_line(input, line, sizeof(line));

                    if (read_status <= 0) {
                        if (read_status < 0) {
                            printf("parser error: incomplete block at end of input\n");
                        }

                        source[0] = '\0';
                        depth = 0;
                        break;
                    }

                    if (!append_line(source, sizeof(source), line)) {
                        source[0] = '\0';
                        depth = 0;
                        break;
                    }

                    depth = brace_depth(source);

                    if (depth < 0) {
                        printf("parser error: unexpected '}'\n");
                        source[0] = '\0';
                        break;
                    }
                }

                if (depth != 0 || !strstr(source, "else")) {
                    break;
                }
            }
        }

        execute_source(source, source_starting_line);
        source[0] = '\0';
    }

    if (source[0] != '\0') {
        execute_source(source, source_starting_line);
    }

    if (input != stdin) {
        fclose(input);
    }

    evaluator_cleanup();
    table_cleanup();
    return 0;
}