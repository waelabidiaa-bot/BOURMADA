#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>  
#include <stdbool.h>

#include "lex.h"



bool is_math_symbole( char c){
    return strchr("+-*/=<>!&|", c) != NULL;
}













Token *lex_input(char *s) {
    Token *head = NULL;
    Token *tail = NULL;

    int i = 0;
    int token_count = 0;

    while (s[i] != '\0') {

        if (s[i] == '/' && s[i + 1] == '/') {
            while (s[i] != '\0' && s[i] != '\n') {
                i++;
            }
            continue;
        }

        if (isspace((unsigned char)s[i])) {
            i++;
            continue;
        }

        char value[MAX_LENGTH];
        int j = 0;

        if (s[i] == '"') {
            i++;
            while (s[i] != '\0' && s[i] != '"') {
                if (j >= (int)sizeof(((Token *)0)->value) - 1) {
                    printf("lexer error: string literal is too long\n");
                    free_tokens(head);
                    return NULL;
                }
                value[j++] = s[i++];
            }
            if (s[i] != '"') {
                printf("lexer error: unterminated string literal\n");
                free_tokens(head);
                return NULL;
            }
            value[j] = '\0';
            i++;

            if (!add_token(&head, &tail, value, "string", token_count++)) {
                printf("error: tokenisation failed\n");
                free_tokens(head);
                return NULL;
            }
        }

        // 1. Numbers
        else if (isdigit((unsigned char)s[i])) {

            while (isdigit((unsigned char)s[i])) {
                if (j >= (int)sizeof(((Token *)0)->value) - 1) {
                    printf("lexer error: number is too long\n");
                    free_tokens(head);
                    return NULL;
                }
                value[j++] = s[i++];
            }

            value[j] = '\0';

            if (!add_token(&head, &tail, value, "number", token_count++)) {
                printf("error: tokenisation failed\n");
                free_tokens(head);
                return NULL;
            }
        }

        // 2. Identifiers
        else if (isalpha((unsigned char)s[i]) || s[i] == '_') {

            while (isalnum((unsigned char)s[i]) || s[i] == '_') {
                if (j >= (int)sizeof(((Token *)0)->value) - 1) {
                    printf("lexer error: identifier is too long\n");
                    free_tokens(head);
                    return NULL;
                }
                value[j++] = s[i++];
            }

            value[j] = '\0';

            if (!add_token(&head, &tail, value, "identifier", token_count++)) {
                printf("error: tokenisation failed\n");
                free_tokens(head);
                return NULL;
            }
        }

        // 3. Mathematical operators
        else if (is_math_symbole(s[i])) {

            value[0] = s[i];
            value[1] = '\0';
            if ((s[i] == '=' || s[i] == '!' || s[i] == '<' || s[i] == '>') &&
                s[i + 1] == '=') {
                value[1] = s[i + 1];
                value[2] = '\0';
                i++;
            } else if ((s[i] == '&' || s[i] == '|') && s[i + 1] == s[i]) {
                value[1] = s[i + 1];
                value[2] = '\0';
                i++;
            }
            i++;

            if (!add_token(&head, &tail, value, "operator", token_count++)) {
                printf("error: tokenisation failed\n");
                free_tokens(head);
                return NULL;
            }
        }

        // 4. Other punctuation: (, ), etc.
        else if (ispunct((unsigned char)s[i])) {

            value[0] = s[i];
            value[1] = '\0';
            i++;

            if (!add_token(&head, &tail, value, "punct", token_count++)) {
                printf("error: tokenisation failed\n");
                free_tokens(head);
                return NULL;
            }
        }

        // 5. Invalid characters
        else {
            printf("lexer error: unexpected character '%c'\n", s[i]);
            free_tokens(head);
            return NULL;
        }

        if (token_count >= MAX_TOKENS) {
            int remaining = i;
            while (isspace((unsigned char)s[remaining])) {
                remaining++;
            }
            if (s[remaining] != '\0') {
                printf("lexer error: maximum token count reached\n");
                free_tokens(head);
                return NULL;
            }
        }
    }

    return head;
}
