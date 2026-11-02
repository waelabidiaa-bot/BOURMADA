#include <stdio.h>
#include <string.h>

#include "var.h"

#define VARIABLE_HASH_CAPACITY 521

typedef struct {
    int variable_index;
    bool occupied;
} variable_hash_entry;

variable vars[MAX_VARS];
int var_count = 0;
static variable_hash_entry variable_hash[VARIABLE_HASH_CAPACITY];

typedef struct {
    int id;
    int parent_id;
    int return_id;
} scope_frame;

static scope_frame scope_stack[MAX_SCOPE_DEPTH];
static int scope_depth;
static int current_scope;
static int next_scope_id = 1;

static unsigned long hash_variable_key(const char *name, int scope) {
    unsigned long hash = 5381;
    for (const unsigned char *character = (const unsigned char *)name;
         *character != '\0'; character++) {
        hash = hash * 33 + *character;
    }
    return (hash * 33 + (unsigned int)scope) % VARIABLE_HASH_CAPACITY;
}

static int find_variable_slot(const char *name, int scope, bool *found) {
    unsigned long index = hash_variable_key(name, scope);
    for (int probes = 0; probes < VARIABLE_HASH_CAPACITY; probes++) {
        variable_hash_entry *entry = &variable_hash[index];
        if (!entry->occupied) {
            *found = false;
            return (int)index;
        }
        variable *candidate = &vars[entry->variable_index];
        if (candidate->scope == scope && strcmp(candidate->name, name) == 0) {
            *found = true;
            return (int)index;
        }
        index = (index + 1) % VARIABLE_HASH_CAPACITY;
    }
    *found = false;
    return -1;
}

static int find_variable(const char *name, int scope) {
    while (scope >= 0) {
        bool found;
        int slot = find_variable_slot(name, scope, &found);
        if (found) {
            return variable_hash[slot].variable_index;
        }
        if (scope == 0) {
            break;
        }
        int parent_scope = -1;
        for (int i = scope_depth - 1; i >= 0; i--) {
            if (scope_stack[i].id == scope) {
                parent_scope = scope_stack[i].parent_id;
                break;
            }
        }
        scope = parent_scope;
    }
    return -1;
}

static bool index_variable(int variable_index) {
    bool found;
    int slot = find_variable_slot(vars[variable_index].name,
                                  vars[variable_index].scope, &found);
    if (slot < 0 || found) {
        return false;
    }
    variable_hash[slot].variable_index = variable_index;
    variable_hash[slot].occupied = true;
    return true;
}

static bool rebuild_variable_hash(void) {
    memset(variable_hash, 0, sizeof(variable_hash));
    for (int i = 0; i < var_count; i++) {
        if (!index_variable(i)) {
            return false;
        }
    }
    return true;
}

static void report_error(int source_line, const char *problem,
                         const char *suggestion) {
    printf("Oops! %s\n", problem);
    if (source_line > 0) {
        printf("  At line %d.\n", source_line);
    } else {
        printf("  (I couldn't pin down the source line.)\n");
    }
    printf("  Try this: %s\n", suggestion);
}

static bool push_scope(int parent_id, int source_line, const char *suggestion) {
    if (scope_depth >= MAX_SCOPE_DEPTH || next_scope_id == 0) {
        char problem[128];
        snprintf(problem, sizeof(problem),
                 "We've hit the maximum nesting depth (%d scopes).", MAX_SCOPE_DEPTH);
        report_error(source_line, problem, suggestion);
        return false;
    }

    scope_frame *frame = &scope_stack[scope_depth++];
    frame->id = next_scope_id++;
    frame->parent_id = parent_id;
    frame->return_id = current_scope;
    current_scope = frame->id;
    return true;
}

bool enter_scope(int source_line) {
    return push_scope(current_scope, source_line,
                      "move some nested blocks into a helper function.");
}

bool enter_function_scope(int source_line) {
    return push_scope(0, source_line,
                      "reduce recursive calls or simplify deeply nested function calls.");
}

void leave_scope(void) {
    if (scope_depth == 0) {
        return;
    }

    scope_frame frame = scope_stack[--scope_depth];
    while (var_count > 0 && vars[var_count - 1].scope == frame.id) {
        var_count--;
    }
    if (!rebuild_variable_hash()) {
        report_error(0, "I couldn't rebuild the variable lookup table after leaving a scope.",
                     "please simplify the current scope nesting and try again.");
    }
    current_scope = frame.return_id;
}

bool declare_var(const char *name, variable_value value, int source_line) {
    bool found;
    find_variable_slot(name, current_scope, &found);
    if (found) {
        char problem[160];
        char suggestion[160];
        snprintf(problem, sizeof(problem),
                 "'%s' already has a declaration in this scope.", name);
        snprintf(suggestion, sizeof(suggestion),
                 "reuse '%s' without 'let', or choose a different name.", name);
        report_error(source_line, problem, suggestion);
        return false;
    }

    if (var_count >= MAX_VARS) {
        char problem[128];
        snprintf(problem, sizeof(problem),
                 "The program has reached its limit of %d variables.", MAX_VARS);
        report_error(source_line, problem,
                     "remove unused variables or split this work into smaller scopes.");
        return false;
    }

    snprintf(vars[var_count].name, sizeof(vars[var_count].name), "%s", name);
    vars[var_count].value = value;
    vars[var_count].scope = current_scope;
    if (!index_variable(var_count)) {
        report_error(source_line, "The variable lookup table is full.",
                     "remove unused variables or simplify the current scope.");
        return false;
    }
    var_count++;
    return true;
}

bool set_var(const char *name, variable_value value, int source_line) {
    int index = find_variable(name, current_scope);
    if (index >= 0) {
        if (strcmp(vars[index].value.type, value.type) != 0) {
            char problem[200];
            snprintf(problem, sizeof(problem),
                     "Can't put a %s value into the %s variable '%s'.",
                     value.type, vars[index].value.type, name);
            report_error(source_line, problem,
                         "assign a value with the same type as this variable.");
            return false;
        }
        vars[index].value = value;
        return true;
    }
    char problem[160];
    snprintf(problem, sizeof(problem), "I can't find a variable named '%s'.", name);
    report_error(source_line, problem,
                 "check the spelling and capitalization, or declare it first with 'let'.");
    return false;
}

bool get_var(const char *name, variable_value *value, int source_line) {
    int index = find_variable(name, current_scope);
    if (index >= 0) {
        *value = vars[index].value;
        return true;
    }

    char problem[160];
    snprintf(problem, sizeof(problem), "I can't find a variable named '%s'.", name);
    report_error(source_line, problem,
                 "check the spelling and capitalization, or declare it first with 'let'.");
    return false;
}
