// Based on csv_reader.c, but memory safe (acc. to Claude & valgrind)
//
// <legal>
// Pointer Ownership Model (POM) Source Code Release
// 
// Copyright 2025 Carnegie Mellon University.
// 
// NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
// INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
// UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
// IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
// FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
// OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
// MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
// TRADEMARK, OR COPYRIGHT INFRINGEMENT.
// 
// Licensed under a MIT (SEI)-style license, please see license.txt or
// contact permission@sei.cmu.edu for full terms.
// 
// [DISTRIBUTION STATEMENT A] This material has been approved for public
// release and unlimited distribution.  Please see Copyright notice for
// non-US Government use and distribution.
// 
// DM25-1262
// </legal>
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FIELDS 20
#define MAX_FIELD_LENGTH 100

typedef struct {
    char **fields;
    int num_fields;
} csv_row;

typedef struct {
    csv_row *rows;
    int num_rows;
} csv_table;

void free_row(csv_row *row) {
    if (row) {
        for (int i = 0; i < row->num_fields; i++) {
            free(row->fields[i]);
        }
        free(row->fields);
        // free(row);
    }
}

void free_table(csv_table *table) {
    if (table) {
        for (int i = 0; i < table->num_rows; i++) {
            free_row(&table->rows[i]);
        }
        free(table->rows);
        free(table);
    }
}

csv_row *parse_csv_row(char *row_str) {
    csv_row *row = (csv_row *) malloc(sizeof(csv_row));
    if (!row) {
        printf("ERROR: Out of memory.\n");
        return NULL;
    }

    row->fields = (char **) malloc(sizeof(char *) * MAX_FIELDS);
    if (!row->fields) {
        printf("ERROR: Out of memory.\n");
        free_row(row);  free(row); // must do this here
        return NULL;
    }

    char *field_str = strtok(row_str, ",");
    int field_num = 0;
    while (field_str && field_num < MAX_FIELDS) {
        row->fields[field_num] = (char *) malloc(sizeof(char) * MAX_FIELD_LENGTH);
        if (!row->fields[field_num]) {
            printf("ERROR: Out of memory.\n");
            free_row(row);   free(row); // must do this here
            return NULL;
        }

        strncpy(row->fields[field_num], field_str, MAX_FIELD_LENGTH - 1);
        row->fields[field_num][MAX_FIELD_LENGTH - 1] = '\0';
        field_str = strtok(NULL, ",");
        field_num++;
    }

    row->num_fields = field_num;
    return row;
}

csv_table *parse_csv_file(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("ERROR: Unable to open file '%s'.\n", filename);
        return NULL;
    }

    csv_table *table = (csv_table *) calloc(sizeof(csv_table), 1);
    if (!table) {
        printf("ERROR: Out of memory.\n");
        fclose(file);
        return NULL;
    }

    char row_str[MAX_FIELD_LENGTH * MAX_FIELDS];
    int row_num = 0;
    while (fgets(row_str, sizeof(row_str), file)) {
        csv_row *row = parse_csv_row(row_str);
        if (!row) {
            printf("ERROR: Failed to parse row %d.\n", row_num);
            free_table(table);
            fclose(file);
            return NULL;
        }

        csv_row *temp_rows = (csv_row *) realloc(table->rows, sizeof(csv_row) * (row_num + 1));
        if (!temp_rows) {
            printf("ERROR: Out of memory.\n");
            free_table(table);
            free_row(row);   free(row); // must do this here
            fclose(file);
            return NULL;
        }
        table->rows = temp_rows;
        table->rows[row_num] = *row;
        free(row);
        row_num++;
    }

    table->num_rows = row_num;
    fclose(file);
    return table;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("USAGE: %s <filename>\n", argv[0]);
        return 1;
    }

    csv_table *table = parse_csv_file(argv[1]);
    if (!table) {
        printf("ERROR: Failed to parse CSV file '%s'.\n", argv[1]);
        return 1;
    }

    printf("Parsed CSV file '%s' with %d rows:\n", argv[1], table->num_rows);
    for (int i = 0; i < table->num_rows; i++) {
        csv_row row = table->rows[i];
        for (int j = 0; j < row.num_fields; j++) {
            printf("%s", row.fields[j]);
            if (j < row.num_fields - 1) {
                printf(", ");
            }
        }
        printf("\n");
    }

    free_table(table);
    return 0;
}
