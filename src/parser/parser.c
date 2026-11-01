#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../ast/astnode.h"

static bool is_value(Token *token, const char *value) {
    return token != NULL && strcmp(token->value, value) == 0;
}

static astnode *parse_statement(Token **current);
static astnode *parse_expression_internal(Token **current);

static astnode *new_node(const char *type, const char *value) {
    astnode *node = create_node(type, value);
    if (node == NULL) {
        printf("parser error: out of memory\n");
    }
    return node;
}

static astnode *parse_block(Token **current) {
    if (!is_value(*current, "{")) {
        printf("parser error: expected '{'\n");
        return NULL;
    }
    *current = advance(*current);

    astnode *head = NULL;
    astnode *tail = NULL;
    while (*current != NULL && !is_value(*current, "}")) {
        if (is_value(*current, ";")) {
            *current = advance(*current);
            continue;
        }
        astnode *statement = parse_statement(current);
        if (statement == NULL) {
            free_ast(head);
            return NULL;
        }
        astnode *entry = new_node("sequence", "");
        if (entry == NULL) {
            free_ast(statement);
            free_ast(head);
            return NULL;
        }
        entry->left = statement;
        if (tail == NULL) {
            head = entry;
        } else {
            tail->right = entry;
        }
        tail = entry;
    }

    if (!is_value(*current, "}")) {
        printf("parser error: expected '}'\n");
        free_ast(head);
        return NULL;
    }
    *current = advance(*current);
    return head == NULL ? new_node("sequence", "") : head;
}

static astnode *parse_primary(Token **current) {
    if (*current == NULL) {
        printf("parser error: expected expression\n");
        return NULL;
    }

    if (is_value(*current, "-") || is_value(*current, "!")) {
        char op[25];
        snprintf(op, sizeof(op), "%s", (*current)->value);
        *current = advance(*current);
        astnode *operand = parse_primary(current);
        if (operand == NULL) {
            return NULL;
        }
        astnode *node = new_node("unary", op);
        if (node == NULL) {
            free_ast(operand);
            return NULL;
        }
        node->right = operand;
        return node;
    }

    if (is_value(*current, "(")) {
        *current = advance(*current);
        astnode *expression = parse_expression_internal(current);
        if (!is_value(*current, ")")) {
            printf("parser error: expected ')'\n");
            free_ast(expression);
            return NULL;
        }
        *current = advance(*current);
        return expression;
    }

    if (is_value(*current, "true") || is_value(*current, "false")) {
        astnode *node = new_node("boolean", (*current)->value);
        if (node != NULL) {
            *current = advance(*current);
        }
        return node;
    }

    if (strcmp((*current)->type, "number") == 0 ||
        strcmp((*current)->type, "string") == 0) {
        astnode *node = new_node((*current)->type, (*current)->value);
        if (node != NULL) {
            *current = advance(*current);
        }
        return node;
    }

    if (strcmp((*current)->type, "identifier") == 0) {
        char name[25];
        snprintf(name, sizeof(name), "%s", (*current)->value);
        *current = advance(*current);

        if (is_value(*current, "(")) {
            *current = advance(*current);
            astnode *call = new_node("call", name);
            if (call == NULL) {
                return NULL;
            }
            astnode *tail = NULL;
            while (*current != NULL && !is_value(*current, ")")) {
                astnode *argument = parse_expression_internal(current);
                astnode *entry = new_node("argument", "");
                if (argument == NULL || entry == NULL) {
                    free_ast(argument);
                    free_ast(entry);
                    free_ast(call);
                    return NULL;
                }
                entry->left = argument;
                if (tail == NULL) {
                    call->left = entry;
                } else {
                    tail->right = entry;
                }
                tail = entry;
                if (is_value(*current, ",")) {
                    *current = advance(*current);
                    if (is_value(*current, ")")) {
                        printf("parser error: trailing comma in function arguments\n");
                        free_ast(call);
                        return NULL;
                    }
                } else if (!is_value(*current, ")")) {
                    printf("parser error: expected ',' or ')' in function call\n");
                    free_ast(call);
                    return NULL;
                }
            }
            if (!is_value(*current, ")")) {
                printf("parser error: expected ')' after function arguments\n");
                free_ast(call);
                return NULL;
            }
            *current = advance(*current);
            return call;
        }

        if (is_value(*current, "=")) {
            *current = advance(*current);
            astnode *assignment = new_node("assign", "=");
            astnode *variable = new_node("var", name);
            astnode *value = parse_expression_internal(current);
            if (assignment == NULL || variable == NULL || value == NULL) {
                free_ast(assignment);
                free_ast(variable);
                free_ast(value);
                return NULL;
            }
            assignment->left = variable;
            assignment->right = value;
            return assignment;
        }
        return new_node("identifier", name);
    }

    printf("parser error: unexpected token '%s'\n", (*current)->value);
    return NULL;
}

