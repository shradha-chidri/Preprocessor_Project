#include <stdio.h>
#include <ctype.h>
#include "preprocessor.h"

int remove_comments(const char *input_file, const char *output_file)
{
    FILE *in = fopen(input_file, "r");
    FILE *out = fopen(output_file, "w");
    int c, next;
    int state = 0;

    if (in == NULL)
    {
        perror("Error opening input file");
        return 1;
    }

    if (out == NULL)
    {
        perror("Error opening output file");
        fclose(in);
        return 1;
    }

    while ((c = fgetc(in)) != EOF)
    {
        if (state == 0)
        {
            if (c == '"')
            {
                fputc(c, out);
                state = 3;
            }
            else if (c == '\'')
            {
                fputc(c, out);
                state = 4;
            }
            else if (c == '/')
            {
                next = fgetc(in);

                if (next == '/')
                {
                    state = 1;
                }
                else if (next == '*')
                {
                    state = 2;
                }
                else
                {
                    fputc(c, out);
                    if (next != EOF)
                        ungetc(next, in);
                }
            }
            else
            {
                fputc(c, out);
            }
        }
        else if (state == 1)
        {
            if (c == '\n')
            {
                fputc('\n', out);
                state = 0;
            }
        }
        else if (state == 2)
        {
            if (c == '\n')
                fputc('\n', out);

            if (c == '*')
            {
                next = fgetc(in);

                if (next == '/')
                    state = 0;
                else if (next != EOF)
                    ungetc(next, in);
            }
        }
        else if (state == 3 || state == 4)
        {
            fputc(c, out);

            if (c == '\\')
            {
                next = fgetc(in);
                if (next != EOF)
                    fputc(next, out);
            }
            else if ((state == 3 && c == '"') ||
                     (state == 4 && c == '\''))
            {
                state = 0;
            }
        }
    }

    if (state == 2)
    {
        fprintf(stderr, "Error: unterminated multi-line comment.\n");
        fclose(in);
        fclose(out);
        return 1;
    }

    if (state == 3 || state == 4)
    {
        fprintf(stderr, "Error: unterminated string or character literal.\n");
        fclose(in);
        fclose(out);
        return 1;
    }

    fclose(in);
    fclose(out);
    return 0;
}
