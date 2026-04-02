/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <glib.h>
#include <unistd.h>

#include "define.h"
#include "utils.h"

gboolean write_to_file (const char *filename,
                        const char *value)
{
    FILE *file;

    g_debug ("%s: %s", filename, value);

    if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
        g_debug ("File doesn't exist: %s", filename);
        return FALSE;
    }

    file = fopen(filename, "w");

    if (file == NULL) {
        g_debug ("Can't write to file: %s", filename);
        return FALSE;
    }

    fprintf (file, "%s", value);

    if (fclose (file) < 0) {
        return FALSE;
    }

    return TRUE;
}


GList *get_applications (void)
{
    g_autoptr (GDir) sys_dir = NULL;
    g_autofree char *dirname = g_strdup_printf(
        CGROUPS_USER_APPS_DIR, getuid(), getuid()
    );
    const char *app_dir;
    GList *apps = NULL;

    sys_dir = g_dir_open (dirname, 0, NULL);
    if (sys_dir == NULL) {
        g_warning ("Can't find cgroups user app slice: %s", dirname);
        return NULL;
    }

    while ((app_dir = g_dir_read_name (sys_dir)) != NULL) {
        if (g_str_has_prefix (app_dir, "app-") &&
                g_str_has_suffix (app_dir, ".scope")) {
            char *app = g_build_filename (
                dirname, app_dir, "cgroup.freeze", NULL
            );

            if (!g_file_test (app, G_FILE_TEST_EXISTS)) {
                g_free (app);
                continue;
            }

            apps = g_list_prepend (apps, app);
        }
    }
    return apps;
}

GList *get_cgroup_apps (const char *path)
{
    g_autoptr (GDir) sys_dir = NULL;

    const char *cgroup_dir;
    GList *apps = NULL;

    sys_dir = g_dir_open (path, 0, NULL);
    if (sys_dir == NULL) {
        g_warning ("Can't find cgroup: %s", path);
        return NULL;
    }

    while ((cgroup_dir = g_dir_read_name (sys_dir)) != NULL) {
        g_autofree char *cgroup = NULL;

        if (g_str_has_suffix (cgroup_dir, ".scope")) {
            cgroup = g_build_filename (
                path, cgroup_dir, "cgroup.procs", NULL
            );

            if (!g_file_test (cgroup, G_FILE_TEST_EXISTS)) {
                g_warning ("cgroup not found: %s", cgroup);
                continue;
            }

            apps = g_list_prepend (apps, g_strdup (cgroup_dir));
        }
    }
    return apps;
}

GList*
get_cgroup_slices (const char *path)
{
    g_autoptr (GDir) sys_dir = NULL;

    const char *cgroup_dir;
    GList *slices = NULL;

    sys_dir = g_dir_open (path, 0, NULL);
    if (sys_dir == NULL) {
        g_warning ("Can't find cgroup: %s", path);
        return NULL;
    }

    while ((cgroup_dir = g_dir_read_name (sys_dir)) != NULL) {
        g_autofree char *slice = NULL;

        if (g_str_has_suffix (cgroup_dir, ".slice")) {
            slice = g_build_filename (
                path, cgroup_dir, NULL
            );
            slices = g_list_concat (slices, get_cgroup_slices (slice));
            slices = g_list_prepend (slices, g_strdup (slice));
        }
    }
    return slices;
}

GList*
get_irqs (void)
{
    g_autoptr (GDir) proc_dir = NULL;
    g_autofree char *all_cpu_mask = get_all_cpu_mask ();

    const char *irq_dir;
    GList *irqs = NULL;

    proc_dir = g_dir_open ("/proc/irq", 0, NULL);
    if (proc_dir == NULL) {
        g_warning ("Can't find /proc/irq");
        return NULL;
    }

    while ((irq_dir = g_dir_read_name (proc_dir)) != NULL) {
        g_autofree char *affinity = NULL;
        g_autofree char *contents = NULL;
        g_autoptr(GError) error = NULL;

        affinity = g_build_filename (
            "/proc/irq", irq_dir, "smp_affinity", NULL
        );

        /* Check affinity is writable */
        if (g_file_get_contents (affinity, &contents, NULL, &error)) {
            if (g_strcmp0 (all_cpu_mask, g_strchomp (contents)) != 0) {
                g_warning ("Affinity already set: %s", affinity);
                continue;
            }
            if (!write_to_file (affinity, contents)) {
                g_warning ("Can't write affinity: %s", affinity);
                continue;
            }
            irqs = g_list_append (irqs, g_strdup (affinity));
        }
    }
    return irqs;
}

GList*
get_list_from_variant (GVariant *value)
{
    GList *list = NULL;
    g_autoptr (GVariantIter) iter;
    const char *item;

    g_variant_get (value, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &item)) {
        list = g_list_append (list, g_strdup (item));
    }

    return list;
}

char*
get_little_cpu_mask (void)
{
    g_auto(GStrv) cpus = NULL;
    g_autofree char *contents = NULL;
    g_autoptr(GError) error = NULL;
    guint32 mask = 0;

    if (!g_file_get_contents (
            "/sys/devices/system/cpu/cpufreq/policy0/related_cpus",
            &contents, NULL, &error)) {
        g_warning ("Failed to read policy0 related_cpus: %s", error->message);
        return NULL;
    }

    cpus = g_strsplit (g_strstrip (contents), " ", -1);
    for (gint i = 0; cpus[i]; i++)
        mask |= 1u << g_ascii_strtoull (cpus[i], NULL, 10);

    return g_strdup_printf ("%x", mask);
}

gchar *
get_all_cpu_mask (void)
{
    g_autofree char *contents = NULL;
    g_autoptr(GError) error = NULL;
    g_auto(GStrv) parts = NULL;
    guint32 last, mask;

    if (!g_file_get_contents ("/sys/devices/system/cpu/possible",
                              &contents, NULL, &error)) {
        g_warning ("Failed to read cpu possible: %s", error->message);
        return NULL;
    }

    parts = g_strsplit (g_strstrip (contents), "-", 2);
    if (!parts[0] || !parts[1])
        return NULL;

    last = (guint32) g_ascii_strtoull (parts[1], NULL, 10);
    mask = (1u << (last + 1)) - 1;

    return g_strdup_printf ("%x", mask);
}
