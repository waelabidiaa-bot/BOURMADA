#include "evaluator.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FUNCTIONS 64

typedef struct {
    char name[25];
    astnode *definition;
} function_entry;

static function_entry functions[MAX_FUNCTIONS];
static int function_count;

static variable_value invalid_value(void) {
    variable_value value = {0};
    return value;
}

static variable_value failure_value(void) {
    variable_value value = invalid_value();
    value.failed = true;
    return value;
}

static variable_value integer_value(int integer) {
    variable_value value = {0};
    snprintf(value.type, sizeof(value.type), "int");
    value.integer = integer;
    value.valid = true;
    return value;
}

static variable_value boolean_value(bool boolean) {
    variable_value value = {0};
    snprintf(value.type, sizeof(value.type), "bool");
    value.integer = boolean ? 1 : 0;
    value.valid = true;
    return value;
}

static variable_value string_value(const char *string) {
    variable_value value = {0};
    snprintf(value.type, sizeof(value.type), "str");
    snprintf(value.string, sizeof(value.string), "%s", string);
    value.valid = true;
    return value;
}

static variable_value checked_integer(long long integer) {
    if (integer < INT_MIN || integer > INT_MAX) {
        printf("evaluation error: integer arithmetic overflow\n");
        return failure_value();
    }
    return integer_value((int)integer);
}

static astnode *copy_ast(const astnode *node) {
    if (node == NULL) {
        return NULL;
    }
    astnode *copy = create_node(node->type, node->value);
    if (copy == NULL) {
        return NULL;
    }
    copy->left = copy_ast(node->left);
    copy->right = copy_ast(node->right);
    if ((node->left != NULL && copy->left == NULL) ||
        (node->right != NULL && copy->right == NULL)) {
        free_ast(copy);
        return NULL;
    }
    return copy;
}

static bool truth_value(variable_value value, bool *truth) {
    if (!value.valid) {
        return false;
    }
    if (strcmp(value.type, "bool") == 0) {
        *truth = value.integer != 0;
        return true;
    }
    if (strcmp(value.type, "int") == 0) {
        *truth = value.integer != 0;
        return true;
    }
    printf("type error: conditions require bool or int values\n");
    return false;
}

static variable_value evaluate_scoped(astnode *body) {
    if (!enter_scope()) {
        return failure_value();
    }
    variable_value result = evaluation(body);
    leave_scope();
    return result;
}

void print_value(variable_value value) {
    if (strcmp(value.type, "int") == 0) {
        printf("%d", value.integer);
    } else if (strcmp(value.type, "str") == 0) {
        printf("%s", value.string);
    } else if (strcmp(value.type, "bool") == 0) {
        printf("%s", value.integer ? "true" : "false");
    }
}

static variable_value evaluate_call(astnode *node) {
    int function_index = -1;
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, node->value) == 0) {
            function_index = i;
            break;
        }
    }
    if (function_index < 0) {
        printf("function error: '%s' is not defined\n", node->value);
        return failure_value();
    }

    astnode *definition = functions[function_index].definition;
    astnode *metadata = definition->left;
    astnode *parameter = metadata->left;
    astnode *argument = node->left;
    variable_value arguments[64];
    int argument_count = 0;
    while (argument != NULL && parameter != NULL && argument_count < 64) {
        arguments[argument_count] = evaluation(argument->left);
        if (!arguments[argument_count].valid) {
            return arguments[argument_count].failed ? arguments[argument_count] : failure_value();
        }
        argument_count++;
        argument = argument->right;
        parameter = parameter->right;
    }
    if (argument != NULL || parameter != NULL) {
        printf("function error: wrong number of arguments for '%s'\n", node->value);
        return failure_value();
    }

    if (!enter_function_scope()) {
        return failure_value();
    }
    parameter = metadata->left;
    for (int i = 0; i < argument_count; i++) {
        if (strcmp(parameter->type, "any") != 0 &&
            strcmp(parameter->type, arguments[i].type) != 0) {
            printf("function error: parameter '%s' of '%s' expects %s, got %s\n",
                   parameter->value, node->value, parameter->type, arguments[i].type);
            leave_scope();
            return failure_value();
        }
        if (!declare_var(parameter->value, arguments[i])) {
            leave_scope();
            return failure_value();
        }
        parameter = parameter->right;
    }

    variable_value result = evaluation(definition->right);
    leave_scope();
    if (result.failed) {
        return result;
    }
    if (!result.returned) {
        printf("function error: '%s' did not return a value\n", node->value);
        return failure_value();
    }
    result.returned = false;
    if (!result.valid || strcmp(result.type, metadata->value) != 0) {
        printf("function error: '%s' must return %s\n", node->value, metadata->value);
        return failure_value();
    }
    return result;
}

