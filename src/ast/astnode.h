#include <stdbool.h>
#include "../tokens/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#ifndef ASTNODE_H
#define ASTNODE_H

typedef struct astnode{
    char type[20];
    char value[25];
    int line;

    struct astnode *left;
    struct astnode *right;

}astnode;

astnode *create_node(const char *type,const char *value);

void free_ast(astnode *node);

void print_ast(astnode *node);


#endif