/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <gio/gio.h>

#include "dozing.h"
#include "../common/define.h"
#include "../common/utils.h"

#define DOZING_MIN          60          /* Minimal dozing duration in seconds */
#define DOZING_MAX          1800        /* Maximal dozing duration in seconds */
#define DOZING_DELTA        60          /* Delta dozing duration in seconds */
#define DOZING_MAINTENANCE  300 / 1800  /* 5 minutes for 30 minutes dozing */


struct _DozingPrivate {
    GList *apps;

    guint timeout;
    guint timeout_id;
};


G_DEFINE_TYPE_WITH_CODE (
    Dozing,
    dozing,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Dozing)
)

static gboolean dozing_unfreeze_apps (Dozing *self);
static gboolean
dozing_freeze_apps (Dozing *self)
{
    const gchar *app;

    g_return_val_if_fail (self->priv->apps != NULL, FALSE);

    g_message("Freezing apps");
    GFOREACH (self->priv->apps, app)
        write_to_file (app, "1");

    self->priv->timeout_id = g_timeout_add_seconds (
        self->priv->timeout,
        (GSourceFunc) dozing_unfreeze_apps,
        self
    );

    return FALSE;
}

static gboolean
dozing_unfreeze_apps (Dozing *self)
{
    const gchar *app;

    g_return_val_if_fail (self->priv->apps != NULL, FALSE);

    g_message("Freezing apps");
    GFOREACH (self->priv->apps, app)
        write_to_file (app, "0");


    self->priv->timeout_id = g_timeout_add_seconds (
        (guint) (self->priv->timeout * DOZING_MAINTENANCE),
        (GSourceFunc) dozing_freeze_apps,
        self
    );

    /* Doze for some more time */
    self->priv->timeout = MIN (DOZING_MAX, self->priv->timeout + DOZING_DELTA);

    return FALSE;
}

static GList *
dozing_get_apps (Dozing *self)
{
    g_autoptr(GDir) sys_dir = NULL;
    g_autofree gchar *dirname = g_strdup_printf(
        CGROUPS_FREEZE_DIR, getuid(), getuid()

    );
    const char *app_dir;
    GList *apps = NULL;

    sys_dir = g_dir_open (dirname, 0, NULL);
    if (sys_dir == NULL) {
        g_warning ("Can't find cgroups user app slice: %s", dirname);
        return NULL;
    }

    while ((app_dir = g_dir_read_name (sys_dir)) != NULL) {
        if (g_str_has_prefix (app_dir, "app-")) {
            gchar *app = g_build_filename (
                dirname, app_dir, "cgroup.freeze", NULL
            );

            if (!g_file_test (app, G_FILE_TEST_EXISTS))
                continue;

            apps = g_list_prepend (apps, app);
        }
    }
    return apps;
}



static void
dozing_dispose (GObject *dozing)
{
    G_OBJECT_CLASS (dozing_parent_class)->dispose (dozing);
}


static void
dozing_finalize (GObject *dozing)
{
    Dozing *self = DOZING (dozing);

    g_list_free_full (self->priv->apps, g_free);

    G_OBJECT_CLASS (dozing_parent_class)->finalize (dozing);
}


static void
dozing_class_init (DozingClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = dozing_dispose;
    object_class->finalize = dozing_finalize;
}

static void
dozing_init (Dozing *self)
{
    self->priv = dozing_get_instance_private (self);

    self->priv->apps = NULL;
    self->priv->timeout = DOZING_MIN;
}

/**
 * dozing_new:
 *
 * Creates a new #Dozing
 *
 * Returns: (transfer full): a new #Dozing
 *
 **/
GObject *
dozing_new (void)
{
    GObject *dozing;

    dozing = g_object_new (TYPE_DOZING, NULL);

    return dozing;
}

/**
 * dozing_start:
 *
 * Start dozing (freezing/unfreezing apps)
 *
 * @param #Dozing
 */
void
dozing_start (Dozing  *self) {
    self->priv->apps = dozing_get_apps(self);

    self->priv->timeout = DOZING_MIN;
    dozing_freeze_apps (self);
}

/**
 * dozing_stop:
 *
 * Stop dozing
 *
 * @param #Dozing
 */
void
dozing_stop (Dozing  *self) {
    const gchar *app;

    g_clear_handle_id (&self->priv->timeout_id, g_source_remove);

    GFOREACH (self->priv->apps, app)
        write_to_file (app, "0");

    g_list_free_full (self->priv->apps, g_free);
    self->priv->apps = NULL;
}
