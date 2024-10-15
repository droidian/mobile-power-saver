/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <gio/gio.h>

#include "bus.h"
#include "dozing.h"
#include "mpris.h"
#include "network_manager.h"
#include "settings.h"
#include "../common/define.h"
#include "../common/utils.h"

#define DOZING_PRE_SLEEP          60
#define DOZING_LIGHT_SLEEP        300
#define DOZING_LIGHT_MAINTENANCE  30
#define DOZING_MEDIUM_SLEEP       600
#define DOZING_MEDIUM_MAINTENANCE 50
#define DOZING_FULL_SLEEP         1200
#define DOZING_FULL_MAINTENANCE   80

enum DozingType {
    DOZING_LIGHT,
    DOZING_LIGHT_1,
    DOZING_LIGHT_2,
    DOZING_LIGHT_3,
    DOZING_MEDIUM,
    DOZING_MEDIUM_1,
    DOZING_MEDIUM_2,
    DOZING_FULL
};

struct _DozingPrivate {
    GList *apps;
    NetworkManager *network_manager;
    Mpris *mpris;

    guint type;
    guint timeout_id;
};

G_DEFINE_TYPE_WITH_CODE (
    Dozing,
    dozing,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Dozing)
)

static gboolean freeze_apps (Dozing *self);
static gboolean unfreeze_apps (Dozing *self);

static guint
get_maintenance (Dozing *self)
{
    if (self->priv->type < DOZING_MEDIUM)
        return DOZING_LIGHT_MAINTENANCE;
    else if (self->priv->type < DOZING_FULL)
        return DOZING_MEDIUM_MAINTENANCE;
    else
        return DOZING_FULL_MAINTENANCE;
}

static guint
get_sleep (Dozing *self)
{
    if (self->priv->type < DOZING_MEDIUM)
        return DOZING_LIGHT_SLEEP;
    else if (self->priv->type < DOZING_FULL)
        return DOZING_MEDIUM_SLEEP;
    else
        return DOZING_FULL_SLEEP;
}

static void
queue_next_freeze (Dozing *self)
{
    self->priv->timeout_id = g_timeout_add_seconds (
        get_maintenance (self),
        (GSourceFunc) freeze_apps,
        self
    );

    if (self->priv->type < DOZING_FULL)
        self->priv->type += 1;
}

static gboolean
freeze_apps (Dozing *self)
{
    Bus *bus = bus_get_default ();
    const char *app;
    gboolean data_used;
    gboolean little_cluster_powersave = TRUE;

    network_manager_stop_modem_monitoring (self->priv->network_manager);

    data_used = network_manager_data_used (self->priv->network_manager);

    if (!data_used)
        bus_set_value (bus, "suspend-modem", g_variant_new ("b", TRUE));
    else
        g_message ("Modem used: not suspending");

    if (self->priv->apps == NULL)
        return FALSE;

    g_message("Freezing apps");
    GFOREACH (self->priv->apps, app) {
        if (!mpris_can_freeze (self->priv->mpris, app)) {
            little_cluster_powersave = FALSE;
            continue;
        }
        if (settings_can_freeze_app (settings_get_default (), app))
            write_to_file (app, "1");
    }

    bus_set_value (bus,
                   "little-cluster-powersave",
                   g_variant_new ("b", little_cluster_powersave));

    self->priv->timeout_id = g_timeout_add_seconds (
        get_sleep (self),
        (GSourceFunc) unfreeze_apps,
        self
    );

    return FALSE;
}

static gboolean
unfreeze_apps (Dozing *self)
{
    Bus *bus = bus_get_default ();
    const char *app;

    bus_set_value (bus, "suspend-modem", g_variant_new ("b", FALSE));

    if (self->priv->apps == NULL)
        return FALSE;

    g_message("Unfreezing apps");
    GFOREACH (self->priv->apps, app)
        write_to_file (app, "0");

    queue_next_freeze (self);

    return FALSE;
}

static GList *
get_apps (Dozing *self)
{
    g_autoptr (GDir) sys_dir = NULL;
    g_autofree char *dirname = g_strdup_printf(
        CGROUPS_APPS_FREEZE_DIR, getuid(), getuid()

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
    Dozing *self = DOZING (dozing);

    g_clear_object (&self->priv->network_manager);
    g_clear_object (&self->priv->mpris);

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

    self->priv->network_manager = NETWORK_MANAGER (network_manager_new ());
    self->priv->mpris = MPRIS (mpris_new ());

    self->priv->apps = NULL;
    self->priv->type = DOZING_LIGHT;
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
    self->priv->apps = get_apps(self);

    self->priv->type = DOZING_LIGHT;
    self->priv->timeout_id = g_timeout_add_seconds (
        DOZING_PRE_SLEEP,
        (GSourceFunc) freeze_apps,
        self
    );

    network_manager_start_modem_monitoring (self->priv->network_manager);
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
    Bus *bus = bus_get_default ();
    const char *app;

    g_clear_handle_id (&self->priv->timeout_id, g_source_remove);

    g_message("Unfreezing apps");
    GFOREACH (self->priv->apps, app)
        write_to_file (app, "0");

    bus_set_value (bus, "suspend-modem", g_variant_new ("b", FALSE));

    g_list_free_full (self->priv->apps, g_free);
    self->priv->apps = NULL;
}