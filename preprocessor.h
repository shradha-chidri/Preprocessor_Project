#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#define MAX_MACROS 100
#define MAX_NAME 100
#define MAX_VALUE 500

typedef struct
{
    char name[MAX_NAME];
    char value[MAX_VALUE];
    int is_function;
    char parameter[MAX_NAME];
} Macro;

int remove_comments(const char *input_file, const char *output_file);
int process_includes(const char *input_file, const char *output_file);
int process_macros(const char *input_file, const char *output_file);

#endif
