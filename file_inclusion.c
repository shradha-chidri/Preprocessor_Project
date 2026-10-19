#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"

static int copy_file(FILE *in, FILE *out)
{
    int c;

    while ((c = fgetc(in)) != EOF)
        fputc(c, out);

    return ferror(in) ? 1 : 0;
}

int process_includes(const char *input_file, const char *output_file)
{
    FILE *in = fopen(input_file, "r");
    FILE *out = fopen(output_file, "w");
    char line[2048];

    if (in == NULL || out == NULL)
    {
        perror("Error opening file");
        if (in != NULL)
            fclose(in);
        if (out != NULL)
            fclose(out);
        return 1;
    }

    while (fgets(line, sizeof(line), in) != NULL)
    {
        char *p = line;

        while (isspace((unsigned char)*p))
            p++;

        if (strncmp(p, "#include", 8) == 0 &&
            isspace((unsigned char)p[8]))
        {
            char *start;
            char *end;
            char filename[512];
            FILE *included;

            p += 8;

            while (isspace((unsigned char)*p))
                p++;

            if (*p != '"' && *p != '<')
            {
                fprintf(stderr, "Error: invalid include directive.\n");
                fclose(in);
                fclose(out);
                return 1;
            }

            char closing = (*p == '"') ? '"' : '>';
            start = p + 1;
            end = strchr(start, closing);

            if (end == NULL)
            {
                fprintf(stderr, "Error: invalid include filename.\n");
                fclose(in);
                fclose(out);
                return 1;
            }

            size_t length = (size_t)(end - start);

            if (length == 0 || length >= sizeof(filename))
            {
                fprintf(stderr, "Error: include filename is too long.\n");
                fclose(in);
                fclose(out);
                return 1;
            }

            memcpy(filename, start, length);
            filename[length] = '\0';

            included = fopen(filename, "r");

            if (included == NULL && *p == '"')
            {
                char *slash = strrchr(input_file, '/');

                if (slash != NULL)
                {
                    char path[1024];
                    size_t directory_length =
                        (size_t)(slash - input_file + 1);

                    if (directory_length + length < sizeof(path))
                    {
                        memcpy(path, input_file, directory_length);
                        memcpy(path + directory_length, filename,
                               length + 1);
                        included = fopen(path, "r");
                    }
                }
            }

            if (included == NULL)
            {
                fprintf(stderr, "Error: cannot open included file: %s\n",
                        filename);
                fclose(in);
                fclose(out);
                return 1;
            }

            if (copy_file(included, out) != 0)
            {
                fprintf(stderr, "Error reading included file.\n");
                fclose(included);
                fclose(in);
                fclose(out);
                return 1;
            }

            fclose(included);

            if (line[strlen(line) - 1] != '\n')
                fputc('\n', out);
        }
        else
        {
            fputs(line, out);
        }
    }

    if (ferror(in))
    {
        fprintf(stderr, "Error reading input file.\n");
        fclose(in);
        fclose(out);
        return 1;
    }

    fclose(in);
    fclose(out);
    return 0;
}
