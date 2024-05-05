/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bus.h"
#include "cpufreq.h"
#include "devfreq.h"
#include "kernel_settings.h"
#include "logind.h"
#include "manager.h"

struct _ManagerPrivate {
    Cpufreq *cpufreq;
    Devfreq *devfreq;
    KernelSettings *kernel_settings;

    gboolean screen_off_power_saving;
};


G_DEFINE_TYPE_WITH_CODE (
    Manager,
    manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Manager)
)

static void
on_screen_on (Logind  *logind, gboolean screen_on, gpointer user_data) {
    Manager *self = MANAGER (user_data);

    if (self->priv->screen_off_power_saving) {
        cpufreq_set_powersave (self->priv->cpufreq, !screen_on);
        devfreq_set_powersave (self->priv->devfreq, !screen_on);
        kernel_settings_set_powersave (self->priv->kernel_settings, !screen_on);
    }
}


static void
on_power_saving_mode_changed (Bus  *bus,
                              const gchar* governor,
                              gpointer user_data) {
    Manager *self = MANAGER (user_data);

    cpufreq_set_governor (self->priv->cpufreq, governor);
    devfreq_set_governor (self->priv->devfreq, governor);
}


static void
on_screen_off_power_saving_changed (Bus  *bus,
                                    gboolean screen_off_power_saving,
                                    gpointer user_data) {
    Manager *self = MANAGER (user_data);

    self->priv->screen_off_power_saving = screen_off_power_saving;

    if (!self->priv->screen_off_power_saving) {
        cpufreq_set_powersave (self->priv->cpufreq, FALSE);
        devfreq_set_powersave (self->priv->devfreq, FALSE);
    }
}

static void
manager_dispose (GObject *manager)
{
    Manager *self = MANAGER (manager);

    g_clear_object (&self->priv->cpufreq);
    g_clear_object (&self->priv->devfreq);
    g_clear_object (&self->priv->kernel_settings);
    g_free (self->priv);

    G_OBJECT_CLASS (manager_parent_class)->dispose (manager);
}

static void
manager_class_init (ManagerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = manager_dispose;
}

static void
manager_init (Manager *self)
{
    self->priv = manager_get_instance_private (self);

    self->priv->cpufreq = CPUFREQ (cpufreq_new ());
    self->priv->devfreq = DEVFREQ (devfreq_new ());
    self->priv->kernel_settings = KERNEL_SETTINGS (kernel_settings_new ());

    self->priv->screen_off_power_saving = TRUE;

    g_signal_connect (
        logind_get_default (),
        "screen-on",
        G_CALLBACK (on_screen_on),
        self
    );

    g_signal_connect (
        bus_get_default (),
        "power-saving-mode-changed",
        G_CALLBACK (on_power_saving_mode_changed),
        self
    );

    g_signal_connect (
        bus_get_default (),
        "screen-off-power-saving-changed",
        G_CALLBACK (on_screen_off_power_saving_changed),
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
