/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "network_manager_modem.h"
#include "../common/define.h"
#include "../common/utils.h"

#define NETWORK_MANAGER_DBUS_NAME             "org.freedesktop.NetworkManager"
#define NETWORK_MANAGER_DBUS_PATH             "/org/freedesktop/NetworkManager"
#define NETWORK_MANAGER_DBUS_INTERFACE        "org.freedesktop.NetworkManager"
#define NETWORK_MANAGER_DBUS_DEVICE_INTERFACE "org.freedesktop.NetworkManager.Device"

#define SYSDIR_PREFIX                         "/sys/class/net"
#define SYSDIR_SUFFIX                         "statistics"

#define BANDWIDTH_THRESHOLD                   10000

struct _NetworkManagerModemPrivate {
    GDBusProxy *network_manager_modem_proxy;

    GList *modem_devices;

    gint64 start_timestamp;
    gint64 end_timestamp;

    guint64  start_modem_rx;
    guint64  end_modem_rx;
};

G_DEFINE_TYPE_WITH_CODE (
    NetworkManagerModem,
    network_manager_modem,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (NetworkManagerModem)
)

static guint64
get_bytes (NetworkManagerModem *self,
           const char     *path)
{
    g_autofree char *contents = NULL;

    if (g_file_get_contents (path, &contents, NULL, NULL))
        return g_ascii_strtoll (contents, NULL, 0);

    return 0;
}

static char*
get_hw_interface (NetworkManagerModem *self,
                  GDBusProxy     *network_device_proxy)
{
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GVariant) inner_value = NULL;
    g_autoptr (GError) error = NULL;
    char *interface;

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
add_device (NetworkManagerModem *self,
            const char     *device_path)
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
        g_warning ("Can't get network device: %s", error->message);
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
        g_warning (
            "Can't read DeviceType: %s",
            error->message
        );
        g_clear_object (&network_device_proxy);
        return;
    }

    g_variant_get (value, "(v)", &inner_value);
    g_variant_get (inner_value, "u", &device_type);

    if (device_type == 8) { /* NM_DEVICE_TYPE_MODEM */
        self->priv->modem_devices = g_list_append (
            self->priv->modem_devices, network_device_proxy
        );
    } else {
        g_clear_object (&network_device_proxy);
    }
}

static void
del_device (NetworkManagerModem *self,
            const char     *device_path)
{
    GDBusProxy *network_device_proxy;

    GFOREACH (self->priv->modem_devices, network_device_proxy) {
        const char *object_path = g_dbus_proxy_get_object_path (
            network_device_proxy
        );
        if (g_strcmp0 (object_path, device_path) == 0) {
            self->priv->modem_devices = g_list_remove (
                self->priv->modem_devices, network_device_proxy
            );
            g_clear_object (&network_device_proxy);
            break;
        }
    }
}

static void
on_network_manager_modem_proxy_signal (GDBusProxy *proxy,
                                 const char *sender_name,
                                 const char *signal_name,
                                 GVariant   *parameters,
                                 gpointer    user_data)
{
    NetworkManagerModem *self = user_data;
    const char *object_path = NULL;

    if (g_strcmp0 (signal_name, "DeviceAdded") == 0) {
        g_variant_get (parameters, "(&o)", &object_path);
        add_device (self, object_path);
    } else if (g_strcmp0 (signal_name, "DeviceRemoved") == 0) {
        g_variant_get (parameters, "(&o)", &object_path);
        del_device (self, object_path);
    }
}

static void
network_manager_modem_dispose (GObject *network_manager_modem)
{
    NetworkManagerModem *self = NETWORK_MANAGER_MODEM (network_manager_modem);

    g_clear_object (&self->priv->network_manager_modem_proxy);

    G_OBJECT_CLASS (network_manager_modem_parent_class)->dispose (network_manager_modem);
}

