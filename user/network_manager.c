/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "network_manager.h"
#include "../common/utils.h"

#define NETWORK_MANAGER_DBUS_NAME             "org.freedesktop.NetworkManager"
#define NETWORK_MANAGER_DBUS_PATH             "/org/freedesktop/NetworkManager"
#define NETWORK_MANAGER_DBUS_INTERFACE        "org.freedesktop.NetworkManager"
#define NETWORK_MANAGER_DBUS_DEVICE_INTERFACE "org.freedesktop.NetworkManager.Device"
#define DBUS_PROPERTIES_INTERFACE             "org.freedesktop.DBus.Properties"

#define SYSDIR_PREFIX                         "/sys/class/net"
#define SYSDIR_SUFFIX                         "statistics"

struct _NetworkManagerPrivate {
    GDBusProxy *network_manager_proxy;

    GList *devices;

    gint64 start_timestamp;
    gint64 end_timestamp;

    guint64  start_rx;
    guint64  end_rx;
};

G_DEFINE_TYPE_WITH_CODE (
    NetworkManager,
    network_manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (NetworkManager)
)

static guint64
get_bytes (NetworkManager *self,
           const gchar    *path)
{
    g_autofree gchar *contents = NULL;

    if (g_file_get_contents (path, &contents, NULL, NULL))
        return g_ascii_strtoll (contents, NULL, 0);

    return 0;
}

static gchar*
get_hw_interface (NetworkManager *self,
                  GDBusProxy     *network_device_proxy)
{
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GVariant) inner_value = NULL;
    g_autoptr (GError) error = NULL;
    gchar *interface;

    value = g_dbus_proxy_call_sync (
        network_device_proxy,
        "Get",
        g_variant_new ("(ss)",
                       NETWORK_MANAGER_DBUS_DEVICE_INTERFACE,
                       "IpInterface"
        ),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error (
            "Can't read IpInterface: %s",
            error->message
        );
        return NULL;
    }

    g_variant_get (value, "(v)", &inner_value);
    g_variant_get (inner_value, "s", &interface);

    return interface;
}

static void
add_device (NetworkManager *self,
            const gchar    *device_path)
{
    GDBusProxy *network_device_proxy = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GVariant) inner_value = NULL;
    g_autoptr (GError) error = NULL;
    guint device_type;

    network_device_proxy = g_dbus_proxy_new_for_bus_sync (
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
        g_error("Can't get network device: %s", error->message);
        return;
    }

    value = g_dbus_proxy_call_sync (
        network_device_proxy,
        "Get",
        g_variant_new ("(ss)",
                       NETWORK_MANAGER_DBUS_DEVICE_INTERFACE,
                       "DeviceType"
        ),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error (
            "Can't read DeviceType: %s",
            error->message
        );
        g_clear_object (&network_device_proxy);
        return;
    }

    g_variant_get (value, "(v)", &inner_value);
    g_variant_get (inner_value, "u", &device_type);

    if (device_type != 8) { /* NM_DEVICE_TYPE_MODEM */
        g_clear_object (&network_device_proxy);
        return;
    }

    self->priv->devices = g_list_append (
        self->priv->devices, network_device_proxy
    );
}

static void
del_device (NetworkManager *self,
            const gchar    *device_path)
{
    GDBusProxy *network_device_proxy;

    GFOREACH (self->priv->devices, network_device_proxy) {
        if (g_strcmp0 (g_dbus_proxy_get_object_path (network_device_proxy), device_path) == 0) {
            self->priv->devices = g_list_remove (
                self->priv->devices, network_device_proxy
            );
            break;
        }
    }
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
}

static void
network_manager_init (NetworkManager *self)
{
    g_autoptr (GVariantIter) iter = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GError) error = NULL;
    const gchar *device_path = NULL;

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
 * network_manager_start_modem_monitoring:
 *
 * Start monitoring modem
 *
 * @param #NetworkManager
 *
 */
void
network_manager_start_modem_monitoring (NetworkManager *self)
{
    GDBusProxy *network_device_proxy;

    self->priv->start_rx = 0;
    self->priv->start_timestamp = g_get_monotonic_time();

    GFOREACH (self->priv->devices, network_device_proxy) {
        g_autofree gchar *interface = get_hw_interface (self, network_device_proxy);
        g_autofree gchar *filename = g_build_filename (
            SYSDIR_PREFIX, interface, SYSDIR_SUFFIX, "rx_bytes", NULL
        );

        self->priv->start_rx += get_bytes (self, filename);
    }
}

/**
 * network_manager_stop_modem_monitoring:
 *
 * Stop monitoring modem
 *
 * @param #NetworkManager
 *
 */
void
network_manager_stop_modem_monitoring  (NetworkManager *self)
{
    GDBusProxy *network_device_proxy;

    self->priv->end_rx = 0;
    self->priv->end_timestamp = g_get_monotonic_time();

    GFOREACH (self->priv->devices, network_device_proxy) {
        g_autofree gchar *interface;
        g_autofree gchar *filename;

        interface = get_hw_interface (self, network_device_proxy);

        if (interface == NULL)
            continue;

        filename = g_build_filename (
            SYSDIR_PREFIX, interface, SYSDIR_SUFFIX, "rx_bytes", NULL
        );

        self->priv->end_rx += get_bytes (self, filename);
    }
}

/**
 * network_manager_get_modem_usage:
 *
 * Get modem Usage
 *
 * @param #NetworkManager
 *
 * Returns: #ModemUsage
 */
ModemUsage
network_manager_get_modem_usage (NetworkManager *self)
{
    gint64 delta_time = self->priv->end_timestamp - self->priv->start_timestamp;
    guint64 delta_rx = self->priv->end_rx - self->priv->start_rx;
    guint64 bandwidth = (guint64) delta_rx * 1000000 / delta_time;

    g_debug ("modem bandwidth: %lu", bandwidth);

    /* Should match a file download */
    if (bandwidth > 1000000)
        return MODEM_USAGE_HIGH;

    /* Should match an audio streaming */
    if (bandwidth > 1000)
        return MODEM_USAGE_MEDIUM;

    return MODEM_USAGE_LOW;
}