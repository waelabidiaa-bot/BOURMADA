#ifndef VAR_H
#define VAR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_VARS 200
#define MAX_SCOPE_DEPTH 128

typedef struct {
    char type[20];
    int integer;
    char string[25];
    bool valid;
    bool returned;
    bool break_loop;
    bool failed;
} variable_value;

typedef struct {
    char name[25];
    variable_value value;
    int scope;
} variable;

extern variable vars[MAX_VARS];
extern int var_count;

bool enter_scope(int source_line);
bool enter_function_scope(int source_line);
void leave_scope(void);
bool declare_var(const char *name, variable_value value, int source_line);
bool set_var(const char *name, variable_value value, int source_line);
bool get_var(const char *name, variable_value *value, int source_line);

#endif