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
#define NETWORK_MANAGER_DBUS_DEVICE      "org.freedesktop.NetworkManager.Device"
#define NETWORK_MANAGER_DBUS_WIRELESS    "org.freedesktop.NetworkManager.Device.Wireless"
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

    GList *devices;

    /* We do not check this on startup, assume it can't be on */
    gboolean access_point;
};

G_DEFINE_TYPE_WITH_CODE (
    NetworkManager,
    network_manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (NetworkManager)
)

static void
on_network_manager_proxy_properties (GDBusProxy  *proxy,
                                     GVariant    *changed_properties,
                                     char       **invalidated_properties,
                                     gpointer     user_data);

static void
add_device (NetworkManager *self,
            const gchar    *device_path)
{
    GDBusProxy *network_manager_proxy = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GError) error = NULL;
    GVariant *inner_value = NULL;
    guint device_type;

    network_manager_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        NETWORK_MANAGER_DBUS_NAME,
        device_path,
        DBUS_PROPERTIES_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error("Can't get network device properties: %s", error->message);
        g_clear_object (&network_manager_proxy);
        return;
    }

    value = g_dbus_proxy_call_sync (
        network_manager_proxy,
        "Get",
        g_variant_new ("(ss)",
                       NETWORK_MANAGER_DBUS_DEVICE,
                       "DeviceType"
        ),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    g_clear_object (&network_manager_proxy);

    if (error != NULL) {
        g_error (
            "Can't read DeviceType: %s",
            error->message
        );
        return;
    }

    g_variant_get (value, "(v)", &inner_value);
    g_variant_get (inner_value, "u", &device_type);

    if (device_type != 2) { /* NM_DEVICE_TYPE_WIFI */
        return;
    }

    network_manager_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        NETWORK_MANAGER_DBUS_NAME,
        device_path,
        NETWORK_MANAGER_DBUS_WIRELESS,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error("Can't get wireless device: %s", error->message);
        g_clear_object (&network_manager_proxy);
        return;
    }

    self->priv->devices = g_list_append (
        self->priv->devices, network_manager_proxy
    );

    g_signal_connect (
        network_manager_proxy,
        "g-properties-changed",
        G_CALLBACK (on_network_manager_proxy_properties),
        self
    );
}

static void
del_device (NetworkManager *self,
            const gchar    *device_path)
{
    GDBusProxy *network_wireless_proxy;

    GFOREACH (self->priv->devices, network_wireless_proxy) {
        if (g_strcmp0 (g_dbus_proxy_get_object_path (network_wireless_proxy), device_path) == 0) {
            self->priv->devices = g_list_remove (
                self->priv->devices, network_wireless_proxy
            );
            g_clear_object (&network_wireless_proxy);
            break;
        }
    }
}

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
on_network_manager_proxy_signal (GDBusProxy  *proxy,
                                 const gchar *sender_name,
                                 const gchar *signal_name,
                                 GVariant    *parameters,
                                 gpointer     user_data)
{
    NetworkManager *self = user_data;

    if (g_strcmp0 (signal_name, "DeviceAdded") == 0) {
        g_autofree gchar *object_path = NULL;

        g_variant_get (parameters, "(o)", &object_path);
        add_device (self, object_path);
    } else if (g_strcmp0 (signal_name, "DeviceRemoved") == 0) {
        const gchar *object_path = NULL;

        g_variant_get (parameters, "(o)", &object_path);
        del_device (self, object_path);
    }
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
        } else if (g_strcmp0 (property, "ActiveAccessPoint") == 0) {
            const gchar *object_path = NULL;

            g_variant_get (value, "&o", &object_path);
            self->priv->access_point = g_strcmp0 (object_path, "/") != 0;
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
    NetworkManager *self = NETWORK_MANAGER (network_manager);

    g_list_free (self->priv->devices);

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
    g_autoptr (GVariantIter) iter = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GError) error = NULL;
    const gchar *device_path;

    self->priv = network_manager_get_instance_private (self);

    self->priv->devices = NULL;
    self->priv->access_point = FALSE;

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

    g_signal_connect (
        self->priv->network_manager_proxy,
        "g-signal",
        G_CALLBACK (on_network_manager_proxy_signal),
        self
    );

    value = g_dbus_proxy_call_sync (
        self->priv->network_manager_proxy,
        "GetDevices",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning ("Can't get network devices: %s", error->message);
        return;
    }

    g_variant_get (value, "(ao)", &iter);
    while (g_variant_iter_loop (iter, "&o", &device_path, NULL)) {
        add_device (self, device_path);
    }
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
 * @param self: #NetworkManager
 *
 **/
void
network_manager_check_wifi (NetworkManager *self)
{
    g_autoptr (GVariant) value = NULL;

    value = get_connection_type (self);
    set_connection_type (self, value);
}

/**
 * network_manager_has_ap:
 *
 * Check if an access point is available
 *
 * @param self: #NetworkManager
 *
 * Returns: True if an access point is available
 *
 **/
gboolean
network_manager_has_ap (NetworkManager *self)
{
    return self->priv->access_point;
}