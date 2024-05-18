/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "bus.h"
#include "config.h"
#include "settings.h"

/* signals */
enum
{
    SETTING_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];



struct _SettingsPrivate {
    GSettings *settings;
};


G_DEFINE_TYPE_WITH_CODE (
    Settings,
    settings,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Settings)
)


static void
on_setting_changed (GSettings   *settings,
                    const gchar *key,
                    gpointer     user_data) {
    Settings *self = SETTINGS (user_data);
    g_autoptr(GVariant) value = g_settings_get_value (settings, key);

    g_signal_emit(
        self,
        signals[SETTING_CHANGED],
        0,
        key,
        value
    );
}


static gboolean
settings_notify_settings (Settings *self)
{
    on_setting_changed (
        self->priv->settings,
        "screen-off-power-saving",
        self
    );
    on_setting_changed (
        self->priv->settings,
        "screen-off-suspend-processes",
        self
    );
    on_setting_changed (
        self->priv->settings,
        "screen-off-suspend-system-services",
        self
    );
    return FALSE;
}


static void
settings_dispose (GObject *settings)
{
    Settings *self = SETTINGS (settings);

    g_clear_object (&self->priv->settings);

    G_OBJECT_CLASS (settings_parent_class)->dispose (settings);
}


static void
settings_finalize (GObject *settings)
{
    G_OBJECT_CLASS (settings_parent_class)->finalize (settings);
}

static void
settings_class_init (SettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = settings_dispose;
    object_class->finalize = settings_finalize;

    signals[SETTING_CHANGED] = g_signal_new (
        "setting-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        2,
        G_TYPE_STRING,
        G_TYPE_VARIANT
    );
}


static void
settings_init (Settings *self)
{
    self->priv = settings_get_instance_private (self);

    self->priv->settings = g_settings_new (APP_ID);

    g_signal_connect (
        self->priv->settings,
        "changed::screen-off-power-saving",
        G_CALLBACK (on_setting_changed),
        self
    );
    g_signal_connect (
        self->priv->settings,
        "changed::screen-off-suspend-processes",
        G_CALLBACK (on_setting_changed),
        self
    );
    g_signal_connect (
        self->priv->settings,
        "changed::screen-off-suspend-system-services",
        G_CALLBACK (on_setting_changed),
        self
    );

    g_idle_add ((GSourceFunc) settings_notify_settings, self);
}


/**
 * settings_new:
 *
 * Creates a new #Settings
 *
 * Returns: (transfer full): a new #Settings
 *
 **/
GObject *
settings_new (void)
{
    GObject *settings;

    settings = g_object_new (TYPE_SETTINGS, NULL);

    return settings;
}


static Settings *default_settings = NULL;
/**
 * settings_get_default:
 *
 * Gets the default #Settings.
 *
 * Return value: (transfer full): the default #Settings.
 */
Settings *
settings_get_default (void)
{
    if (!default_settings) {
        default_settings = SETTINGS (settings_new ());
    }
    return default_settings;
}

/**
 * settings_can_freeze_app:
 *
 * Check if an application scope can be freezed
 *
 * @self: a #Settings
 * @app_scope: a setting key
 *
 * Returns: TRUE if application scope can be freezed
 */
gboolean
settings_can_freeze_app (Settings *self,
                         const gchar *app_scope)
{
    g_autoptr(GVariant) value = g_settings_get_value (
        self->priv->settings, "screen-off-suspend-apps-blacklist"
    );
    g_autoptr (GVariantIter) iter;
    const gchar *application;

    g_variant_get (value, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &application)) {
        if (g_strrstr (app_scope, application) != NULL)
            return FALSE;
    }
    return TRUE;
}


/**
 * settings_get_suspend_services
 *
 * Get services those can be suspended
 *
 * @self: a #Settings
 * @app_scope: a setting key
 *
 * Return value: (transfer full): services list.
 */
GList *
settings_get_suspend_services (Settings *self)
{
    g_autoptr(GVariant) value = g_settings_get_value (
        self->priv->settings, "screen-off-suspend-user-services"
    );
    g_autoptr (GVariantIter) iter;
    const gchar *service;
    GList *services = NULL;

    g_variant_get (value, "as", &iter);
    while (g_variant_iter_loop (iter, "s", &service)) {
        services = g_list_append (services, g_strdup (service));
    }
    return services;
}