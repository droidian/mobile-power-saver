/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <gio/gio.h>

#include "bus.h"
#include "config.h"
#include "dozing.h"
#include "modem.h"
#ifdef MM_ENABLED
#include "modem_mm.h"
#else
#include "modem_ofono.h"
#endif
#include "network_manager.h"
#include "network_manager_modem.h"
#include "settings.h"
#include "../common/services.h"
#include "../common/utils.h"

#define DOZING_PRE_SLEEP          60
#define DOZING_LIGHT_SLEEP        300
#define DOZING_LIGHT_MAINTENANCE  20
#define DOZING_MEDIUM_SLEEP       600
#define DOZING_MEDIUM_MAINTENANCE 40
#define DOZING_FULL_SLEEP         1200
#define DOZING_FULL_MAINTENANCE   60
#define MODEM_APPLY_DELAY 500

enum DozingType {
    DOZING_LIGHT,
    DOZING_LIGHT_1,
    DOZING_LIGHT_2,
    DOZING_LIGHT_3,
    DOZING_LIGHT_4,
    DOZING_LIGHT_5,
    DOZING_LIGHT_6, /* 30 minutes */
    DOZING_MEDIUM,
    DOZING_MEDIUM_1,
    DOZING_MEDIUM_2,
    DOZING_MEDIUM_3,
    DOZING_MEDIUM_4,
    DOZING_MEDIUM_5,
    DOZING_MEDIUM_6, /* 1 hour */
    DOZING_FULL
};

struct _DozingPrivate {
    GList *apps;
    Mpris *mpris;
    Modem  *modem;
    NetworkManager *network_manager;
    NetworkManagerModem *network_manager_modem;
    Services *services;

    guint type;
    guint timeout_id;

    gboolean radio_power_saving;

