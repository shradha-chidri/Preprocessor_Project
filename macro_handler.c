#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"

static Macro macros[MAX_MACROS];
static int macro_count = 0;

static int is_identifier_char(int c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static int find_macro(const char *name)
{
    for (int i = 0; i < macro_count; i++)
    {
        if (strcmp(macros[i].name, name) == 0)
            return i;
    }

    return -1;
}

static int add_macro(const char *name, const char *value,
                     int is_function, const char *parameter)
{
    int index = find_macro(name);

    if (index < 0)
    {
        if (macro_count >= MAX_MACROS)
        {
            fprintf(stderr, "Error: too many macros.\n");
            return 1;
        }

        index = macro_count++;
    }

    snprintf(macros[index].name, MAX_NAME, "%s", name);
    snprintf(macros[index].value, MAX_VALUE, "%s", value);
    snprintf(macros[index].parameter, MAX_NAME, "%s",
             parameter != NULL ? parameter : "");
    macros[index].is_function = is_function;

    return 0;
}

static int substitute_text(const char *input, char *output,
                           size_t output_size, int depth)
{
    size_t used = 0;
    const char *p = input;
    int quote = 0;

    if (depth > 20)
    {
        fprintf(stderr, "Error: macro expansion too deep.\n");
        return 1;
    }

    while (*p != '\0')
    {
        if (*p == '"' || *p == '\'')
        {
            quote = *p;
            if (used + 1 >= output_size)
                return 1;

            output[used++] = *p++;

            while (*p != '\0')
            {
                if (used + 1 >= output_size)
                    return 1;

                output[used++] = *p;

                if (*p == '\\' && p[1] != '\0')
                {
                    p++;
                    if (used + 1 >= output_size)
                        return 1;
                    output[used++] = *p++;
                    continue;
                }

                if (*p == quote)
                {
                    p++;
                    break;
                }

                p++;
            }

            quote = 0;
            continue;
        }

        if (isalpha((unsigned char)*p) || *p == '_')
        {
            char name[MAX_NAME];
            size_t n = 0;

            while (is_identifier_char((unsigned char)*p))
            {
                if (n + 1 < sizeof(name))
                    name[n++] = *p;
                p++;
            }

            name[n] = '\0';

            int index = find_macro(name);

            if (index >= 0 && !macros[index].is_function)
            {
                char expanded[4096];

                if (substitute_text(macros[index].value, expanded,
                                    sizeof(expanded), depth + 1) != 0)
                    return 1;

                size_t length = strlen(expanded);

                if (used + length >= output_size)
                    return 1;

                memcpy(output + used, expanded, length);
                used += length;
            }
            else if (index >= 0 && macros[index].is_function)
            {
                const char *look = p;

                while (isspace((unsigned char)*look))
                    look++;

                if (*look == '(')
                {
                    const char *arg_start = look + 1;
                    const char *arg_end = arg_start;
                    int level = 1;

                    while (*arg_end != '\0' && level > 0)
                    {
                        if (*arg_end == '(')
                            level++;
                        else if (*arg_end == ')')
                            level--;
                        if (level > 0)
                            arg_end++;
                    }

                    if (level != 0)
                    {
                        fprintf(stderr, "Error: missing ')' for macro %s.\n",
                                name);
                        return 1;
                    }

                    size_t arg_len = (size_t)(arg_end - arg_start);
                    char argument[2048];
                    char expanded_argument[4096];
                    char replacement[4096];
                    size_t r = 0;

                    if (arg_len >= sizeof(argument))
                        return 1;

                    memcpy(argument, arg_start, arg_len);
                    argument[arg_len] = '\0';

                    if (substitute_text(argument, expanded_argument,
                                        sizeof(expanded_argument),
                                        depth + 1) != 0)
                        return 1;

                    const char *value = macros[index].value;
                    size_t param_len = strlen(macros[index].parameter);

                    while (*value != '\0')
                    {
                        if ((isalpha((unsigned char)*value) ||
                             *value == '_') &&
                            strncmp(value, macros[index].parameter,
                                    param_len) == 0 &&
                            !is_identifier_char((unsigned char)value[param_len]))
                        {
                            size_t arg_size = strlen(expanded_argument);

                            if (r + arg_size >= sizeof(replacement))
                                return 1;

                            memcpy(replacement + r, expanded_argument,
                                   arg_size);
                            r += arg_size;
                            value += param_len;
                        }
                        else
                        {
                            if (r + 1 >= sizeof(replacement))
                                return 1;

                            replacement[r++] = *value++;
                        }
                    }

                    replacement[r] = '\0';

                    char expanded[4096];

                    if (substitute_text(replacement, expanded,
                                        sizeof(expanded), depth + 1) != 0)
                        return 1;

                    size_t length = strlen(expanded);

                    if (used + length >= output_size)
                        return 1;

                    memcpy(output + used, expanded, length);
                    used += length;
                    p = arg_end + 1;
                }
                else
                {
                    size_t length = strlen(name);

                    if (used + length >= output_size)
                        return 1;

                    memcpy(output + used, name, length);
                    used += length;
                }
            }
            else
            {
                size_t length = strlen(name);

                if (used + length >= output_size)
                    return 1;

                memcpy(output + used, name, length);
                used += length;
            }
        }
        else
        {
            if (used + 1 >= output_size)
                return 1;

            output[used++] = *p++;
        }
    }

    output[used] = '\0';
    return 0;
}

int process_macros(const char *input_file, const char *output_file)
{
    FILE *in = fopen(input_file, "r");
    FILE *out = fopen(output_file, "w");
    char line[4096];

    if (in == NULL || out == NULL)
    {
        perror("Error opening file");
        if (in != NULL)
            fclose(in);
        if (out != NULL)
            fclose(out);
        return 1;
    }

    macro_count = 0;

    while (fgets(line, sizeof(line), in) != NULL)
    {
        char *p = line;

        while (isspace((unsigned char)*p))
            p++;

        if (strncmp(p, "#define", 7) == 0 &&
            isspace((unsigned char)p[7]))
        {
            char name[MAX_NAME];
            char parameter[MAX_NAME] = "";
            char value[MAX_VALUE] = "";
            int is_function = 0;
            size_t n = 0;

            p += 7;
            while (isspace((unsigned char)*p))
                p++;

            while (is_identifier_char((unsigned char)*p))
            {
                if (n + 1 < sizeof(name))
                    name[n++] = *p;
                p++;
            }

            name[n] = '\0';

            if (n == 0)
            {
                fprintf(stderr, "Error: invalid #define directive.\n");
                fclose(in);
                fclose(out);
                return 1;
            }

            if (*p == '(')
            {
                char *end = strchr(p, ')');

                if (end == NULL)
                {
                    fprintf(stderr, "Error: invalid function macro.\n");
                    fclose(in);
                    fclose(out);
                    return 1;
                }

                size_t plen = (size_t)(end - p - 1);

                if (plen >= sizeof(parameter))
                    return 1;

                memcpy(parameter, p + 1, plen);
                parameter[plen] = '\0';

                is_function = 1;
                p = end + 1;
            }

            while (isspace((unsigned char)*p))
                p++;

            size_t vlen = strcspn(p, "\r\n");

            if (vlen >= sizeof(value))
                vlen = sizeof(value) - 1;

            memcpy(value, p, vlen);
            value[vlen] = '\0';

            if (add_macro(name, value, is_function, parameter) != 0)
            {
                fclose(in);
                fclose(out);
                return 1;
            }
        }
        else
        {
            char expanded[8192];

            if (substitute_text(line, expanded, sizeof(expanded), 0) != 0)
            {
                fprintf(stderr, "Error: macro substitution failed.\n");
                fclose(in);
                fclose(out);
                return 1;
            }

            fputs(expanded, out);
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}
