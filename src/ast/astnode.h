#include <stdbool.h>
#include "../tokens/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#ifndef ASTNODE_H
#define ASTNODE_H

typedef enum {
    AST_NODE_OTHER,
    AST_NODE_NUMBER,
    AST_NODE_STRING,
    AST_NODE_TABLE_DECLARATION,
    AST_NODE_TABLE_INSERT,
    AST_NODE_TABLE_DELETE,
    AST_NODE_TABLE_UPDATE,
    AST_NODE_TABLE_ACCESS,
    AST_NODE_BOOLEAN,
    AST_NODE_IDENTIFIER,
    AST_NODE_VAR,
    AST_NODE_LET,
    AST_NODE_ASSIGN,
    AST_NODE_PRINT,
    AST_NODE_SEQUENCE,
    AST_NODE_FUNCTION,
    AST_NODE_CALL,
    AST_NODE_RETURN,
    AST_NODE_BREAK,
    AST_NODE_IF,
    AST_NODE_BRANCHES,
    AST_NODE_WHILE,
    AST_NODE_FOR,
    AST_NODE_UNARY,
    AST_NODE_OPERATOR
} astnode_kind;

typedef struct astnode{
    char type[20];
    char value[25];
    astnode_kind kind;
    int line;

    struct astnode *left;
    struct astnode *right;

}astnode;

astnode *create_node(const char *type,const char *value);

void free_ast(astnode *node);

void print_ast(astnode *node);


#endif