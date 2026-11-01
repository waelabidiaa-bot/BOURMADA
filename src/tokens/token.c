#include "token.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define MAX_LENGTH 100
#define MAX_TOKENS 100




bool add_token(Token **head,Token **tail,const char *value,const char *type,int number){
    Token *new_token = malloc(sizeof(Token));

    if(new_token==NULL){
        return false;
    }
    snprintf(new_token->value,sizeof(new_token->value),"%s",value);

    snprintf(new_token->type,sizeof(new_token->type),"%s",type);

    new_token->number = number;
    new_token->next = NULL;
    if(*head == NULL){
        *head = new_token;
        *tail =new_token;

    }else{
        (*tail)->next = new_token;
        *tail = new_token;
    }
    return true;


}

bool match(Token **token,const char *expected_value){
    if(*token==NULL){
        return false;
    }
    if(strcmp((*token)->value,expected_value)==0){
        *token = (*token)->next;
        return true;

    }
    return false;
}


Token *peek(Token *token){
    if ( token ==NULL|| token->next==NULL){
        printf("nothing to peek on :)");
        return NULL;

    }
    else {
        return token->next;
    }
}


//moves from token to the next in one direction from head to tail
Token *advance(Token *token){
    return token == NULL ? NULL : token->next;
}
void free_tokens(Token *token) {
    while (token != NULL) {
        Token *next = token->next;
        free(token);
        token = next;
    }
}
