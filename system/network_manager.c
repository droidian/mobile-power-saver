/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "network_manager.h"
#include "../common/utils.h"

#define NETWORK_MANAGER_DBUS_NAME        "org.freedesktop.NetworkManager"
#define NETWORK_MANAGER_DBUS_PATH        "/org/freedesktop/NetworkManager"
#define NETWORK_MANAGER_DBUS_INTERFACE   "org.freedesktop.NetworkManager"
#define DBUS_PROPERTIES_INTERFACE        "org.freedesktop.DBus.Properties"

/* signals */
enum
{
    CONNECTION_TYPE_WIFI,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _NetworkManagerPrivate {
    GDBusProxy *network_manager_proxy;
};

G_DEFINE_TYPE_WITH_CODE (
    NetworkManager,
    network_manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (NetworkManager)
)


static GVariant *
get_connection_type (NetworkManager *self)
{
    g_autoptr (GDBusProxy) properties_bus_proxy;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GError) error = NULL;
    GVariant *inner_value = NULL;

    properties_bus_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        NETWORK_MANAGER_DBUS_NAME,
        NETWORK_MANAGER_DBUS_PATH,
        DBUS_PROPERTIES_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error (
            "Can't read properties from NetworkManager: %s", error->message
        );
        return NULL;
    }

    value = g_dbus_proxy_call_sync (
        properties_bus_proxy,
        "Get",
        g_variant_new ("(ss)",
                       NETWORK_MANAGER_DBUS_INTERFACE,
                       "PrimaryConnectionType"
        ),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error (
            "Can't read PrimaryConnectionType from NetworkManager: %s",
            error->message
        );
        return NULL;
    }

    g_variant_get (value, "(v)", &inner_value);

    return inner_value;
}

static void
set_connection_type (NetworkManager *self,
                     GVariant       *value)
{
    g_autofree gchar *connection_type = NULL;

    g_variant_get (value, "s", &connection_type);
    g_debug ("Connection type: %s", connection_type);

    g_signal_emit(
        self,
        signals[CONNECTION_TYPE_WIFI],
        0,
        g_strcmp0 (connection_type, "802-11-wireless") == 0
    );
}

static void
on_network_manager_proxy_properties (GDBusProxy  *proxy,
                                     GVariant    *changed_properties,
                                     char       **invalidated_properties,
                                     gpointer     user_data)
{
    NetworkManager *self = user_data;
    GVariant *value;
    char *property;
    GVariantIter i;

    g_variant_iter_init (&i, changed_properties);
    while (g_variant_iter_next (&i, "{&sv}", &property, &value)) {
        if (g_strcmp0 (property, "PrimaryConnectionType") == 0) {
            set_connection_type (self, value);
        }
        g_variant_unref (value);
    }
}

static void
network_manager_dispose (GObject *network_manager)
{
    NetworkManager *self = NETWORK_MANAGER (network_manager);

    g_clear_object (&self->priv->network_manager_proxy);

    G_OBJECT_CLASS (network_manager_parent_class)->dispose (network_manager);
}

static void
network_manager_finalize (GObject *network_manager)
{
    G_OBJECT_CLASS (network_manager_parent_class)->finalize (network_manager);
}

static void
network_manager_class_init (NetworkManagerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = network_manager_dispose;
    object_class->finalize = network_manager_finalize;

    signals[CONNECTION_TYPE_WIFI] = g_signal_new (
        "connection-type-wifi",
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
network_manager_init (NetworkManager *self)
{
    g_autoptr (GError) error = NULL;

    self->priv = network_manager_get_instance_private (self);

    self->priv->network_manager_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        NETWORK_MANAGER_DBUS_NAME,
        NETWORK_MANAGER_DBUS_PATH,
        NETWORK_MANAGER_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error("Can't contact NetworkManager: %s", error->message);
        return;
    }

    g_signal_connect (
        self->priv->network_manager_proxy,
        "g-properties-changed",
        G_CALLBACK (on_network_manager_proxy_properties),
        self
    );


}

/**
 * network_manager_new:
 *
 * Creates a new #NetworkManager
 *
 * Returns: (transfer full): a new #NetworkManager
 *
 **/
GObject *
network_manager_new (void)
{
    GObject *network_manager;

    network_manager = g_object_new (TYPE_NETWORK_MANAGER, NULL);

    return network_manager;
}

/**
 * network_manager_check_wifi:
 *
 * Check for a Wi-Fi main connection
 *
 **/
void
network_manager_check_wifi (NetworkManager *self)
{
    g_autoptr (GVariant) value = NULL;

    value = get_connection_type (self);
    set_connection_type (self, value);
}