    guint modem_timeout_id;
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

static void
powersave_modem (Dozing   *self,
                 gboolean  enabled)
{
    ModemClass *klass;
    gboolean updated;

    if (!self->priv->radio_power_saving ||
            /* Here we assume AP set with screen on/dozing off */
            network_manager_has_ap (self->priv->network_manager))
        return;

    klass = MODEM_GET_CLASS (self->priv->modem);

    updated = modem_set_powersave (
        self->priv->modem, enabled, MODEM_POWERSAVE_DOZING
    );
    if (updated && self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
}

static void
freeze_services (Dozing *self)
{
    Bus *bus = bus_get_default ();

    g_message("Freezing services");

    bus_set_value (bus,
                   "dozing",
                   g_variant_new ("b", TRUE));

    if (settings_suspend_services (settings_get_default ())) {
        GList *blacklist = settings_get_suspend_services_blacklist (
            settings_get_default ()
        );

        services_freeze_all (self->priv->services, blacklist);

        g_list_free_full (blacklist, g_free);
    }
}

static void
unfreeze_services (Dozing *self)
{
    Bus *bus = bus_get_default ();

    g_message("Unfreezing services");

    bus_set_value (bus,
                   "dozing",
                   g_variant_new ("b", FALSE));

    if (settings_suspend_services (settings_get_default ())) {
        GList *blacklist = settings_get_suspend_services_blacklist (
            settings_get_default ()
        );

        services_unfreeze_all (self->priv->services, blacklist);

        g_list_free_full (blacklist, g_free);
    }
}

static gboolean
freeze_apps (Dozing *self)
{
    Bus *bus = bus_get_default ();
    const char *app;
    gboolean data_used;
    gboolean apps_active = FALSE;

    network_manager_modem_stop_monitoring (
        self->priv->network_manager_modem
    );
    data_used = network_manager_modem_data_used (
        self->priv->network_manager_modem
    );

    if (self->priv->apps != NULL) {
        g_message("Freezing apps");
        GFOREACH (self->priv->apps, app) {
            if (!mpris_can_freeze (self->priv->mpris, app)) {
                apps_active = TRUE;
                continue;
            }
            if (settings_can_freeze_app (settings_get_default (), app))
                write_to_file (app, "1");
        }
    }

    if (apps_active) {
        g_message ("Active apps: Keep little cluster active");
    } else {
        bus_set_value (bus,
                       "little-cluster-powersave",
                       g_variant_new ("b", TRUE));
    }

    if (data_used) {
        g_message ("Active modem: Keep data alive");
    } else {
        powersave_modem (self, TRUE);
    }

    freeze_services (self);

    g_clear_handle_id (&self->priv->timeout_id, g_source_remove);
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
    const char *app;

    powersave_modem (self, FALSE);
    unfreeze_services (self);

    network_manager_modem_start_monitoring (
        self->priv->network_manager_modem
    );

    g_message("Unfreezing apps");
    GFOREACH (self->priv->apps, app)
        write_to_file (app, "0");

    queue_next_freeze (self);

    return FALSE;
}

static gboolean
on_modem_timeout (gpointer user_data)
{
    Dozing *self = DOZING (user_data);
    ModemClass *klass;

    klass = MODEM_GET_CLASS (self->priv->modem);

    self->priv->modem_timeout_id = 0;

    if (self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
    else
        klass->reset_powersave (self->priv->modem);

    return FALSE;
}

static void
on_setting_changed (Settings   *settings,
                    const char *key,
                    GVariant   *value,
                    gpointer    user_data)
{
    Dozing *self = DOZING (user_data);

    if (g_strcmp0 (key, "radio-power-saving") == 0) {
        self->priv->radio_power_saving = g_variant_get_boolean (value);
        g_clear_handle_id (&self->priv->modem_timeout_id, g_source_remove);
        self->priv->modem_timeout_id = g_timeout_add (
            MODEM_APPLY_DELAY, (GSourceFunc) on_modem_timeout, self
        );
    }
}

static void
on_connection_type_wifi (NetworkManager *network_manager,
                         gboolean        enabled,
                         gpointer        user_data)
{
    Dozing *self = DOZING (user_data);
    ModemClass *klass;
    gboolean updated;

    klass = MODEM_GET_CLASS (self->priv->modem);

    updated = modem_set_powersave (
        self->priv->modem, enabled, MODEM_POWERSAVE_WIFI
    );

    if (updated && self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
}

static void
dozing_dispose (GObject *dozing)
{
    Dozing *self = DOZING (dozing);

    g_clear_object (&self->priv->network_manager);
    g_clear_object (&self->priv->modem);
    g_clear_object (&self->priv->mpris);
    g_clear_object (&self->priv->services);

    G_OBJECT_CLASS (dozing_parent_class)->dispose (dozing);
}

static void
dozing_finalize (GObject *dozing)
{
    Dozing *self = DOZING (dozing);

    g_list_free_full (self->priv->apps, g_free);
    g_clear_handle_id (&self->priv->timeout_id, g_source_remove);
    g_clear_handle_id (&self->priv->modem_timeout_id, g_source_remove);

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
    self->priv->network_manager_modem = NETWORK_MANAGER_MODEM (
        network_manager_modem_new ()
    );
#ifdef MM_ENABLED
    self->priv->modem = MODEM (modem_mm_new ());
#else
    self->priv->modem = MODEM (modem_ofono_new ());
#endif
    self->priv->mpris = MPRIS (mpris_new ());
    self->priv->services = SERVICES (services_new (G_BUS_TYPE_SESSION));

    self->priv->apps = NULL;
    self->priv->type = DOZING_LIGHT;

    self->priv->radio_power_saving = settings_get_radio_powersaving (
        settings_get_default()
    );

    self->priv->timeout_id = 0;
    self->priv->modem_timeout_id = 0;


    g_signal_connect (
        settings_get_default (),
        "setting-changed",
        G_CALLBACK (on_setting_changed),
        self
    );

    g_signal_connect (
        self->priv->network_manager,
        "connection-type-wifi",
        G_CALLBACK (on_connection_type_wifi),
        self
    );

    network_manager_check_wifi (self->priv->network_manager);
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

static Dozing *default_dozing = NULL;
/**
 * dozing_get_default:
 *
 * Gets the default #Dozing.
 *
 * Return value: (transfer full): the default #Dozing.
 */
Dozing *
dozing_get_default (void)
{
    if (!default_dozing) {
        default_dozing = DOZING (dozing_new ());
    }
    return default_dozing;
}

/**
 * dozing_start:
 *
 * Start dozing (freezing/unfreezing apps)
 *
 * @param #Dozing
 */
void
dozing_start (Dozing  *self)
{
    g_clear_handle_id (&self->priv->timeout_id, g_source_remove);

    self->priv->apps = get_applications();

    self->priv->type = DOZING_LIGHT;
    self->priv->timeout_id = g_timeout_add_seconds (
        DOZING_PRE_SLEEP,
        (GSourceFunc) freeze_apps,
        self
    );

    network_manager_modem_start_monitoring (self->priv->network_manager_modem);
}

/**
 * dozing_stop:
 *
 * Stop dozing
 *
 * @param #Dozing
 */
void
dozing_stop (Dozing  *self)
{
    const char *app;

    g_clear_handle_id (&self->priv->timeout_id, g_source_remove);

    powersave_modem (self, FALSE);
    unfreeze_services (self);

    network_manager_modem_stop_monitoring (
        self->priv->network_manager_modem
    );

    g_message("Unfreezing apps");
    GFOREACH (self->priv->apps, app)
        write_to_file (app, "0");

    g_list_free_full (self->priv->apps, g_free);
    self->priv->apps = NULL;
}
