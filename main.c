#include <stdio.h>
#include <string.h>
#include "preprocessor.h"

int main(int argc, char *argv[])
{
    char temp1[512];
    char temp2[512];
    char output_file[512];

    if (argc != 2)
    {
        printf("Usage: ./mypreprocessor inputfile.c\n");
        return 1;
    }

    if (snprintf(output_file, sizeof(output_file), "%s", argv[1]) >=
        (int)sizeof(output_file))
    {
        printf("Error: input filename is too long.\n");
        return 1;
    }

    char *dot = strrchr(output_file, '.');

    if (dot != NULL && strcmp(dot, ".c") == 0)
        *dot = '\0';

    if (snprintf(output_file + strlen(output_file),
                 sizeof(output_file) - strlen(output_file),
                 ".i") >=
        (int)(sizeof(output_file) - strlen(output_file)))
    {
        printf("Error: output filename is too long.\n");
        return 1;
    }

    snprintf(temp1, sizeof(temp1), "%s", "temp_comments.c");
    snprintf(temp2, sizeof(temp2), "%s", "temp_includes.c");

    if (remove_comments(argv[1], temp1) != 0)
        return 1;

    if (process_includes(temp1, temp2) != 0)
        return 1;

    if (process_macros(temp2, output_file) != 0)
        return 1;

    remove(temp1);
    remove(temp2);

    printf("Preprocessing completed successfully.\n");
    printf("Output file: %s\n", output_file);

    return 0;
}
