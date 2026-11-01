#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "tokens/token.h"
#include "ast/astnode.h"
#include "parser/parser.h"
#include "evaluator/evaluator.h"
#include "lexer/lex.h"

#define MAX_SOURCE_LENGTH 16384

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

static int read_input_line(char *line, size_t capacity) {
    if (fgets(line, (int)capacity, stdin) == NULL) {
        return -1;
    }
    if (strchr(line, '\n') == NULL && !feof(stdin)) {
        int character;
        while ((character = getchar()) != '\n' && character != EOF) {
        }
        printf("input error: line exceeds the maximum length\n");
        return 0;
    }
    return 1;
}

static bool should_echo_result(astnode *program) {
    astnode *last = program;
    while (last != NULL && strcmp(last->type, "sequence") == 0 &&
           last->right != NULL) {
        last = last->right;
    }
    if (last == NULL || strcmp(last->type, "sequence") != 0 ||
        last->left == NULL) {
        return false;
    }
    const char *type = last->left->type;
    return strcmp(type, "print") != 0 &&
           strcmp(type, "function") != 0 &&
           strcmp(type, "if") != 0 &&
           strcmp(type, "while") != 0 &&
           strcmp(type, "return") != 0 &&
           strcmp(type, "break") != 0;
}

static void execute_source(char *source) {
    Token *tokens = lex_input(source);
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

int main(void) {
    char line[MAX_LENGTH];
    char pending_line[MAX_LENGTH];
    char source[MAX_SOURCE_LENGTH] = "";
    bool has_pending_line = false;

    printf("starting.....\n");
    for (;;) {
        printf("~");
        if (has_pending_line) {
            snprintf(line, sizeof(line), "%s", pending_line);
            has_pending_line = false;
        } else {
            int read_status = read_input_line(line, sizeof(line));
            if (read_status < 0) {
                break;
            }
            if (read_status == 0) {
                source[0] = '\0';
                continue;
            }
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
            printf("~");
            int read_status = read_input_line(line, sizeof(line));
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
                printf("~");
                int read_status = read_input_line(line, sizeof(line));
                if (read_status < 0) {
                    break;
                }
                if (read_status == 0) {
                    source[0] = '\0';
                    break;
                }
                if (!begins_with_keyword(line, "else")) {
                    snprintf(pending_line, sizeof(pending_line), "%s", line);
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
                    printf("~");
                    read_status = read_input_line(line, sizeof(line));
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

        execute_source(source);
        source[0] = '\0';
    }
    if (source[0] != '\0') {
        execute_source(source);
    }
    return 0;
}