static astnode *parse_multiplication(Token **current) {
    astnode *left = parse_primary(current);
    while (left != NULL && (*current != NULL) &&
           (is_value(*current, "*") || is_value(*current, "/"))) {
        char op[25];
        snprintf(op, sizeof(op), "%s", (*current)->value);
        *current = advance(*current);
        astnode *right = parse_primary(current);
        astnode *node = new_node("operator", op);
        if (right == NULL || node == NULL) {
            free_ast(left);
            free_ast(right);
            free_ast(node);
            return NULL;
        }
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

static astnode *parse_addition(Token **current) {
    astnode *left = parse_multiplication(current);
    while (left != NULL && (*current != NULL) &&
           (is_value(*current, "+") || is_value(*current, "-"))) {
        char op[25];
        snprintf(op, sizeof(op), "%s", (*current)->value);
        *current = advance(*current);
        astnode *right = parse_multiplication(current);
        astnode *node = new_node("operator", op);
        if (right == NULL || node == NULL) {
            free_ast(left);
            free_ast(right);
            free_ast(node);
            return NULL;
        }
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

static astnode *parse_comparison(Token **current) {
    astnode *left = parse_addition(current);
    while (left != NULL && *current != NULL &&
           (is_value(*current, "<") || is_value(*current, ">") ||
            is_value(*current, "<=") || is_value(*current, ">=") ||
            is_value(*current, "==") || is_value(*current, "!="))) {
        char op[25];
        snprintf(op, sizeof(op), "%s", (*current)->value);
        *current = advance(*current);
        astnode *right = parse_addition(current);
        astnode *node = new_node("operator", op);
        if (right == NULL || node == NULL) {
            free_ast(left);
            free_ast(right);
            free_ast(node);
            return NULL;
        }
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

static astnode *parse_logical_and(Token **current) {
    astnode *left = parse_comparison(current);
    while (left != NULL && is_value(*current, "&&")) {
        *current = advance(*current);
        astnode *right = parse_comparison(current);
        astnode *node = new_node("operator", "&&");
        if (right == NULL || node == NULL) {
            free_ast(left);
            free_ast(right);
            free_ast(node);
            return NULL;
        }
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

static astnode *parse_expression_internal(Token **current) {
    astnode *left = parse_logical_and(current);
    while (left != NULL && is_value(*current, "||")) {
        *current = advance(*current);
        astnode *right = parse_logical_and(current);
        astnode *node = new_node("operator", "||");
        if (right == NULL || node == NULL) {
            free_ast(left);
            free_ast(right);
            free_ast(node);
            return NULL;
        }
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

static astnode *parse_condition_block(Token **current, const char *kind) {
    astnode *condition = NULL;
    if (is_value(*current, "(")) {
        *current = advance(*current);
        condition = parse_expression_internal(current);
        if (condition == NULL || !is_value(*current, ")")) {
            printf("parser error: expected ')' after condition\n");
            free_ast(condition);
            return NULL;
        }
        *current = advance(*current);
    } else {
        printf("parser error: expected '(' before condition\n");
        return NULL;
    }
    if (strcmp(kind, "while") == 0) {
        if (!is_value(*current, "do")) {
            printf("parser error: expected 'do' after while condition\n");
            free_ast(condition);
            return NULL;
        }
        *current = advance(*current);
    }
    astnode *body = parse_block(current);
    if (body == NULL) {
        free_ast(condition);
        return NULL;
    }
    astnode *node = new_node(kind, kind);
    if (node == NULL) {
        free_ast(condition);
        free_ast(body);
        return NULL;
    }
    node->left = condition;
    node->right = body;
    return node;
}

static astnode *parse_if_statement(Token **current) {
    astnode *node = parse_condition_block(current, "if");
    if (node == NULL || !is_value(*current, "else")) {
        return node;
    }

    *current = advance(*current);
    astnode *otherwise;
    if (is_value(*current, "if")) {
        *current = advance(*current);
        otherwise = parse_if_statement(current);
    } else {
        otherwise = parse_block(current);
    }
    if (otherwise == NULL) {
        free_ast(node);
        return NULL;
    }
    astnode *branch = new_node("branches", "");
    if (branch == NULL) {
        free_ast(node);
        free_ast(otherwise);
        return NULL;
    }
    branch->left = node->right;
    branch->right = otherwise;
    node->right = branch;
    return node;
}

static astnode *parse_statement(Token **current) {
    if (is_value(*current, "let")) {
        *current = advance(*current);
        const char *declared_type = "infer";
        if (*current != NULL && (strcmp((*current)->value, "int") == 0 ||
            strcmp((*current)->value, "str") == 0 ||
            strcmp((*current)->value, "bool") == 0)) {
            declared_type = (*current)->value;
            *current = advance(*current);
        }
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected variable name after 'let'\n");
            return NULL;
        }
        astnode *declaration = new_node("let", declared_type);
        astnode *variable = new_node("var", (*current)->value);
        *current = advance(*current);
        if (!is_value(*current, "=")) {
            printf("parser error: expected '=' in declaration\n");
            free_ast(declaration);
            free_ast(variable);
            return NULL;
        }
        *current = advance(*current);
        astnode *value = parse_expression_internal(current);
        if (declaration == NULL || variable == NULL || value == NULL) {
            free_ast(declaration);
            free_ast(variable);
            free_ast(value);
            return NULL;
        }
        declaration->left = variable;
        declaration->right = value;
        return declaration;
    }

    if (is_value(*current, "print")) {
        *current = advance(*current);
        if (!is_value(*current, "(")) {
            printf("parser error: expected '(' after 'print'\n");
            return NULL;
        }
        *current = advance(*current);
        astnode *value = parse_expression_internal(current);
        if (!is_value(*current, ")")) {
            printf("parser error: expected ')' after print expression\n");
            free_ast(value);
            return NULL;
        }
        *current = advance(*current);
        astnode *node = new_node("print", "print");
        if (node == NULL) {
            free_ast(value);
            return NULL;
        }
        node->right = value;
        return node;
    }

    if (is_value(*current, "if")) {
        *current = advance(*current);
        return parse_if_statement(current);
    }

    if (is_value(*current, "while")) {
        *current = advance(*current);
        return parse_condition_block(current, "while");
    }

    if (is_value(*current, "fn")) {
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0 ||
            (strcmp((*current)->value, "int") != 0 &&
             strcmp((*current)->value, "str") != 0 &&
             strcmp((*current)->value, "bool") != 0)) {
            printf("parser error: expected function return type after 'fn'\n");
            return NULL;
        }
        char return_type[25];
        snprintf(return_type, sizeof(return_type), "%s", (*current)->value);
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected function name\n");
            return NULL;
        }
        astnode *function = new_node("function", (*current)->value);
        astnode *metadata = new_node("function_metadata", return_type);
        if (function == NULL || metadata == NULL) {
            free_ast(function);
            free_ast(metadata);
            return NULL;
        }
        *current = advance(*current);
        if (!is_value(*current, "(")) {
            printf("parser error: expected '(' after function name\n");
            free_ast(function);
            free_ast(metadata);
            return NULL;
        }
        *current = advance(*current);
        astnode *params = NULL;
        astnode *tail = NULL;
        while (*current != NULL && !is_value(*current, ")")) {
            const char *parameter_type = "any";
            if (strcmp((*current)->value, "int") == 0 ||
                strcmp((*current)->value, "str") == 0 ||
                strcmp((*current)->value, "bool") == 0) {
                parameter_type = (*current)->value;
                *current = advance(*current);
            }
            if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
                printf("parser error: expected parameter name after type\n");
                free_ast(function);
                free_ast(metadata);
                free_ast(params);
                return NULL;
            }
            astnode *param = new_node("parameter", (*current)->value);
            if (param != NULL) {
                memcpy(param->type, parameter_type, strlen(parameter_type) + 1);
            }
            *current = advance(*current);
            if (param == NULL) {
                free_ast(function);
                free_ast(metadata);
                free_ast(params);
                return NULL;
            }
            if (tail == NULL) {
                params = param;
            } else {
                tail->right = param;
            }
            tail = param;
            if (is_value(*current, ",")) {
                *current = advance(*current);
                if (is_value(*current, ")")) {
                    printf("parser error: trailing comma after function parameter\n");
                    free_ast(function);
                    free_ast(metadata);
                    free_ast(params);
                    return NULL;
                }
            } else if (!is_value(*current, ")")) {
                printf("parser error: expected ',' or ')' after parameter\n");
                free_ast(function);
                free_ast(metadata);
                free_ast(params);
                return NULL;
            }
        }
        if (!is_value(*current, ")")) {
            printf("parser error: expected ')' after parameters\n");
            free_ast(function);
            free_ast(metadata);
            free_ast(params);
            return NULL;
        }
        *current = advance(*current);
        astnode *body = parse_block(current);
        if (body == NULL) {
            free_ast(function);
            free_ast(metadata);
            free_ast(params);
            return NULL;
        }
        metadata->left = params;
        function->left = metadata;
        function->right = body;
        return function;
    }

    if (is_value(*current, "return")) {
        *current = advance(*current);
        astnode *value = parse_expression_internal(current);
        astnode *node = new_node("return", "return");
        if (node == NULL || value == NULL) {
            free_ast(node);
            free_ast(value);
            return NULL;
        }
        node->right = value;
        return node;
    }

    if (is_value(*current, "break")) {
        *current = advance(*current);
        return new_node("break", "break");
    }

    return parse_expression_internal(current);
}

astnode *parse_expression(Token **current) {
    return parse_expression_internal(current);
}

astnode *parse_program(Token **current) {
    astnode *head = NULL;
    astnode *tail = NULL;
    while (*current != NULL) {
        if (is_value(*current, ";")) {
            *current = advance(*current);
            continue;
        }
        astnode *statement = parse_statement(current);
        if (statement == NULL) {
            free_ast(head);
            return NULL;
        }
        astnode *entry = new_node("sequence", "");
        if (entry == NULL) {
            free_ast(statement);
            free_ast(head);
            return NULL;
        }
        entry->left = statement;
        if (tail == NULL) {
            head = entry;
        } else {
            tail->right = entry;
        }
        tail = entry;
    }
    return head;
}
