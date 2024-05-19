/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "binder.h"
#include "bus.h"
#include "cpufreq.h"
#include "devfreq.h"
#include "freezer.h"
#include "kernel_settings.h"
#include "logind.h"
#include "manager.h"
#include "../common/services.h"


struct _ManagerPrivate {
    Binder *binder;
    Cpufreq *cpufreq;
    Devfreq *devfreq;
    KernelSettings *kernel_settings;
    Freezer *freezer;
    Services *services;

    gboolean screen_off_power_saving;
    GList *screen_off_suspend_processes;
    GList *screen_off_suspend_services;
};


G_DEFINE_TYPE_WITH_CODE (
    Manager,
    manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Manager)
)

static void
on_screen_state_changed (gpointer ignore,
                         gboolean screen_on,
                         gpointer user_data)
{
    Manager *self = MANAGER (user_data);

    if (self->priv->screen_off_power_saving) {
        bus_screen_state_changed (bus_get_default (), screen_on);

        binder_set_powersave (self->priv->binder, !screen_on);
        cpufreq_set_powersave (self->priv->cpufreq, !screen_on);
        devfreq_set_powersave (self->priv->devfreq, !screen_on);
        kernel_settings_set_powersave (self->priv->kernel_settings, !screen_on);

        if (screen_on) {
            freezer_resume_processes (
                self->priv->freezer,
                self->priv->screen_off_suspend_processes
            );
            services_unfreeze (
                self->priv->services,
                self->priv->screen_off_suspend_services
            );
        } else {
            freezer_suspend_processes (
                self->priv->freezer,
                self->priv->screen_off_suspend_processes
            );
            services_freeze (
                self->priv->services,
                self->priv->screen_off_suspend_services
            );
        }
    }
}


static void
on_power_saving_mode_changed (Bus         *bus,
                              const gchar *governor,
                              gpointer     user_data)
{
    Manager *self = MANAGER (user_data);

    cpufreq_set_governor (self->priv->cpufreq, governor);
    devfreq_set_governor (self->priv->devfreq, governor);
}


static void
on_screen_off_power_saving_changed (Bus      *bus,
                                    gboolean  screen_off_power_saving,
                                    gpointer  user_data)
{
    Manager *self = MANAGER (user_data);

    self->priv->screen_off_power_saving = screen_off_power_saving;

    if (!self->priv->screen_off_power_saving) {
        cpufreq_set_powersave (self->priv->cpufreq, FALSE);
        devfreq_set_powersave (self->priv->devfreq, FALSE);
    }
}

static void
on_screen_off_suspend_processes_changed (Bus      *bus,
                                         GVariant *value,
                                         gpointer  user_data)
{
    Manager *self = MANAGER (user_data);
    g_autoptr (GVariantIter) iter;
    gchar *process;

    g_list_free_full (
        self->priv->screen_off_suspend_processes, g_free
    );
    self->priv->screen_off_suspend_processes = NULL;

    g_variant_get (value, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &process)) {
        self->priv->screen_off_suspend_processes =
            g_list_append (
                self->priv->screen_off_suspend_processes, g_strdup (process)
            );
    }
    g_variant_unref (value);
}


static void
on_screen_off_suspend_services_changed (Bus      *bus,
                                        GVariant *value,
                                        gpointer  user_data)
{
    Manager *self = MANAGER (user_data);
    g_autoptr (GVariantIter) iter;
    gchar *service;

    g_list_free_full (
        self->priv->screen_off_suspend_services,
        g_free
    );
    self->priv->screen_off_suspend_services = NULL;

    g_variant_get (value, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &service)) {
        self->priv->screen_off_suspend_services =
            g_list_append (
                self->priv->screen_off_suspend_services, g_strdup (service)
            );
    }
    g_variant_unref (value);
}


static void
manager_dispose (GObject *manager)
{
    Manager *self = MANAGER (manager);

    g_clear_object (&self->priv->binder);
    g_clear_object (&self->priv->cpufreq);
    g_clear_object (&self->priv->devfreq);
    g_clear_object (&self->priv->kernel_settings);
    g_clear_object (&self->priv->freezer);
    g_clear_object (&self->priv->services);

    G_OBJECT_CLASS (manager_parent_class)->dispose (manager);
}


static void
manager_finalize (GObject *manager)
{
    Manager *self = MANAGER (manager);

    g_list_free_full (
        self->priv->screen_off_suspend_processes, g_free
    );
    g_list_free_full (
        self->priv->screen_off_suspend_services, g_free
    );
    G_OBJECT_CLASS (manager_parent_class)->finalize (manager);
}


static void
manager_class_init (ManagerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = manager_dispose;
    object_class->finalize = manager_finalize;
}

static void
manager_init (Manager *self)
{
    self->priv = manager_get_instance_private (self);

    self->priv->binder = BINDER (binder_new ());
    self->priv->cpufreq = CPUFREQ (cpufreq_new ());
    self->priv->devfreq = DEVFREQ (devfreq_new ());
    self->priv->kernel_settings = KERNEL_SETTINGS (kernel_settings_new ());
    self->priv->freezer = FREEZER (freezer_new ());
    self->priv->services = SERVICES (services_new (G_BUS_TYPE_SYSTEM));

    self->priv->screen_off_power_saving = TRUE;
    self->priv->screen_off_suspend_processes = NULL;

    g_signal_connect (
        logind_get_default (),
        "screen-state-changed",
        G_CALLBACK (on_screen_state_changed),
        self
    );

    g_signal_connect (
        bus_get_default (),
        "screen-state-changed",
        G_CALLBACK (on_screen_state_changed),
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
    g_signal_connect (
        bus_get_default (),
        "screen-off-suspend-processes-changed",
        G_CALLBACK (on_screen_off_suspend_processes_changed),
        self
    );
    g_signal_connect (
        bus_get_default (),
        "screen-off-suspend-services-changed",
        G_CALLBACK (on_screen_off_suspend_services_changed),
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
