/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "network_manager.h"
#include "modem_ofono.h"
#include "modem_ofono_device.h"
#include "../common/utils.h"

#define OFONO_DBUS_NAME                     "org.ofono"
#define OFONO_DBUS_PATH                     "/"
#define OFONO_MANAGER_DBUS_INTERFACE        "org.ofono.Manager"
#define OFONO_MODEM_DBUS_INTERFACE          "org.ofono.Modem"
#define OFONO_RADIO_SETTINGS_DBUS_INTERFACE "org.ofono.RadioSettings"

struct _ModemOfonoPrivate {
    GDBusProxy *modem_ofono_manager_proxy;

    GList *modems;
};

G_DEFINE_TYPE_WITH_CODE (
    ModemOfono,
    modem_ofono,
    TYPE_MODEM,
    G_ADD_PRIVATE (ModemOfono)
)

static void
on_modem_ofono_device_ready (ModemOfonoDevice      *device,
                             gpointer  user_data);

static void
add_modem (ModemOfono  *self,
           const gchar *path)
{
    ModemOfonoDevice *device = MODEM_OFONO_DEVICE (modem_ofono_device_new (path));

    self->priv->modems = g_list_append (self->priv->modems, device);
    g_signal_connect (
        device,
        "device-ready",
        G_CALLBACK (on_modem_ofono_device_ready),
        self
    );
}

static void
del_modem (ModemOfono  *self,
           const gchar *path)
{
    ModemOfonoDevice *device;

    GFOREACH (self->priv->modems, device) {
        if (g_strcmp0 (modem_ofono_device_get_path (device), path) == 0) {
            self->priv->modems = g_list_remove (self->priv->modems, device);
            break;
        }
    }
}

static void
on_modem_ofono_device_ready (ModemOfonoDevice *device,
                             gpointer          user_data)
{
    ModemOfono *self = MODEM_OFONO (user_data);
    gboolean powersave = (
        modem_get_powersave (MODEM (self)) & MODEM_POWERSAVE_ENABLED
    ) == MODEM_POWERSAVE_ENABLED;

    modem_ofono_device_apply_powersave (device, powersave);
}

static void
on_modem_ofono_manager_signal (GDBusProxy  *proxy,
                               const gchar *sender_name,
                               const gchar *signal_name,
                               GVariant    *parameters,
                               gpointer     user_data)
{
    ModemOfono *self = MODEM_OFONO (user_data);

    if (g_strcmp0 (signal_name, "ModemAdded") == 0) {
        const gchar *modem;

        g_variant_get (parameters, "(&oa{sv})", &modem, NULL);
        add_modem (self, modem);
    } else if (g_strcmp0 (signal_name, "ModemRemoved") == 0) {
        const gchar *modem;

        g_variant_get (parameters, "(&o)", &modem);
        del_modem (self, modem);
    }
}

static void
modem_ofono_apply_powersave (Modem *self)
{
    ModemOfono *this = MODEM_OFONO (self);
    ModemOfonoDevice *device;
    gboolean powersave = (
        modem_get_powersave (MODEM (self)) & MODEM_POWERSAVE_ENABLED
    ) == MODEM_POWERSAVE_ENABLED;

    GFOREACH (this->priv->modems, device) {
        modem_ofono_device_apply_powersave (device, powersave);
    }
}

static void
modem_ofono_dispose (GObject *modem_ofono)
{
    ModemOfono *self = MODEM_OFONO (modem_ofono);

    g_clear_object (&self->priv->modem_ofono_manager_proxy);

    G_OBJECT_CLASS (modem_ofono_parent_class)->dispose (modem_ofono);
}

static void
modem_ofono_finalize (GObject *modem_ofono)
{
    ModemOfono *self = MODEM_OFONO (modem_ofono);

    g_list_free_full (self->priv->modems, (GDestroyNotify) g_object_unref);

    G_OBJECT_CLASS (modem_ofono_parent_class)->finalize (modem_ofono);
}

static void
modem_ofono_class_init (ModemOfonoClass *klass)
{
    GObjectClass *object_class;
    ModemClass *modem_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = modem_ofono_dispose;
    object_class->finalize = modem_ofono_finalize;

    modem_class = MODEM_CLASS (klass);
    modem_class->apply_powersave = modem_ofono_apply_powersave;
}

static void
modem_ofono_init (ModemOfono *self)
{
    g_autoptr (GVariantIter) iter = NULL;
    g_autoptr (GVariant) value = NULL;
    g_autoptr (GError) error = NULL;
    const gchar *modem;

    self->priv = modem_ofono_get_instance_private (self);

    self->priv->modems = NULL;

    self->priv->modem_ofono_manager_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        OFONO_DBUS_NAME,
        OFONO_DBUS_PATH,
        OFONO_MANAGER_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (self->priv->modem_ofono_manager_proxy == NULL)
        g_error("Can't connect to modem_ofono manager: %s", error->message);

    g_signal_connect_after (
        self->priv->modem_ofono_manager_proxy,
        "g-signal",
        G_CALLBACK (on_modem_ofono_manager_signal),
        self
    );

    value = g_dbus_proxy_call_sync (
        self->priv->modem_ofono_manager_proxy,
        "GetModems",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning ("Can't get modem_ofono modems: %s", error->message);
        return;
    }

    g_variant_get (value, "(a(oa{sv}))", &iter);
    while (g_variant_iter_loop (iter, "(&oa{sv})", &modem, NULL)) {
        add_modem (self, modem);
    }
}

/**
 * modem_ofono_new:
 *
 * Creates a new #ModemOfono
 *
 * Returns: (transfer full): a new #ModemOfono
 *
 **/
GObject *
modem_ofono_new (void)
{
    GObject *modem_ofono;

    modem_ofono = g_object_new (TYPE_MODEM_OFONO, NULL);

    return modem_ofono;
}