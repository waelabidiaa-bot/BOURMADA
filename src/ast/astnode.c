
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "astnode.h"

static astnode_kind classify_type(const char *type) {
    static const struct {
        const char *name;
        astnode_kind kind;
    } types[] = {
        {"number", AST_NODE_NUMBER},
        {"string", AST_NODE_STRING},
        {"table_declaration", AST_NODE_TABLE_DECLARATION},
        {"table_insert", AST_NODE_TABLE_INSERT},
        {"table_delete", AST_NODE_TABLE_DELETE},
        {"table_update", AST_NODE_TABLE_UPDATE},
        {"table_access", AST_NODE_TABLE_ACCESS},
        {"boolean", AST_NODE_BOOLEAN},
        {"identifier", AST_NODE_IDENTIFIER},
        {"var", AST_NODE_VAR},
        {"let", AST_NODE_LET},
        {"assign", AST_NODE_ASSIGN},
        {"print", AST_NODE_PRINT},
        {"sequence", AST_NODE_SEQUENCE},
        {"function", AST_NODE_FUNCTION},
        {"call", AST_NODE_CALL},
        {"return", AST_NODE_RETURN},
        {"break", AST_NODE_BREAK},
        {"if", AST_NODE_IF},
        {"branches", AST_NODE_BRANCHES},
        {"while", AST_NODE_WHILE},
        {"for", AST_NODE_FOR},
        {"unary", AST_NODE_UNARY},
        {"operator", AST_NODE_OPERATOR}
    };

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        if (strcmp(type, types[i].name) == 0) {
            return types[i].kind;
        }
    }
    return AST_NODE_OTHER;
}

astnode *create_node(const char *type,const char *value){
    astnode *node = malloc(sizeof(astnode));

    if (node==NULL){
        return NULL;
    }

    snprintf(node->type,sizeof(node->type),"%s",type);
    snprintf(node->value,sizeof(node->value),"%s",value);
    node->kind = classify_type(node->type);
    node->line = 0;

    node->left= NULL;
    node->right=NULL;

    return node;
}

void free_ast(astnode *node) {
    if (node == NULL) {
        return;
    }

    free_ast(node->left);
    free_ast(node->right);
    free(node);
}
void print_ast(astnode *node) {
    if (node == NULL) {
        return;
    }

    printf("Value: %s | Type: %s\n", node->value, node->type);

    print_ast(node->left);
    print_ast(node->right);
}
