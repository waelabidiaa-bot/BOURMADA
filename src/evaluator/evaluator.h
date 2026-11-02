#include <stdbool.h>
#include "../ast/astnode.h"
#include "../variables/var.h"

#ifndef EVALUATOR_H
#define EVALUATOR_H

variable_value evaluation(astnode *node);
void print_value(variable_value value);
void evaluator_cleanup(void);


#endif