/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bus.h"
#include "logind.h"
#include "../common/define.h"

#define LOGIND_DBUS_NAME       "org.freedesktop.login1"
#define LOGIND_DBUS_PATH       "/org/freedesktop/login1/seat/seat0"
#define LOGIND_DBUS_INTERFACE  "org.freedesktop.login1.Seat"
#define LOGIND_DBUS_SESSION_INTERFACE  "org.freedesktop.login1.Session"

/* signals */
enum
{
    SCREEN_STATE_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _LogindPrivate {
    GDBusProxy *logind_proxy;
};

G_DEFINE_TYPE_WITH_CODE (
    Logind,
    logind,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Logind)
)

static void
on_logind_proxy_properties (GDBusProxy  *proxy,
                            GVariant    *changed_properties,
                            char       **invalidated_properties,
                            gpointer     user_data)
{
    Logind *self = LOGIND (user_data);
    GVariant *value;
    char *property;
    GVariantIter i;

    g_variant_iter_init (&i, changed_properties);
    while (g_variant_iter_next (&i, "{&sv}", &property, &value)) {
        if (g_strcmp0 (property, "IdleHint") == 0) {
            gboolean idle_hint = g_variant_get_boolean (value);
            g_signal_emit(
                self,
                signals[SCREEN_STATE_CHANGED],
                0,
                !idle_hint
            );
        }

        g_variant_unref (value);
    }
}

static void
connect_logind (Logind *self)
{
    g_autoptr (GDBusProxy) properties = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr(GVariant) value = NULL;
    g_autoptr(GVariant) inner_value = NULL;
    const char *session_id;
    const char *session_path;

    properties = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        LOGIND_DBUS_NAME,
        LOGIND_DBUS_PATH,
        DBUS_PROPERTIES_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL)
        g_error ("Can't contact Logind: %s", error->message);

    value = g_dbus_proxy_call_sync(
        properties,
        "Get",
        g_variant_new (
            "(ss)",
            LOGIND_DBUS_INTERFACE,
            "ActiveSession"
        ),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL)
        g_error ("Can't contact Logind: %s", error->message);

    g_variant_get (value, "(v)", &inner_value);
    g_variant_get(inner_value, "(&s&o)", &session_id, &session_path);

    g_warning("Logind session path: %s", session_path);

    self->priv->logind_proxy = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM,
        0,
        NULL,
        LOGIND_DBUS_NAME,
        session_path,
        LOGIND_DBUS_SESSION_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL)
        g_error ("Can't contact Logind: %s", error->message);

    g_signal_connect (
        self->priv->logind_proxy,
        "g-properties-changed",
        G_CALLBACK (on_logind_proxy_properties),
        self
    );
}

static void
logind_dispose (GObject *logind)
{
    Logind *self = LOGIND (logind);

    g_clear_object (&self->priv->logind_proxy);

    G_OBJECT_CLASS (logind_parent_class)->dispose (logind);
}

static void
logind_finalize (GObject *logind)
{
    G_OBJECT_CLASS (logind_parent_class)->finalize (logind);
}

static void
logind_class_init (LogindClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = logind_dispose;
    object_class->finalize = logind_finalize;

    signals[SCREEN_STATE_CHANGED] = g_signal_new (
        "screen-state-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_BOOLEAN
    );
}

static void
logind_init (Logind *self)
{
    self->priv = logind_get_instance_private (self);

    connect_logind (self);
}

/**
 * logind_new:
 *
 * Creates a new #Logind
 *
 * Returns: (transfer full): a new #Logind
 *
 **/
GObject *
logind_new (void)
{
    GObject *logind;

    logind = g_object_new (TYPE_LOGIND, NULL);

    return logind;
}

static Logind *default_logind = NULL;
/**
 * logind_get_default:
 *
 * Gets the default #Logind.
 *
 * Return value: (transfer full): the default #Logind.
 */
Logind *
logind_get_default (void)
{
    if (default_logind == NULL) {
        default_logind = LOGIND (logind_new ());
    }
    return default_logind;
}

/**
 * logind_free_default:
 *
 * Free the default #Logind.
 *
 */
void
logind_free_default (void)
{
    if (default_logind != NULL) {
        g_clear_object (&default_logind);
        default_logind = NULL;
    }
}
