#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../ast/astnode.h"

static bool is_value(Token *token, const char *value) {
    return token != NULL && strcmp(token->value, value) == 0;
}

static bool is_type_name(Token *token) {
    return token != NULL &&
           (strcmp(token->value, "int") == 0 ||
            strcmp(token->value, "str") == 0 ||
            strcmp(token->value, "bool") == 0);
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

static astnode *parse_table_fields(Token **current) {
    if (!is_value(*current, "{")) {
        printf("parser error: expected '{' before table fields\n");
        return NULL;
    }
    *current = advance(*current);

    astnode *head = NULL;
    astnode *tail = NULL;
    while (*current != NULL && !is_value(*current, "}")) {
        if (strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected a table field name\n");
            free_ast(head);
            return NULL;
        }
        int line = (*current)->line;
        char field_name[25];
        snprintf(field_name, sizeof(field_name), "%s", (*current)->value);
        *current = advance(*current);
        if (!is_value(*current, ":")) {
            printf("parser error: expected ':' after table field name\n");
            free_ast(head);
            return NULL;
        }
        *current = advance(*current);
        astnode *value = parse_expression_internal(current);
        astnode *field = new_node("table_field", field_name);
        if (value == NULL || field == NULL) {
            free_ast(value);
            free_ast(field);
            free_ast(head);
            return NULL;
        }
        field->line = line;
        field->left = value;
        if (tail == NULL) {
            head = field;
        } else {
            tail->right = field;
        }
        tail = field;

        if (is_value(*current, ",")) {
            *current = advance(*current);
        } else if (!is_value(*current, "}")) {
            printf("parser error: expected ',' or '}' after table field\n");
            free_ast(head);
            return NULL;
        }
    }
    if (!is_value(*current, "}")) {
        printf("parser error: expected '}' after table fields\n");
        free_ast(head);
        return NULL;
    }
    *current = advance(*current);
    if (head == NULL) {
        printf("parser error: a table row needs at least one field\n");
        return NULL;
    }
    return head;
}

static astnode *parse_table_declaration(Token **current) {
    int line = (*current)->line;
    *current = advance(*current);
    if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
        printf("parser error: expected table name after 'table'\n");
        return NULL;
    }
    char name[25];
    snprintf(name, sizeof(name), "%s", (*current)->value);
    *current = advance(*current);
    if (!is_value(*current, "{")) {
        printf("parser error: expected '{' after table name\n");
        return NULL;
    }
    *current = advance(*current);

    astnode *table = new_node("table_declaration", name);
    astnode *tail = NULL;
    if (table == NULL) {
        return NULL;
    }
    table->line = line;
    while (*current != NULL && !is_value(*current, "}")) {
        if (!is_type_name(*current)) {
            printf("parser error: expected int, str, or bool in table schema\n");
            free_ast(table);
            return NULL;
        }
        char field_type[25];
        snprintf(field_type, sizeof(field_type), "%s", (*current)->value);
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected column name after its type\n");
            free_ast(table);
            return NULL;
        }
        astnode *column = new_node(field_type, (*current)->value);
        if (column == NULL) {
            free_ast(table);
            return NULL;
        }
        column->line = (*current)->line;
        *current = advance(*current);
        if (tail == NULL) {
            table->left = column;
        } else {
            tail->right = column;
        }
        tail = column;
    }
    if (!is_value(*current, "}")) {
        printf("parser error: expected '}' after table schema\n");
        free_ast(table);
        return NULL;
    }
    *current = advance(*current);
    if (table->left == NULL) {
        printf("parser error: a table schema needs at least one column\n");
        free_ast(table);
        return NULL;
    }
    return table;
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
        int source_line = (*current)->line;
        snprintf(name, sizeof(name), "%s", (*current)->value);
        *current = advance(*current);

        if (is_value(*current, "[")) {
            *current = advance(*current);
            astnode *row_index = parse_expression_internal(current);
            if (row_index == NULL || !is_value(*current, "]")) {
                printf("parser error: expected ']' after table row index\n");
                free_ast(row_index);
                return NULL;
            }
            *current = advance(*current);
            if (!is_value(*current, ".")) {
                printf("parser error: expected '.' and a column name after table row\n");
                free_ast(row_index);
                return NULL;
            }
            *current = advance(*current);
            if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
                printf("parser error: expected table column name after '.'\n");
                free_ast(row_index);
                return NULL;
            }
            astnode *column = new_node("table_column_ref", (*current)->value);
            astnode *access = new_node("table_access", name);
            if (column == NULL || access == NULL) {
                free_ast(row_index);
                free_ast(column);
                free_ast(access);
                return NULL;
            }
            column->line = (*current)->line;
            access->line = source_line;
            access->left = row_index;
            access->right = column;
            *current = advance(*current);
            return access;
        }

        if (is_value(*current, "(")) {
            *current = advance(*current);
            astnode *call = new_node("call", name);
            if (call == NULL) {
                return NULL;
            }
            call->line = source_line;
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
            if (assignment != NULL) {
                assignment->line = source_line;
            }
            if (variable != NULL) {
                variable->line = source_line;
            }
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
        astnode *identifier = new_node("identifier", name);
        if (identifier != NULL) {
            identifier->line = source_line;
        }
        return identifier;
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

static astnode *parse_for_statement(Token **current) {
    int source_line = (*current)->line;
    *current = advance(*current);
    if (!is_value(*current, "(")) {
        printf("parser error: expected '(' after 'for'\n");
        return NULL;
    }
    *current = advance(*current);

    astnode *initializer = NULL;
    char counter_name[25] = "";
    if (is_value(*current, "int") || is_value(*current, "let")) {
        if (is_value(*current, "let")) {
            *current = advance(*current);
        }
        if (!is_value(*current, "int")) {
            printf("parser error: for-loop counters must have type 'int'\n");
            return NULL;
        }
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected counter name in for-loop header\n");
            return NULL;
        }

        int counter_line = (*current)->line;
        snprintf(counter_name, sizeof(counter_name), "%s", (*current)->value);
        astnode *declaration = new_node("let", "int");
        astnode *variable = new_node("var", counter_name);
        if (variable != NULL) {
            variable->line = counter_line;
        }
        *current = advance(*current);

        astnode *initial_value;
        if (is_value(*current, "=")) {
            *current = advance(*current);
            initial_value = parse_expression_internal(current);
        } else {
            initial_value = new_node("number", "0");
            if (initial_value != NULL) {
                initial_value->line = counter_line;
            }
        }
        if (declaration == NULL || variable == NULL || initial_value == NULL) {
            free_ast(declaration);
            free_ast(variable);
            free_ast(initial_value);
            return NULL;
        }
        declaration->line = counter_line;
        declaration->left = variable;
        declaration->right = initial_value;
        initializer = declaration;

        if (!is_value(*current, ";")) {
            printf("parser error: expected ';' after for-loop counter\n");
            free_ast(initializer);
            return NULL;
        }
        *current = advance(*current);
    }

    astnode *condition = parse_expression_internal(current);
    if (condition == NULL || !is_value(*current, ")")) {
        printf("parser error: expected ')' after for-loop condition\n");
        free_ast(initializer);
        free_ast(condition);
        return NULL;
    }
    *current = advance(*current);
    if (!is_value(*current, "do")) {
        printf("parser error: expected 'do' after for-loop header\n");
        free_ast(initializer);
        free_ast(condition);
        return NULL;
    }
    *current = advance(*current);

    astnode *body = parse_block(current);
    if (body == NULL) {
        free_ast(initializer);
        free_ast(condition);
        return NULL;
    }

    astnode *header = new_node("for_header", counter_name);
    astnode *loop = new_node("for", "for");
    if (header == NULL || loop == NULL) {
        free_ast(initializer);
        free_ast(condition);
        free_ast(body);
        free_ast(header);
        free_ast(loop);
        return NULL;
    }
    header->line = source_line;
    header->left = initializer;
    header->right = condition;
    loop->line = source_line;
    loop->left = header;
    loop->right = body;
    return loop;
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
    if (is_value(*current, "table")) {
        return parse_table_declaration(current);
    }

    if (is_value(*current, "insert")) {
        int line = (*current)->line;
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected table name after 'insert'\n");
            return NULL;
        }
        astnode *insert = new_node("table_insert", (*current)->value);
        if (insert == NULL) {
            return NULL;
        }
        insert->line = line;
        *current = advance(*current);
        insert->left = parse_table_fields(current);
        if (insert->left == NULL) {
            free_ast(insert);
            return NULL;
        }
        return insert;
    }

    if (is_value(*current, "delete")) {
        int line = (*current)->line;
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected table name after 'delete'\n");
            return NULL;
        }
        astnode *remove = new_node("table_delete", (*current)->value);
        if (remove == NULL) {
            return NULL;
        }
        remove->line = line;
        *current = advance(*current);
        if (!is_value(*current, "[")) {
            printf("parser error: expected '[' before deleted row index\n");
            free_ast(remove);
            return NULL;
        }
        *current = advance(*current);
        remove->left = parse_expression_internal(current);
        if (remove->left == NULL || !is_value(*current, "]")) {
            printf("parser error: expected ']' after deleted row index\n");
            free_ast(remove);
            return NULL;
        }
        *current = advance(*current);
        return remove;
    }

    if (is_value(*current, "update")) {
        int line = (*current)->line;
        *current = advance(*current);
        if (*current == NULL || strcmp((*current)->type, "identifier") != 0) {
            printf("parser error: expected table name after 'update'\n");
            return NULL;
        }
        astnode *update = new_node("table_update", (*current)->value);
        if (update == NULL) {
            return NULL;
        }
        update->line = line;
        *current = advance(*current);
        if (!is_value(*current, "[")) {
            printf("parser error: expected '[' before updated row index\n");
            free_ast(update);
            return NULL;
        }
        *current = advance(*current);
        update->left = parse_expression_internal(current);
        if (update->left == NULL || !is_value(*current, "]")) {
            printf("parser error: expected ']' after updated row index\n");
            free_ast(update);
            return NULL;
        }
        *current = advance(*current);
        update->right = parse_table_fields(current);
        if (update->right == NULL) {
            free_ast(update);
            return NULL;
        }
        return update;
    }

    if (is_value(*current, "let")) {
        int declaration_line = (*current)->line;
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
        if (declaration != NULL) {
            declaration->line = declaration_line;
        }
        if (variable != NULL) {
            variable->line = (*current)->line;
        }
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
        int source_line = (*current)->line;
        *current = advance(*current);
        astnode *node = parse_if_statement(current);
        if (node != NULL) {
            node->line = source_line;
        }
        return node;
    }

    if (is_value(*current, "while")) {
        int source_line = (*current)->line;
        *current = advance(*current);
        astnode *node = parse_condition_block(current, "while");
        if (node != NULL) {
            node->line = source_line;
        }
        return node;
    }

    if (is_value(*current, "for")) {
        return parse_for_statement(current);
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
                param->line = (*current)->line;
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
