
#include <stdbool.h>


#ifndef TOKEN_H
#define TOKEN_H

typedef struct Token {
    int number;
    int line;
    char type[20];
    char value[25];
    struct Token* next; 
} Token;




bool add_token(Token **head,Token **tail,const char *value,const char *type,
               int number, int line);
bool match(Token **token,const char *expected_value);
Token *peek(Token *token);
Token *advance(Token *token);
void free_tokens(Token *token);



#endif