/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bus.h"
#include "cpufreq.h"
#include "config.h"
#include "devfreq.h"
#include "processes.h"
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
    Devfreq *devfreq;
    KernelSettings *kernel_settings;
    Processes *processes;
    Services *services;
#ifdef WIFI_ENABLED
    WiFi *wifi;
#endif

    gboolean screen_off_power_saving;
    gboolean suspend_services;
    gboolean suspend_bluetooth;

    GList *suspend_processes;
    GList *cpuset_background_processes;
    GList *suspend_system_services_blacklist;
    GList *suspend_bluetooth_services;

    char *cgroups_user_dir;

    gboolean radio_power_saving;
};

G_DEFINE_TYPE_WITH_CODE (
    Manager,
    manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Manager)
)

static const char *
get_governor_from_power_profile (PowerProfile power_profile) {
    if (power_profile == POWER_PROFILE_POWER_SAVER)
        return "powersave";
    if (power_profile == POWER_PROFILE_PERFORMANCE)
        return "performance";
    return NULL;
}

static void
on_screen_state_changed (Logind logind,
                         gboolean screen_on,
                         gpointer user_data)
{
    Manager *self = MANAGER (user_data);
    GList *system_services = get_cgroup_services (CGROUPS_SYSTEM_SERVICES_DIR);
    GList *user_slices = get_cgroup_slices (self->priv->cgroups_user_dir);
    GList *user_services = NULL;
    const char *slice;

    GFOREACH (user_slices, slice) {
        GList *services = get_cgroup_services (slice);

        user_services = g_list_concat (user_services, services);
    }

    if (self->priv->screen_off_power_saving) {
        bus_screen_state_changed (bus_get_default (), screen_on);

        devfreq_set_powersave (self->priv->devfreq, !screen_on);
        kernel_settings_set_powersave (self->priv->kernel_settings, !screen_on);

#ifdef WIFI_ENABLED
        if (self->priv->radio_power_saving)
            wifi_set_powersave (self->priv->wifi, !screen_on);
#endif
        if (screen_on) {
            cpufreq_set_powersave (self->priv->cpufreq, FALSE, TRUE);
            processes_set_cpuset (
                self->priv->processes,
                self->priv->cpuset_background_processes,
                CPUSET_SYSTEM_BACKGROUND
            );
            processes_set_services_cpuset (
                self->priv->processes,
                CGROUPS_SYSTEM_SERVICES_DIR,
                system_services,
                CPUSET_SYSTEM_BACKGROUND
            );
            processes_set_services_cpuset (
                self->priv->processes,
                self->priv->cgroups_user_dir,
                user_services,
                CPUSET_FOREGROUND
            );
        } else {
            cpufreq_set_powersave (self->priv->cpufreq, TRUE, FALSE);
            processes_update (self->priv->processes);
            processes_set_cpuset (
                self->priv->processes,
                self->priv->cpuset_background_processes,
                CPUSET_BACKGROUND
            );
            processes_set_services_cpuset (
                self->priv->processes,
                CGROUPS_SYSTEM_SERVICES_DIR,
                system_services,
                CPUSET_BACKGROUND
            );
            processes_set_services_cpuset (
                self->priv->processes,
                self->priv->cgroups_user_dir,
                user_services,
                CPUSET_SYSTEM_BACKGROUND
            );
        }
    }

    g_list_free_full (system_services, g_free);
    g_list_free_full (user_slices, g_free);
    g_list_free_full (user_services, g_free);
}

static void
set_power_profile (Manager      *self,
                   PowerProfile  power_profile)
{
    const char *governor = get_governor_from_power_profile (power_profile);

    cpufreq_set_governor (self->priv->cpufreq, governor);
    devfreq_set_governor (self->priv->devfreq, governor);
}

