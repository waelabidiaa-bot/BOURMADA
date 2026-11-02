
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "astnode.h"



astnode *create_node(const char *type,const char *value){
    astnode *node = malloc(sizeof(astnode));

    if (node==NULL){
        return NULL;
    }

    snprintf(node->type,sizeof(node->type),"%s",type);
    snprintf(node->value,sizeof(node->value),"%s",value);
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
