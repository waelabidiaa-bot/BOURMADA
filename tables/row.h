#ifndef ROW_H
#define ROW_H

#include "table.h"

typedef struct {
    variable_value values[MAX_TABLE_COLUMNS];
} table_row;

#endif