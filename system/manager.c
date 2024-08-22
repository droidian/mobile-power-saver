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
#include "freezer.h"
#include "kernel_settings.h"
#include "logind.h"
#include "manager.h"
#include "modem.h"
#ifdef MM_ENABLED
#include "modem_mm.h"
#else
#include "modem_ofono.h"
#endif
#include "network_manager.h"

#ifdef WIFI_ENABLED
#include "wifi.h"
#endif

#include "../common/define.h"
#include "../common/services.h"

struct _ManagerPrivate {
    Cpufreq *cpufreq;
    Devfreq *devfreq;
    KernelSettings *kernel_settings;
    Freezer *freezer;
    NetworkManager *network_manager;
    Modem  *modem;
    Services *services;
#ifdef WIFI_ENABLED
    WiFi *wifi;
#endif

    gboolean screen_off_power_saving;
    GList *screen_off_suspend_processes;
    GList *screen_off_suspend_services;

    gboolean radio_power_saving;
};

G_DEFINE_TYPE_WITH_CODE (
    Manager,
    manager,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Manager)
)

static const gchar *
get_governor_from_power_profile (PowerProfile power_profile) {
    if (power_profile == POWER_PROFILE_POWER_SAVER)
        return "powersave";
    if (power_profile == POWER_PROFILE_PERFORMANCE)
        return "performance";
    return NULL;
}

static void
on_screen_state_changed (gpointer ignore,
                         gboolean screen_on,
                         gpointer user_data)
{
    Manager *self = MANAGER (user_data);

    if (self->priv->screen_off_power_saving) {
        bus_screen_state_changed (bus_get_default (), screen_on);
        cpufreq_set_powersave (self->priv->cpufreq, !screen_on);
        devfreq_set_powersave (self->priv->devfreq, !screen_on);
        kernel_settings_set_powersave (self->priv->kernel_settings, !screen_on);
#ifdef WIFI_ENABLED
        if (self->priv->radio_power_saving)
            wifi_set_powersave (self->priv->wifi, !screen_on);
#endif

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
                              PowerProfile power_profile,
                              gpointer     user_data)
{
    Manager *self = MANAGER (user_data);
    const gchar *governor = get_governor_from_power_profile (power_profile);

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
    const gchar *process;

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
on_devfreq_blacklist_setted (Bus      *bus,
                             GVariant *value,
                             gpointer  user_data)
{
    Manager *self = MANAGER (user_data);
    g_autoptr (GVariantIter) iter;
    const gchar *device;

    g_variant_get (value, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &device)) {
        devfreq_blacklist (self->priv->devfreq, device);
    }
    g_variant_unref (value);
}

static void
on_radio_power_saving_changed (Bus      *bus,
                               gboolean  radio_power_saving,
                               gpointer  user_data)
{
    Manager *self = MANAGER (user_data);
    ModemClass *klass;

    klass = MODEM_GET_CLASS (self->priv->modem);

    self->priv->radio_power_saving = radio_power_saving;

    if (self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
    else
        klass->reset_powersave (self->priv->modem);
}

static void
on_radio_power_saving_blacklist_changed (Bus      *bus,
                                         gint      blacklist,
                                         gpointer  user_data)
{
    Manager *self = MANAGER (user_data);
    ModemClass *klass;

    klass = MODEM_GET_CLASS (self->priv->modem);

    klass->set_blacklist (self->priv->modem, blacklist);

    if (self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
    else
        klass->reset_powersave (self->priv->modem);
}

static void
on_suspend_modem_changed (NetworkManager *network_manager,
                          gboolean        enabled,
                          gpointer        user_data)
{
    Manager *self = MANAGER (user_data);
    ModemClass *klass;
    gboolean updated;

    /* Here we assume AP set with screen on/dozing off */
    if (network_manager_has_ap (self->priv->network_manager))
        return;

    klass = MODEM_GET_CLASS (self->priv->modem);

    updated = modem_set_powersave (
        self->priv->modem, enabled, MODEM_POWERSAVE_DOZING
    );

    if (updated && self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
}

static void
on_connection_type_wifi (NetworkManager *network_manager,
                         gboolean        enabled,
                         gpointer        user_data)
{
    Manager *self = MANAGER (user_data);
    ModemClass *klass;
    gboolean updated;

    klass = MODEM_GET_CLASS (self->priv->modem);

    updated = modem_set_powersave (
        self->priv->modem, enabled, MODEM_POWERSAVE_WIFI
    );

    if (updated && self->priv->radio_power_saving)
        klass->apply_powersave (self->priv->modem);
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

    g_clear_object (&self->priv->cpufreq);
    g_clear_object (&self->priv->devfreq);
    g_clear_object (&self->priv->kernel_settings);
    g_clear_object (&self->priv->freezer);
    g_clear_object (&self->priv->network_manager);
    g_clear_object (&self->priv->modem);
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

    self->priv->cpufreq = CPUFREQ (cpufreq_new ());
    self->priv->devfreq = DEVFREQ (devfreq_new ());
    self->priv->kernel_settings = KERNEL_SETTINGS (kernel_settings_new ());
    self->priv->freezer = FREEZER (freezer_new ());
    self->priv->network_manager = NETWORK_MANAGER (network_manager_new ());
#ifdef MM_ENABLED
    self->priv->modem = MODEM (modem_mm_new ());
#else
    self->priv->modem = MODEM (modem_ofono_new ());
#endif
    self->priv->services = SERVICES (services_new (G_BUS_TYPE_SYSTEM));
#ifdef WIFI_ENABLED
    self->priv->wifi = WIFI (wifi_new ());
#endif

    self->priv->screen_off_power_saving = TRUE;
    self->priv->radio_power_saving = FALSE;
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
    g_signal_connect (
        bus_get_default (),
        "devfreq-blacklist-setted",
        G_CALLBACK (on_devfreq_blacklist_setted),
        self
    );
    g_signal_connect (
        bus_get_default (),
        "suspend-modem-changed",
        G_CALLBACK (on_suspend_modem_changed),
        self
    );
    g_signal_connect (
        bus_get_default (),
        "radio-power-saving-changed",
        G_CALLBACK (on_radio_power_saving_changed),
        self
    );
    g_signal_connect (
        bus_get_default (),
        "radio-power-saving-blacklist-changed",
        G_CALLBACK (on_radio_power_saving_blacklist_changed),
        self
    );
    g_signal_connect (
        self->priv->network_manager,
        "connection-type-wifi",
        G_CALLBACK (on_connection_type_wifi),
        self
    );

    network_manager_check_wifi (self->priv->network_manager);
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
