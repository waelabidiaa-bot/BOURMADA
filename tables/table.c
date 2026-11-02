#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "row.h"

typedef struct {
    char name[25];
    char type[20];
} table_column;

typedef struct {
    bool used;
    char name[25];
    int column_count;
    int row_count;
    int row_capacity;
    table_column columns[MAX_TABLE_COLUMNS];
    table_row *rows;
} table_entry;

static table_entry tables[MAX_TABLES];
static int table_count;

static void table_error(int source_line, const char *message) {
    printf("table error: %s", message);
    if (source_line > 0) {
        printf(" (line %d)", source_line);
    }
    printf("\n");
}

static table_entry *find_table(const char *name) {
    for (int i = 0; i < table_count; i++) {
        if (tables[i].used && strcmp(tables[i].name, name) == 0) {
            return &tables[i];
        }
    }
    return NULL;
}

static int find_column(const table_entry *table, const char *name) {
    for (int i = 0; i < table->column_count; i++) {
        if (strcmp(table->columns[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

bool table_create(const char *name, const char **column_names,
                  const char **column_types, int column_count, int source_line) {
    if (find_table(name) != NULL) {
        table_error(source_line, "that table already exists.");
        return false;
    }
    if (table_count >= MAX_TABLES) {
        table_error(source_line, "the table limit has been reached.");
        return false;
    }
    if (column_count <= 0 || column_count > MAX_TABLE_COLUMNS) {
        table_error(source_line, "a table needs between 1 and 32 columns.");
        return false;
    }

    table_entry *table = &tables[table_count];
    memset(table, 0, sizeof(*table));
    snprintf(table->name, sizeof(table->name), "%s", name);
    table->column_count = column_count;
    table->used = true;
    for (int i = 0; i < column_count; i++) {
        if (strcmp(column_types[i], "int") != 0 &&
            strcmp(column_types[i], "str") != 0 &&
            strcmp(column_types[i], "bool") != 0) {
            table_error(source_line, "columns can only use int, str, or bool.");
            return false;
        }
        for (int j = 0; j < i; j++) {
            if (strcmp(column_names[i], column_names[j]) == 0) {
                table_error(source_line, "column names must be unique.");
                return false;
            }
        }
        snprintf(table->columns[i].name, sizeof(table->columns[i].name), "%s",
                 column_names[i]);
        snprintf(table->columns[i].type, sizeof(table->columns[i].type), "%s",
                 column_types[i]);
    }

    table_count++;
    return true;
}

static bool validate_fields(const table_entry *table, const char **field_names,
                            const variable_value *values, int field_count,
                            bool require_all, int source_line,
                            variable_value *ordered_values) {
    if (field_count <= 0 || field_count > table->column_count) {
        table_error(source_line, "the supplied fields do not match this table.");
        return false;
    }

    bool supplied[MAX_TABLE_COLUMNS] = {false};
    for (int i = 0; i < field_count; i++) {
        int column = find_column(table, field_names[i]);
        if (column < 0) {
            table_error(source_line, "a supplied field is not in this table's schema.");
            return false;
        }
        if (supplied[column]) {
            table_error(source_line, "a field was supplied more than once.");
            return false;
        }
        if (strcmp(values[i].type, table->columns[column].type) != 0) {
            table_error(source_line, "a field value has the wrong type.");
            return false;
        }
        supplied[column] = true;
        if (ordered_values != NULL) {
            ordered_values[column] = values[i];
        }
    }

    if (require_all) {
        for (int i = 0; i < table->column_count; i++) {
            if (!supplied[i]) {
                table_error(source_line, "an insert must provide a value for every column.");
                return false;
            }
        }
    }
    return true;
}

bool table_insert(const char *name, const char **field_names,
                  const variable_value *values, int field_count, int source_line) {
    table_entry *table = find_table(name);
    if (table == NULL) {
        table_error(source_line, "that table has not been declared.");
        return false;
    }
    if (table->row_count >= MAX_TABLE_ROWS) {
        table_error(source_line, "the table is full (maximum 256 rows).");
        return false;
    }
    variable_value ordered_values[MAX_TABLE_COLUMNS] = {0};
    if (!validate_fields(table, field_names, values, field_count, true,
                         source_line, ordered_values)) {
        return false;
    }
    if (table->row_count == table->row_capacity) {
        int new_capacity = table->row_capacity == 0 ? 8 : table->row_capacity * 2;
        if (new_capacity > MAX_TABLE_ROWS) {
            new_capacity = MAX_TABLE_ROWS;
        }
        table_row *rows = realloc(table->rows, (size_t)new_capacity * sizeof(*rows));
        if (rows == NULL) {
            table_error(source_line, "not enough memory to add another row.");
            return false;
        }
        table->rows = rows;
        table->row_capacity = new_capacity;
    }
    for (int i = 0; i < table->column_count; i++) {
        table->rows[table->row_count].values[i] = ordered_values[i];
    }
    table->row_count++;
    return true;
}

bool table_delete(const char *name, int row_index, int source_line) {
    table_entry *table = find_table(name);
    if (table == NULL) {
        table_error(source_line, "that table has not been declared.");
        return false;
    }
    if (row_index < 0 || row_index >= table->row_count) {
        table_error(source_line, "that row index is out of range.");
        return false;
    }
    for (int i = row_index; i < table->row_count - 1; i++) {
        table->rows[i] = table->rows[i + 1];
    }
    table->row_count--;
    memset(&table->rows[table->row_count], 0, sizeof(table->rows[table->row_count]));
    return true;
}

bool table_update(const char *name, int row_index, const char **field_names,
                  const variable_value *values, int field_count, int source_line) {
    table_entry *table = find_table(name);
    if (table == NULL) {
        table_error(source_line, "that table has not been declared.");
        return false;
    }
    if (row_index < 0 || row_index >= table->row_count) {
        table_error(source_line, "that row index is out of range.");
        return false;
    }
    variable_value ordered_values[MAX_TABLE_COLUMNS] = {0};
    if (!validate_fields(table, field_names, values, field_count, false,
                         source_line, ordered_values)) {
        return false;
    }
    for (int i = 0; i < field_count; i++) {
        int column = find_column(table, field_names[i]);
        table->rows[row_index].values[column] = ordered_values[column];
    }
    return true;
}

bool table_get(const char *name, int row_index, const char *column_name,
               variable_value *value, int source_line) {
    table_entry *table = find_table(name);
    if (table == NULL) {
        table_error(source_line, "that table has not been declared.");
        return false;
    }
    if (row_index < 0 || row_index >= table->row_count) {
        table_error(source_line, "that row index is out of range.");
        return false;
    }
    int column = find_column(table, column_name);
    if (column < 0) {
        table_error(source_line, "that column is not in this table's schema.");
        return false;
    }
    *value = table->rows[row_index].values[column];
    return true;
}

void table_cleanup(void) {
    for (int i = 0; i < table_count; i++) {
        free(tables[i].rows);
        memset(&tables[i], 0, sizeof(tables[i]));
    }
    table_count = 0;
}