#include <stdbool.h>
#include "../ast/astnode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PARSER_H
#define PARSER_H

astnode *parse_expression(Token **current);
astnode *parse_program(Token **current);


#endif