static void
network_manager_modem_finalize (GObject *network_manager_modem)
{
    NetworkManagerModem *self = NETWORK_MANAGER_MODEM (network_manager_modem);

    g_list_free (self->priv->modem_devices);

    G_OBJECT_CLASS (network_manager_modem_parent_class)->finalize (network_manager_modem);
}

static void
network_manager_modem_class_init (NetworkManagerModemClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = network_manager_modem_dispose;
    object_class->finalize = network_manager_modem_finalize;
}

static void
network_manager_modem_init (NetworkManagerModem *self)
{
    g_autoptr (GVariantIter) iter = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GError) error = NULL;
    const char *device_path = NULL;

    self->priv = network_manager_modem_get_instance_private (self);
    self->priv->modem_devices = NULL;

    self->priv->network_manager_modem_proxy = g_dbus_proxy_new_for_bus_sync (
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
        g_warning ("Can't contact NetworkManagerModem: %s", error->message);
        return;
    }

    g_signal_connect (
        self->priv->network_manager_modem_proxy,
        "g-signal",
        G_CALLBACK (on_network_manager_modem_proxy_signal),
        self
    );

    value = g_dbus_proxy_call_sync (
        self->priv->network_manager_modem_proxy,
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
 * network_manager_modem_new:
 *
 * Creates a new #NetworkManagerModem
 *
 * Returns: (transfer full): a new #NetworkManagerModem
 *
 **/
GObject *
network_manager_modem_new (void)
{
    GObject *network_manager_modem;

    network_manager_modem = g_object_new (TYPE_NETWORK_MANAGER_MODEM, NULL);

    return network_manager_modem;
}

/**
 * network_manager_modem_start_monitoring:
 *
 * Start monitoring modem
 *
 * @param #NetworkManagerModem
 *
 */
void
network_manager_modem_start_monitoring (NetworkManagerModem *self)
{
    GDBusProxy *network_device_proxy;

    self->priv->start_modem_rx = 0;
    self->priv->start_timestamp = g_get_monotonic_time();

    GFOREACH (self->priv->modem_devices, network_device_proxy) {
        g_autofree char *interface = get_hw_interface (self, network_device_proxy);
        g_autofree char *filename = g_build_filename (
            SYSDIR_PREFIX, interface, SYSDIR_SUFFIX, "rx_bytes", NULL
        );

        self->priv->start_modem_rx += get_bytes (self, filename);
    }
}

/**
 * network_manager_modem_stop_monitoring:
 *
 * Stop monitoring modem
 *
 * @param #NetworkManagerModem
 *
 */
void
network_manager_modem_stop_monitoring  (NetworkManagerModem *self)
{
    GDBusProxy *network_device_proxy;

    self->priv->end_modem_rx = 0;
    self->priv->end_timestamp = g_get_monotonic_time();

    GFOREACH (self->priv->modem_devices, network_device_proxy) {
        g_autofree char *interface = NULL;
        g_autofree char *filename = NULL;

        interface = get_hw_interface (self, network_device_proxy);

        if (interface == NULL)
            continue;

        filename = g_build_filename (
            SYSDIR_PREFIX, interface, SYSDIR_SUFFIX, "rx_bytes", NULL
        );

        self->priv->end_modem_rx += get_bytes (self, filename);
    }
}

/**
 * network_manager_modem_data_used:
 *
 * Get network data usage
 *
 * @param #NetworkManagerModem
 *
 * Returns: TRUE if date in use
 */
gboolean
network_manager_modem_data_used (NetworkManagerModem *self)
{
    guint64 bandwidth_modem = (
        (self->priv->end_modem_rx - self->priv->start_modem_rx) *
        1000000 /
        (self->priv->end_timestamp - self->priv->start_timestamp)
    );

    g_debug (
        "Network bandwidth: modem: %" G_GUINT64_FORMAT, bandwidth_modem
    );

    return bandwidth_modem > BANDWIDTH_THRESHOLD;
}