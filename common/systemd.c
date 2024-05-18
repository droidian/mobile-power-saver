/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bus.h"
#include "systemd.h"
#include "../common/utils.h"

#define SYSTEMD_DBUS_NAME       "org.freedesktop.systemd1"
#define SYSTEMD_DBUS_PATH       "/org/freedesktop/systemd1"
#define SYSTEMD_DBUS_INTERFACE  "org.freedesktop.systemd1.Manager"


struct _SystemdPrivate {
    GDBusProxy *systemd_proxy;
};


G_DEFINE_TYPE_WITH_CODE (
    Systemd,
    systemd,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Systemd)
)


static void
systemd_init_bus_proxy (Systemd *self, GBusType bus_type) {
    g_autoptr (GError) error = NULL;

    self->priv->systemd_proxy = g_dbus_proxy_new_for_bus_sync (
        bus_type,
        0,
        NULL,
        SYSTEMD_DBUS_NAME,
        SYSTEMD_DBUS_PATH,
        SYSTEMD_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (self->priv->systemd_proxy == NULL)
        g_error("Can't contact Systemd: %s", error->message);
}


static void
systemd_dispose (GObject *systemd)
{
    Systemd *self = SYSTEMD (systemd);

    g_clear_object (&self->priv->systemd_proxy);

    G_OBJECT_CLASS (systemd_parent_class)->dispose (systemd);
}


static void
systemd_finalize (GObject *systemd)
{
    G_OBJECT_CLASS (systemd_parent_class)->finalize (systemd);
}


static void
systemd_class_init (SystemdClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = systemd_dispose;
    object_class->finalize = systemd_finalize;
}


static void
systemd_init (Systemd *self)
{
    self->priv = systemd_get_instance_private (self);
}


/**
 * systemd_new:
 *
 * Creates a new #Systemd
 *
 * @param #GBusType: target bus type
 *
 * Returns: (transfer full): a new #Systemd
 *
 **/
GObject *
systemd_new (GBusType bus_type)
{
    GObject *systemd;

    systemd = g_object_new (TYPE_SYSTEMD, NULL);

    systemd_init_bus_proxy (SYSTEMD (systemd), bus_type);

    return systemd;
}


/**
 * systemd_start:
 *
 * Start service
 *
 * @param #Systemd: target bus type
 * @param services: services to start
 *
 **/
void
systemd_start (Systemd *self,
               GList   *services)
{
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    const gchar *service;

    if (services == NULL)
        return;

    GFOREACH (services, service) {
        g_message ("start: %s", service);
        result = g_dbus_proxy_call_sync (
            self->priv->systemd_proxy,
            "StartUnit",
            g_variant_new ("(&s&s)", service, "replace"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error
        );

        if (error != NULL)
            g_warning (
                "Error starting service: %s %s", service, error->message
            );
    }
}


/**
 * systemd_stop:
 *
 * Stop service
 *
 * @param #Systemd: target bus type
 * @param services: services to stop
 *
 **/
void
systemd_stop (Systemd *self,
              GList   *services)
{
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    const gchar *service;
g_message("STOP!!!!!!!!!!!!!");
    if (services == NULL)
        return;

    GFOREACH (services, service) {
        g_message ("stop: %s", service);
        result = g_dbus_proxy_call_sync (
            self->priv->systemd_proxy,
            "StopUnit",
            g_variant_new ("(&s&s)", service, "replace"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error
        );

        if (error != NULL)
            g_warning (
                "Error stopping service: %s %s", service, error->message
            );
    }
}