static void
set_cgroups_user_dir (Manager  *self,
                      GVariant *value)
{
    if (self->priv->cgroups_user_dir != NULL) {
        g_free (self->priv->cgroups_user_dir);
    }

    g_variant_get (value, "s", &self->priv->cgroups_user_dir);
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

    if (g_strcmp0 (setting, "power-saving-mode") == 0) {
        gint power_profile = g_variant_get_int32 (inner_value);
        set_power_profile (self, power_profile);
    } else if (g_strcmp0 (setting, "screen-off-power-saving") == 0) {
        self->priv->screen_off_power_saving = g_variant_get_boolean (inner_value);

        if (!self->priv->screen_off_power_saving) {
            cpufreq_set_powersave (self->priv->cpufreq, FALSE, TRUE);
            devfreq_set_powersave (self->priv->devfreq, FALSE);
        }
    } else if (g_strcmp0 (setting, "cpuset-background-processes") == 0) {
        g_list_free_full (
            self->priv->cpuset_background_processes, g_free
        );
        self->priv->cpuset_background_processes = get_list_from_variant (
            inner_value
        );
    } else if (g_strcmp0 (setting, "suspend-system-services-blacklist") == 0) {
        g_list_free_full (
            self->priv->suspend_system_services_blacklist, g_free
        );
        self->priv->suspend_system_services_blacklist = get_list_from_variant (
            inner_value
        );
    } else if (g_strcmp0 (setting, "devfreq-blacklist") == 0) {
        GList *list = get_list_from_variant (inner_value);
        const char *device;

        GFOREACH (list, device) {
            devfreq_blacklist (self->priv->devfreq, device);
        }

        g_list_free_full (list, g_free);
    } else if (g_strcmp0 (setting, "cpuset-blacklist") == 0) {
        GList *list = get_list_from_variant (inner_value);

        processes_cpuset_set_blacklist (self->priv->processes, list);
    } else if (g_strcmp0 (setting, "cpuset-topapp") == 0) {
        GList *list = get_list_from_variant (inner_value);

        processes_cpuset_set_topapp (self->priv->processes, list);
    } else if (g_strcmp0 (setting, "cgroups-user-dir") == 0) {
        set_cgroups_user_dir (self, inner_value);
    } else if (g_strcmp0 (setting, "little-cluster-powersave") == 0) {
        gboolean enabled = g_variant_get_boolean (inner_value);

        cpufreq_set_powersave (self->priv->cpufreq, TRUE, enabled);
    } else if (g_strcmp0 (setting, "radio-power-saving") == 0) {
        self->priv->radio_power_saving = g_variant_get_boolean (inner_value);
    } else if (g_strcmp0 (setting, "dozing") == 0) {
        gboolean dozing = g_variant_get_boolean (inner_value);

        if (self->priv->suspend_services) {
            GList *blacklist = g_list_copy_deep (
                self->priv->suspend_system_services_blacklist,
                (GCopyFunc) g_strdup,
                NULL
            );
            const char *service;

            GFOREACH (self->priv->suspend_bluetooth_services, service) {
                blacklist = g_list_prepend (blacklist, g_strdup (service));
            }

            if (dozing) {
                services_freeze_all (
                    self->priv->services,
                    blacklist
                );
                if (self->priv->suspend_bluetooth) {
                    services_freeze (
                        self->priv->services,
                        self->priv->suspend_bluetooth_services
                    );
                }
            } else {
                services_unfreeze_all (
                    self->priv->services,
                    blacklist
                );
                services_unfreeze (
                    self->priv->services,
                    self->priv->suspend_bluetooth_services
                );
            }

            g_list_free_full (blacklist, g_free);
        }

        if (dozing) {
            processes_suspend (
                self->priv->processes,
                self->priv->suspend_processes
            );
        } else {
            processes_resume (
                self->priv->processes,
                self->priv->suspend_processes
            );
        }
    } else if (g_strcmp0 (setting, "suspend-processes") == 0) {
        g_list_free_full (
            self->priv->suspend_processes, g_free
        );
        self->priv->suspend_processes = get_list_from_variant (
            inner_value
        );
    } else if (g_strcmp0 (setting, "suspend-bluetooth-services") == 0) {
        g_list_free_full (
            self->priv->suspend_bluetooth_services, g_free
        );
        self->priv->suspend_bluetooth_services = get_list_from_variant (
            inner_value
        );
    } else if (g_strcmp0 (setting, "suspend-bluetooth") == 0) {
        self->priv->suspend_bluetooth = g_variant_get_boolean (inner_value);
    } else if (g_strcmp0 (setting, "suspend-services") == 0) {
        self->priv->suspend_services = g_variant_get_boolean (inner_value);
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

    services_unfreeze_all (
        self->priv->services,
        self->priv->suspend_system_services_blacklist
    );
    services_unfreeze (
        self->priv->services,
        self->priv->suspend_bluetooth_services
    );

    wifi_set_powersave (self->priv->wifi, FALSE);

    g_clear_object (&self->priv->cpufreq);
    g_clear_object (&self->priv->devfreq);
    g_clear_object (&self->priv->kernel_settings);
    g_clear_object (&self->priv->processes);
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
        self->priv->suspend_processes, g_free
    );
    g_list_free_full (
        self->priv->cpuset_background_processes, g_free
    );
    g_list_free_full (
        self->priv->suspend_system_services_blacklist, g_free
    );
    g_list_free_full (
        self->priv->suspend_bluetooth_services, g_free
    );

    if (self->priv->cgroups_user_dir != NULL) {
        g_free (self->priv->cgroups_user_dir);
    }

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
    self->priv->devfreq = DEVFREQ (devfreq_new ());
    self->priv->kernel_settings = KERNEL_SETTINGS (kernel_settings_new ());
    self->priv->processes = PROCESSES (processes_new ());
    self->priv->services = SERVICES (services_new (G_BUS_TYPE_SYSTEM));
#ifdef WIFI_ENABLED
    self->priv->wifi = WIFI (wifi_new ());
#endif

    self->priv->screen_off_power_saving = TRUE;
    self->priv->suspend_services = FALSE;
    self->priv->suspend_bluetooth = FALSE;

    self->priv->radio_power_saving = FALSE;
    self->priv->suspend_processes = NULL;
    self->priv->cgroups_user_dir = NULL;
    self->priv->suspend_system_services_blacklist = NULL;
    self->priv->cpuset_background_processes = NULL;
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
