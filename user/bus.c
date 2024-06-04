/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>
#include <glib/gi18n.h>

#include "config.h"
#include "bus.h"
#include "settings.h"

#define DBUS_MPS_NAME                "org.adishatz.Mps"
#define DBUS_MPS_PATH                "/org/adishatz/Mps"
#define DBUS_MPS_INTERFACE           "org.adishatz.Mps"

/* signals */
enum
{
    SCREEN_STATE_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _BusPrivate {
    GDBusProxy *mps_proxy;
};

G_DEFINE_TYPE_WITH_CODE (Bus, bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (Bus))

static void
on_mps_proxy_signal (GDBusProxy  *proxy,
                     const gchar *sender_name,
                     const gchar *signal_name,
                     GVariant    *parameters,
                     gpointer     user_data)
{
    Bus *self = BUS (user_data);

    if (g_strcmp0 (signal_name, "ScreenStateChanged") == 0) {
        gboolean enabled;

        g_variant_get (parameters, "(b)", &enabled);

        g_signal_emit(
            self,
            signals[SCREEN_STATE_CHANGED],
            0,
            enabled
        );
    }
}

static void
bus_dispose (GObject *bus)
{
    Bus *self = BUS (bus);

    g_clear_object (&self->priv->mps_proxy);

    G_OBJECT_CLASS (bus_parent_class)->dispose (bus);
}

static void
bus_finalize (GObject *bus)
{
    G_OBJECT_CLASS (bus_parent_class)->finalize (bus);
}

static void
bus_class_init (BusClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = bus_dispose;
    object_class->finalize = bus_finalize;

    signals[SCREEN_STATE_CHANGED] = g_signal_new (
        "screen-state-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_BOOLEAN
    );

}

static void
bus_init (Bus *self)
{
    self->priv = bus_get_instance_private (self);

    self->priv->mps_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        DBUS_MPS_NAME,
        DBUS_MPS_PATH,
        DBUS_MPS_INTERFACE,
        NULL,
        NULL
    );

    g_signal_connect (
        self->priv->mps_proxy,
        "g-signal",
        G_CALLBACK (on_mps_proxy_signal),
        self
    );
}

/**
 * bus_new:
 *
 * Creates a new #Bus
 *
 * Returns: (transfer full): a new #Bus
 *
 **/
GObject *
bus_new (void)
{
    GObject *bus;

    bus = g_object_new (
        TYPE_BUS,
        NULL
    );

    return bus;
}

/**
 * bus_set_value:
 *
 * Set value on the bus.
 *
 * @self: a #Bus
 * @key: a setting key
 * @value: a setting value
 */
void
bus_set_value (Bus         *self,
               const gchar *key,
               GVariant    *value)
{
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) result = NULL;

    result = g_dbus_proxy_call_sync (
        self->priv->mps_proxy,
        "Set",
        g_variant_new ("(&sv)", key, value),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL)
        g_warning ("Error setting value: %s", error->message);
}

static Bus *default_bus = NULL;
/**
 * bus_get_default:
 *
 * Gets the default #Bus.
 *
 * Return value: (transfer full): the default #Bus.
 */
Bus *
bus_get_default (void)
{
    if (!default_bus) {
        default_bus = BUS (bus_new ());
    }
    return default_bus;
}
