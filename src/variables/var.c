#include <stdio.h>
#include <string.h>

#include "var.h"

variable vars[MAX_VARS];
int var_count = 0;
typedef struct {
    int id;
    int parent_id;
    int return_id;
} scope_frame;

static scope_frame scope_stack[MAX_SCOPE_DEPTH];
static int scope_depth;
static int current_scope;
static int next_scope_id = 1;

static bool push_scope(int parent_id) {
    if (scope_depth >= MAX_SCOPE_DEPTH || next_scope_id == 0) {
        printf("scope error: maximum scope depth reached\n");
        return false;
    }

    scope_frame *frame = &scope_stack[scope_depth++];
    frame->id = next_scope_id++;
    frame->parent_id = parent_id;
    frame->return_id = current_scope;
    current_scope = frame->id;
    return true;
}

bool enter_scope(void) {
    return push_scope(current_scope);
}

bool enter_function_scope(void) {
    return push_scope(0);
}

void leave_scope(void) {
    if (scope_depth == 0) {
        return;
    }

    scope_frame frame = scope_stack[--scope_depth];
    while (var_count > 0 && vars[var_count - 1].scope == frame.id) {
        var_count--;
    }
    current_scope = frame.return_id;
}

static bool scope_is_visible(int target_scope) {
    int scope = current_scope;
    while (scope != target_scope && scope != 0) {
        bool found = false;
        for (int i = scope_depth - 1; i >= 0; i--) {
            if (scope_stack[i].id == scope) {
                scope = scope_stack[i].parent_id;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return scope == target_scope;
}

bool declare_var(const char *name, variable_value value) {
    for (int i = var_count - 1; i >= 0; i--) {
        if (vars[i].scope != current_scope) {
            continue;
        }
        if (strcmp(vars[i].name, name) == 0) {
            printf("variable error: '%s' is already declared in this scope\n", name);
            return false;
        }
    }

    if (var_count >= MAX_VARS) {
        printf("Error: max variables reached\n");
        return false;
    }

    snprintf(vars[var_count].name, sizeof(vars[var_count].name), "%s", name);
    vars[var_count].value = value;
    vars[var_count].scope = current_scope;
    var_count++;
    return true;
}

bool set_var(const char *name, variable_value value) {
    for (int i = var_count - 1; i >= 0; i--) {
        if (scope_is_visible(vars[i].scope) && strcmp(vars[i].name, name) == 0) {
            if (strcmp(vars[i].value.type, value.type) != 0) {
                printf("variable error: cannot assign %s to %s variable '%s'\n",
                       value.type, vars[i].value.type, name);
                return false;
            }
            vars[i].value = value;
            return true;
        }
    }
    printf("variable error: '%s' is not defined\n", name);
    return false;
}

bool get_var(const char *name, variable_value *value) {
    for (int i = var_count - 1; i >= 0; i--) {
        if (scope_is_visible(vars[i].scope) && strcmp(vars[i].name, name) == 0) {
            *value = vars[i].value;
            return true;
        }
    }

    printf("variable error: '%s' is not defined\n", name);
    return false;
}
