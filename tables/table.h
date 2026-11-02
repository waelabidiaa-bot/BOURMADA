#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>

#include "../src/variables/var.h"

#define MAX_TABLES 32
#define MAX_TABLE_ROWS 256
#define MAX_TABLE_COLUMNS 32

bool table_create(const char *name, const char **column_names,
                  const char **column_types, int column_count, int source_line);
bool table_insert(const char *name, const char **field_names,
                  const variable_value *values, int field_count, int source_line);
bool table_delete(const char *name, int row_index, int source_line);
bool table_update(const char *name, int row_index, const char **field_names,
                  const variable_value *values, int field_count, int source_line);
bool table_get(const char *name, int row_index, const char *column_name,
               variable_value *value, int source_line);
void table_cleanup(void);

#endif