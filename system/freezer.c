/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <gio/gio.h>

#include "freezer.h"
#include "../common/utils.h"

struct _FreezerPrivate {
    GList *processes;
};


G_DEFINE_TYPE_WITH_CODE (
    Freezer,
    freezer,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Freezer)
)


struct Process {
    pid_t pid;
    gchar *cmdline;
};


static gboolean
freezer_in_list (GList *names, struct Process *process)
{
    gchar *name;

    GFOREACH (names, name)
        if (g_strrstr (process->cmdline, name) != NULL)
            return TRUE;
    return FALSE;
}

static void
freezer_get_pids (Freezer *self)
{
    g_autoptr(GDir) proc_dir = NULL;
    const char *pid_dir;

    proc_dir = g_dir_open ("/proc", 0, NULL);
    if (proc_dir == NULL) {
        g_warning ("/proc not mounted");
        return;
    }

    g_list_free_full (self->priv->processes, g_free);
    self->priv->processes = NULL;

    while ((pid_dir = g_dir_read_name (proc_dir)) != NULL) {
        g_autofree gchar *contents = NULL;
        g_autofree gchar *filename = g_build_filename (
            "/proc", pid_dir, "cmdline", NULL
        );

        if (!g_file_test (filename, G_FILE_TEST_EXISTS))
            continue;

        if (g_file_get_contents (filename, &contents, NULL, NULL)) {
            struct Process *process = g_malloc (sizeof (struct Process));

            contents = g_strchomp (contents);
            process->cmdline = g_steal_pointer (&contents);
            sscanf (pid_dir, "%d", &process->pid);

            self->priv->processes = g_list_prepend (
                self->priv->processes, process
            );
        }
    }
}



static void
freezer_dispose (GObject *freezer)
{
    Freezer *self = FREEZER (freezer);

    g_list_free_full (self->priv->processes, g_free);
    g_free (self->priv);

    G_OBJECT_CLASS (freezer_parent_class)->dispose (freezer);
}

static void
freezer_class_init (FreezerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = freezer_dispose;
}

static void
freezer_init (Freezer *self)
{
    self->priv = freezer_get_instance_private (self);

    self->priv->processes = NULL;
}

/**
 * freezer_new:
 *
 * Creates a new #Freezer
 *
 * Returns: (transfer full): a new #Freezer
 *
 **/
GObject *
freezer_new (void)
{
    GObject *freezer;

    freezer = g_object_new (TYPE_FREEZER, NULL);

    return freezer;
}

/**
 * freezer_suspend_processes:
 *
 * Set freezer devices to powersave
 *
 * @param #Freezer
 * @param suspend: True to suspend
 * @param cmdlines: process cmdline list
 */
void
freezer_suspend_processes (Freezer  *self,
                           gboolean  suspend,
                           GList *names) {

    struct Process *process;

    freezer_get_pids (self);

    GFOREACH (self->priv->processes, process)
        if (freezer_in_list (names, process)) {
            if (suspend) {
                kill (process->pid, SIGSTOP);
            } else {
                kill (process->pid, SIGCONT);
            }
        }
}