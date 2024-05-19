/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <glib.h>

#include "utils.h"

void write_to_file (const gchar *filename,
                    const gchar *value)
{
    FILE *file;

    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
        return;

    file = fopen(filename, "w");

    if (file == NULL)
        g_warning ("Can't write to %s", filename);
    else {
        fprintf (file, "%s", value);
        fclose (file);
    }
}