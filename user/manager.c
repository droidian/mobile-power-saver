/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bus.h"
#include "dozing.h"
#include "manager.h"
#include "settings.h"


struct _ManagerPrivate {
    Dozing *dozing;

    gboolean screen_off_power_saving;
};


G_DEFINE_TYPE_WITH_CODE (
    Manager,
    manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Manager)
)


static void
on_setting_changed (Settings    *settings,
                    const gchar *key,
                    GVariant    *value,
                    gpointer     user_data)
{
    Manager *self = MANAGER (user_data);
    Bus *bus = bus_get_default ();

    bus_set_value (bus, key, value);

    if (g_strcmp0 (key, "screen-off-power-saving") == 0) {
        self->priv->screen_off_power_saving = g_variant_get_boolean (value);
    }
}

static void
on_screen_state_changed (Bus *bus, gboolean screen_on, gpointer user_data)
{
    Manager *self = MANAGER (user_data);

    if (self->priv->screen_off_power_saving) {
        if (screen_on) {
            dozing_stop (self->priv->dozing);
        } else {
            dozing_start (self->priv->dozing);
        }
    }
}


static void
manager_dispose (GObject *manager)
{
    Manager *self = MANAGER (manager);

    g_clear_object (&self->priv->dozing);

    G_OBJECT_CLASS (manager_parent_class)->dispose (manager);
}


static void
manager_finalize (GObject *manager)
{
    G_OBJECT_CLASS (manager_parent_class)->finalize (manager);
}


static void
manager_class_init (ManagerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = manager_dispose;
    object_class->finalize = manager_finalize;
}


static void
manager_init (Manager *self)
{
    self->priv = manager_get_instance_private (self);

    self->priv->dozing = DOZING (dozing_new ());
    self->priv->screen_off_power_saving = TRUE;

    g_signal_connect (
        bus_get_default (),
        "screen-state-changed",
        G_CALLBACK (on_screen_state_changed),
        self
    );

    g_signal_connect (
        settings_get_default (),
        "setting-changed",
        G_CALLBACK (on_setting_changed),
        self
    );
}

/**
 * manager_new:
 *
 * Creates a new #Manager
 *
 * Returns: (transfer full): a new #Manager
 *
 **/
GObject *
manager_new (void)
{
    GObject *manager;

    manager = g_object_new (TYPE_MANAGER, NULL);

    return manager;
}
