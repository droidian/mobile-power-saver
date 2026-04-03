/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <gio/gio.h>

#include "cpuset.h"
#include "../common/utils.h"

#define SYSTEMD_DBUS_NAME       "org.freedesktop.systemd1"
#define SYSTEMD_DBUS_PATH       "/org/freedesktop/systemd1"
#define SYSTEMD_DBUS_INTERFACE  "org.freedesktop.systemd1.Manager"

struct _CpusetPrivate {
    GDBusProxy *systemd_proxy;

    guint8      little_cpu_mask;
    guint8      all_cpu_mask;
};

G_DEFINE_TYPE_WITH_CODE (
    Cpuset,
    cpuset,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Cpuset)
)

static void
cpuset_slice_set_powersave (Cpuset   *self,
                            const char     *slice,
                            gboolean  powersave)
{
    g_autoptr (GVariant) value = NULL;
    GVariant *params = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GVariantBuilder) builder = g_variant_builder_new(
        G_VARIANT_TYPE("a(sv)")
    );

    GVariant *allowed_cpus =
        powersave ?
        bytes_from_mask (self->priv->little_cpu_mask) :
        bytes_from_mask (self->priv->all_cpu_mask);

    g_message (
        "Allowed CPUs for %s: %s", slice, g_variant_print (allowed_cpus, TRUE)
    );

    g_variant_builder_add(builder, "(sv)", "AllowedCPUs", allowed_cpus);

    params = g_variant_new(
        "(sba(sv))",
        slice,
        TRUE,
        builder
    );

    value = g_dbus_proxy_call_sync (
        self->priv->systemd_proxy,
        "SetUnitProperties",
        params,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error ("Can't set unit properties: %s", error->message);
    }
}

static void
cpuset_dispose (GObject *cpuset)
{
    Cpuset *self = CPUSET (cpuset);

    g_clear_object (&self->priv->systemd_proxy);

    G_OBJECT_CLASS (cpuset_parent_class)->dispose (cpuset);
}

static void
cpuset_finalize (GObject *cpuset)
{
    G_OBJECT_CLASS (cpuset_parent_class)->finalize (cpuset);
}

static void
cpuset_class_init (CpusetClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = cpuset_dispose;
    object_class->finalize = cpuset_finalize;
}

static void
cpuset_init (Cpuset *self)
{
    g_autoptr (GError) error = NULL;

    self->priv = cpuset_get_instance_private (self);

    self->priv->little_cpu_mask = get_little_cpu_mask ();
    self->priv->all_cpu_mask = get_all_cpu_mask ();

    self->priv->systemd_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        SYSTEMD_DBUS_NAME,
        SYSTEMD_DBUS_PATH,
        SYSTEMD_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_error ("Can't contact Systemd: %s", error->message);
    }
}

/**
 * cpuset_new:
 *
 * Creates a new #Cpuset
 *
 * Returns: (transfer full): a new #Cpuset
 *
 **/
GObject *
cpuset_new (void)
{
    GObject *cpuset;

    cpuset = g_object_new (TYPE_CPUSET, NULL);

    return cpuset;
}

/**
 * cpuset_set_powersave:
 *
 * Enable powersave by moving some cgroup to little cluster
 *
 * @param #Cpuset
 * @param powersave: True if we want to powersave
 *
 */
void
cpuset_set_powersave (Cpuset   *self,
                      gboolean  powersave)
{
    cpuset_slice_set_powersave (self, "system.slice", powersave);
    cpuset_slice_set_powersave (self, "user.slice", powersave);
}
