/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bus.h"
#include "cpufreq.h"
#include "cpuset.h"
#include "config.h"
#include "devfreq.h"
#include "kernel_settings.h"
#include "logind.h"
#include "manager.h"

#ifdef WIFI_ENABLED
#include "wifi.h"
#endif

#include "../common/define.h"
#include "../common/services.h"
#include "../common/utils.h"

struct _ManagerPrivate {
    Cpufreq *cpufreq;
    Cpuset  *cpuset;
    Devfreq *devfreq;
    KernelSettings *kernel_settings;
    Services *services;
#ifdef WIFI_ENABLED
    WiFi *wifi;
#endif

    gboolean screen_off_power_saving;

    GList *suspend_system_services;
    GList *suspend_bluetooth_services;

    gboolean radio_power_saving;
};

G_DEFINE_TYPE_WITH_CODE (
    Manager,
    manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Manager)
)

static void
on_screen_state_changed (Logind logind,
                         gboolean screen_on,
                         gpointer user_data)
{
    Manager *self = MANAGER (user_data);

    if (self->priv->screen_off_power_saving) {
        bus_screen_state_changed (bus_get_default (), screen_on);

        devfreq_set_powersave (self->priv->devfreq, !screen_on);
        kernel_settings_set_powersave (self->priv->kernel_settings, !screen_on);

#ifdef WIFI_ENABLED
        if (self->priv->radio_power_saving)
            wifi_set_powersave (self->priv->wifi, !screen_on);
#endif
        cpufreq_set_powersave (self->priv->cpufreq, !screen_on, screen_on);
        cpuset_set_powersave (self->priv->cpuset, !screen_on);
    }
}

static void
on_bus_setting_changed (Bus      *bus,
                        GVariant *value,
                        gpointer  user_data)
{
    Manager *self = MANAGER (user_data);
    const char *setting = NULL;
    g_autoptr (GVariant) inner_value = NULL;

    g_variant_get (value, "(&sv)", &setting, &inner_value);

    if (g_strcmp0 (setting, "screen-off-power-saving") == 0) {
        self->priv->screen_off_power_saving = g_variant_get_boolean (inner_value);

        if (!self->priv->screen_off_power_saving) {
            cpufreq_set_powersave (self->priv->cpufreq, FALSE, TRUE);
            devfreq_set_powersave (self->priv->devfreq, FALSE);
        }
    } else if (g_strcmp0 (setting, "suspend-system-services") == 0) {
        g_list_free_full (
            self->priv->suspend_system_services, g_free
        );
        self->priv->suspend_system_services = get_list_from_variant (
            inner_value
        );
    } else if (g_strcmp0 (setting, "devfreq-blacklist") == 0) {
        GList *list = get_list_from_variant (inner_value);
        const char *device;

        GFOREACH (list, device) {
            devfreq_blacklist (self->priv->devfreq, device);
        }

        g_list_free_full (list, g_free);
    } else if (g_strcmp0 (setting, "little-cluster-powersave") == 0) {
        gboolean enabled = g_variant_get_boolean (inner_value);

        cpufreq_set_powersave (self->priv->cpufreq, enabled, TRUE);
    } else if (g_strcmp0 (setting, "radio-power-saving") == 0) {
        self->priv->radio_power_saving = g_variant_get_boolean (inner_value);
    } else if (g_strcmp0 (setting, "dozing") == 0) {
        gboolean dozing = g_variant_get_boolean (inner_value);

        if (dozing) {
            services_freeze (
                self->priv->services,
                self->priv->suspend_system_services
            );
        } else {
            services_unfreeze (
                self->priv->services,
                self->priv->suspend_system_services
            );
        }
    } else if (g_strcmp0 (setting, "suspend-system-bluetooth-services") == 0) {
        g_list_free_full (
            self->priv->suspend_bluetooth_services, g_free
        );
        self->priv->suspend_bluetooth_services = get_list_from_variant (
            inner_value
        );
    } else if (g_strcmp0 (setting, "suspend-bluetooth-services") == 0) {
        gboolean suspend_bluetooth = g_variant_get_boolean (inner_value);

        if (suspend_bluetooth) {
            services_freeze (
                self->priv->services,
                self->priv->suspend_bluetooth_services
            );
        } else {
            services_unfreeze (
                self->priv->services,
                self->priv->suspend_bluetooth_services
            );
        }
    }
}

static void
manager_dispose (GObject *manager)
{
    Manager *self = MANAGER (manager);

    on_screen_state_changed (
        *logind_get_default (),
        TRUE,
        manager
    );

    services_unfreeze (
        self->priv->services,
        self->priv->suspend_system_services
    );
    services_unfreeze (
        self->priv->services,
        self->priv->suspend_bluetooth_services
    );

    cpuset_set_powersave (
        self->priv->cpuset,
        FALSE
    );

    wifi_set_powersave (self->priv->wifi, FALSE);

    g_clear_object (&self->priv->cpufreq);
    g_clear_object (&self->priv->cpuset);
    g_clear_object (&self->priv->devfreq);
    g_clear_object (&self->priv->kernel_settings);
    g_clear_object (&self->priv->services);
#ifdef WIFI_ENABLED
    g_clear_object (&self->priv->wifi);
#endif

    G_OBJECT_CLASS (manager_parent_class)->dispose (manager);
}

static void
manager_finalize (GObject *manager)
{
    Manager *self = MANAGER (manager);

    g_list_free_full (
        self->priv->suspend_system_services, g_free
    );
    g_list_free_full (
        self->priv->suspend_bluetooth_services, g_free
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

    self->priv->cpufreq = CPUFREQ (cpufreq_new ());
    self->priv->cpuset = CPUSET (cpuset_new ());
    self->priv->devfreq = DEVFREQ (devfreq_new ());
    self->priv->kernel_settings = KERNEL_SETTINGS (kernel_settings_new ());

    self->priv->services = SERVICES (services_new (G_BUS_TYPE_SYSTEM));

#ifdef WIFI_ENABLED
    self->priv->wifi = WIFI (wifi_new ());
#endif

    self->priv->screen_off_power_saving = FALSE;
    self->priv->radio_power_saving = FALSE;

    self->priv->suspend_system_services = NULL;
    self->priv->suspend_bluetooth_services = NULL;

    g_signal_connect (
        logind_get_default (),
        "screen-state-changed",
        G_CALLBACK (on_screen_state_changed),
        self
    );

    g_signal_connect (
        bus_get_default (),
        "bus-setting-changed",
        G_CALLBACK (on_bus_setting_changed),
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