variable_value evaluation(astnode *node) {
    if (node == NULL) {
        return invalid_value();
    }
    if (strcmp(node->type, "number") == 0) {
        errno = 0;
        char *end = NULL;
        long parsed = strtol(node->value, &end, 10);
        if (errno == ERANGE || end == node->value || *end != '\0' ||
            parsed < INT_MIN || parsed > INT_MAX) {
            printf("evaluation error: integer literal '%s' is out of range\n", node->value);
            return failure_value();
        }
        return integer_value((int)parsed);
    }
    if (strcmp(node->type, "string") == 0) {
        return string_value(node->value);
    }
    if (strcmp(node->type, "boolean") == 0) {
        return boolean_value(strcmp(node->value, "true") == 0);
    }
    if (strcmp(node->type, "identifier") == 0 ||
        strcmp(node->type, "var") == 0) {
        variable_value value;
        return get_var(node->value, &value) ? value : failure_value();
    }
    if (strcmp(node->type, "let") == 0) {
        variable_value value = evaluation(node->right);
        if (!value.valid) {
            return value.failed ? value : failure_value();
        }
        if (strcmp(node->value, "infer") != 0 &&
            strcmp(node->value, value.type) != 0) {
            printf("type error: expected %s for '%s', got %s\n",
                   node->value, node->left->value, value.type);
            return failure_value();
        }
        return declare_var(node->left->value, value) ? value : failure_value();
    }
    if (strcmp(node->type, "assign") == 0) {
        variable_value value = evaluation(node->right);
        if (!value.valid || !set_var(node->left->value, value)) {
            return value.failed ? value : failure_value();
        }
        return value;
    }
    if (strcmp(node->type, "print") == 0) {
        variable_value value = evaluation(node->right);
        if (!value.valid) {
            return value.failed ? value : failure_value();
        }
        print_value(value);
        printf("\n");
        return value;
    }
    if (strcmp(node->type, "sequence") == 0) {
        variable_value result = invalid_value();
        for (astnode *entry = node; entry != NULL; entry = entry->right) {
            result = evaluation(entry->left);
            if (result.returned || result.break_loop || result.failed) {
                return result;
            }
        }
        return result;
    }
    if (strcmp(node->type, "function") == 0) {
        astnode *copy = copy_ast(node);
        if (copy == NULL) {
            printf("function error: unable to store function '%s'\n", node->value);
            return failure_value();
        }
        for (int i = 0; i < function_count; i++) {
            if (strcmp(functions[i].name, node->value) == 0) {
                free_ast(functions[i].definition);
                functions[i].definition = copy;
                return invalid_value();
            }
        }
        if (function_count >= MAX_FUNCTIONS) {
            free_ast(copy);
            printf("function error: maximum function count reached\n");
            return failure_value();
        }
        snprintf(functions[function_count].name,
                 sizeof(functions[function_count].name), "%s", node->value);
        functions[function_count++].definition = copy;
        return invalid_value();
    }
    if (strcmp(node->type, "call") == 0) {
        return evaluate_call(node);
    }
    if (strcmp(node->type, "return") == 0) {
        variable_value result = evaluation(node->right);
        if (result.failed) {
            return result;
        }
        result.returned = true;
        return result;
    }
    if (strcmp(node->type, "break") == 0) {
        variable_value result = invalid_value();
        result.break_loop = true;
        return result;
    }
    if (strcmp(node->type, "if") == 0) {
        variable_value condition = evaluation(node->left);
        bool truth;
        if (!truth_value(condition, &truth)) {
            return condition.failed ? condition : failure_value();
        }
        astnode *then_body = node->right;
        astnode *else_body = NULL;
        if (then_body != NULL && strcmp(then_body->type, "branches") == 0) {
            else_body = then_body->right;
            then_body = then_body->left;
        }
        return truth ? evaluate_scoped(then_body) : evaluate_scoped(else_body);
    }
    if (strcmp(node->type, "while") == 0) {
        variable_value result = invalid_value();
        for (;;) {
            variable_value condition = evaluation(node->left);
            bool truth;
            if (!truth_value(condition, &truth)) {
                return condition.failed ? condition : failure_value();
            }
            if (!truth) {
                return result;
            }
            result = evaluate_scoped(node->right);
            if (result.returned) {
                return result;
            }
            if (result.failed) {
                return result;
            }
            if (result.break_loop) {
                result.break_loop = false;
                return result;
            }
        }
    }
    if (strcmp(node->type, "unary") == 0) {
        variable_value operand = evaluation(node->right);
        if (!operand.valid) {
            return operand.failed ? operand : failure_value();
        }
        if (strcmp(node->value, "-") == 0 && strcmp(operand.type, "int") == 0) {
            return checked_integer(-(long long)operand.integer);
        }
        if (strcmp(node->value, "!") == 0 && strcmp(operand.type, "bool") == 0) {
            return boolean_value(!operand.integer);
        }
        printf("type error: invalid operand for unary '%s'\n", node->value);
        return failure_value();
    }
    if (strcmp(node->type, "operator") == 0) {
        variable_value left = evaluation(node->left);
        if (!left.valid) {
            return left.failed ? left : failure_value();
        }
        if (strcmp(node->value, "&&") == 0 || strcmp(node->value, "||") == 0) {
            bool left_truth;
            if (!truth_value(left, &left_truth)) {
                return left.failed ? left : failure_value();
            }
            if ((strcmp(node->value, "&&") == 0 && !left_truth) ||
                (strcmp(node->value, "||") == 0 && left_truth)) {
                return boolean_value(left_truth);
            }
            variable_value right = evaluation(node->right);
            bool right_truth;
            if (!truth_value(right, &right_truth)) {
                return right.failed ? right : failure_value();
            }
            return boolean_value(right_truth);
        }

        variable_value right = evaluation(node->right);
        if (!right.valid) {
            return right.failed ? right : failure_value();
        }
        if (strcmp(node->value, "==") == 0 || strcmp(node->value, "!=") == 0) {
            bool equal = false;
            if (strcmp(left.type, right.type) == 0) {
                if (strcmp(left.type, "str") == 0) {
                    equal = strcmp(left.string, right.string) == 0;
                } else {
                    equal = left.integer == right.integer;
                }
            }
            return boolean_value(strcmp(node->value, "==") == 0 ? equal : !equal);
        }
        if (strcmp(left.type, "int") != 0 || strcmp(right.type, "int") != 0) {
            printf("type error: operator '%s' requires int values\n", node->value);
            return failure_value();
        }
        if (strcmp(node->value, "+") == 0) {
            return checked_integer((long long)left.integer + right.integer);
        }
        if (strcmp(node->value, "-") == 0) {
            return checked_integer((long long)left.integer - right.integer);
        }
        if (strcmp(node->value, "*") == 0) {
            return checked_integer((long long)left.integer * right.integer);
        }
        if (strcmp(node->value, "/") == 0) {
            if (right.integer == 0) {
                printf("evaluation error: division by zero\n");
                return failure_value();
            }
            if (left.integer == INT_MIN && right.integer == -1) {
                printf("evaluation error: integer arithmetic overflow\n");
                return failure_value();
            }
            return integer_value(left.integer / right.integer);
        }
        if (strcmp(node->value, "<") == 0) {
            return boolean_value(left.integer < right.integer);
        }
        if (strcmp(node->value, ">") == 0) {
            return boolean_value(left.integer > right.integer);
        }
        if (strcmp(node->value, "<=") == 0) {
            return boolean_value(left.integer <= right.integer);
        }
        if (strcmp(node->value, ">=") == 0) {
            return boolean_value(left.integer >= right.integer);
        }
    }
    printf("evaluation error: unknown node '%s'\n", node->type);
    return failure_value();
}
