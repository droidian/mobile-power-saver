/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "dozing.h"
#include "modem_ofono_device.h"
#include "../common/define.h"
#include "../common/utils.h"

#define OFONO_DBUS_NAME                           "org.ofono"
#define OFONO_MODEM_DBUS_INTERFACE                "org.ofono.Modem"
#define OFONO_VOICE_CALL_MANAGER_DBUS_INTERFACE   "org.ofono.VoiceCallManager"

/* props */
enum {
    PROP_0,
    PROP_DEVICE_PATH
};

struct _ModemOfonoDevicePrivate {
    GDBusProxy *modem_ofono_device_modem_proxy;
    GDBusProxy *modem_ofono_voice_call_proxy;

    char *device_path;
};

G_DEFINE_TYPE_WITH_CODE (
    ModemOfonoDevice,
    modem_ofono_device,
    TYPE_MODEM,
    G_ADD_PRIVATE (ModemOfonoDevice)
)

static void
on_proxy_signal (GDBusProxy *proxy,
                 const char *sender_name,
                 const char *signal_name,
                 GVariant   *parameters,
                 gpointer    user_data);

static void
init_voice_call_interface (ModemOfonoDevice *self)
{
    g_autoptr (GError) error = NULL;

    self->priv->modem_ofono_voice_call_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        OFONO_DBUS_NAME,
        self->priv->device_path,
        OFONO_VOICE_CALL_MANAGER_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning ("Can't connect to OFono voice call: %s", error->message);
        return;
    }

    g_signal_connect (
        self->priv->modem_ofono_voice_call_proxy,
        "g-signal",
        G_CALLBACK (on_proxy_signal),
        self
    );
}

static void
on_proxy_signal (GDBusProxy *proxy,
                 const char *sender_name,
                 const char *signal_name,
                 GVariant   *parameters,
                 gpointer    user_data)
{
    ModemOfonoDevice *self = MODEM_OFONO_DEVICE (user_data);

    if (g_strcmp0 (signal_name, "CallAdded") == 0) {
        dozing_stop (dozing_get_default());
    } else if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
        const char *name;
        g_autoptr (GVariant) value = NULL;

        g_variant_get (parameters, "(&sv)", &name, &value);

        if (g_strcmp0 (name, "Interfaces") == 0) {
            g_autoptr (GVariantIter) inner_iter = NULL;
            const char *inner_name = NULL;

            g_variant_get (value, "as", &inner_iter);
            while (g_variant_iter_loop (inner_iter, "&s", &inner_name)) {
                if (g_strcmp0 (inner_name,
                               OFONO_VOICE_CALL_MANAGER_DBUS_INTERFACE) == 0) {
                    init_voice_call_interface (self);
                    break;
                }
            }
        }
    }
}

static void
modem_ofono_device_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    ModemOfonoDevice *self = MODEM_OFONO_DEVICE (object);

    switch (property_id) {
        case PROP_DEVICE_PATH:
            self->priv->device_path = g_strdup (g_value_get_string (value));
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
modem_ofono_device_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ModemOfonoDevice *self = MODEM_OFONO_DEVICE (object);

    switch (property_id) {
        case PROP_DEVICE_PATH:
            g_value_set_string (
                value, self->priv->device_path);
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
modem_ofono_device_constructed (GObject *modem_ofono_device)
{
    ModemOfonoDevice *self = MODEM_OFONO_DEVICE (modem_ofono_device);
    g_autoptr (GVariantIter) iter = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GVariant) property_value = NULL;
    const char *property_name = NULL;

    self->priv->modem_ofono_device_modem_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        OFONO_DBUS_NAME,
        self->priv->device_path,
        OFONO_MODEM_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning ("Can't connect to OFono modem interface: %s", error->message);
        return;
    }

    g_signal_connect (
        self->priv->modem_ofono_device_modem_proxy,
        "g-signal",
        G_CALLBACK (on_proxy_signal),
        self
    );

    value = g_dbus_proxy_call_sync (
        self->priv->modem_ofono_device_modem_proxy,
        "GetProperties",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
       -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning ("Can't get modem online status: %s", error->message);
        return;
    }

    g_variant_get (value, "(a{sv})", &iter);
    while (g_variant_iter_loop (iter, "{&sv}", &property_name, &property_value)) {
        if (g_strcmp0 (property_name, "Interfaces") == 0) {
            on_proxy_signal (
                self->priv->modem_ofono_device_modem_proxy,
                NULL,
                "PropertyChanged",
                g_variant_new ("(sv)", property_name, property_value),
                self
            );
        }
    }

    G_OBJECT_CLASS (modem_ofono_device_parent_class)->constructed (modem_ofono_device);
}

static void
modem_ofono_device_dispose (GObject *modem_ofono_device)
{
    ModemOfonoDevice *self = MODEM_OFONO_DEVICE (modem_ofono_device);

    g_clear_object (&self->priv->modem_ofono_device_modem_proxy);
    g_clear_object (&self->priv->modem_ofono_voice_call_proxy);

    G_OBJECT_CLASS (modem_ofono_device_parent_class)->dispose (modem_ofono_device);
}

static void
modem_ofono_device_finalize (GObject *modem_ofono_device)
{
    ModemOfonoDevice *self = MODEM_OFONO_DEVICE (modem_ofono_device);

    g_free (self->priv->device_path);

    G_OBJECT_CLASS (modem_ofono_device_parent_class)->finalize (modem_ofono_device);
}

static void
modem_ofono_device_class_init (ModemOfonoDeviceClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->constructed = modem_ofono_device_constructed;
    object_class->dispose = modem_ofono_device_dispose;
    object_class->finalize = modem_ofono_device_finalize;
    object_class->set_property = modem_ofono_device_set_property;
    object_class->get_property = modem_ofono_device_get_property;

    g_object_class_install_property (
        object_class,
        PROP_DEVICE_PATH,
        g_param_spec_string (
            "device-path",
            "Modem path",
            "Modem path",
            "",
            G_PARAM_WRITABLE |
            G_PARAM_CONSTRUCT_ONLY
        )
    );
}

static void
modem_ofono_device_init (ModemOfonoDevice *self)
{
    self->priv = modem_ofono_device_get_instance_private (self);

    self->priv->modem_ofono_device_modem_proxy = NULL;
    self->priv->modem_ofono_voice_call_proxy = NULL;
}

/**
 * modem_ofono_device_new:
 *
 * Creates a new #ModemOfonoDevice
 *
 * @param path: modem path
 *
 * Returns: (transfer full): a new #ModemOfonoDevice
 *
 **/
GObject *
modem_ofono_device_new (const char *path)
{
    GObject *modem_ofono_device;

    modem_ofono_device = g_object_new (
        TYPE_MODEM_OFONO_DEVICE, "device-path", path, NULL
    );

    return modem_ofono_device;
}

/**
 * modem_ofono_device_get_path:
 *
 * Get device path
 *
 * @param self: #ModemOfonoDevice
 *
 * Returns: device path as string
 *
 **/
const char*
modem_ofono_device_get_path (ModemOfonoDevice *self)
{
    return self->priv->device_path;
}