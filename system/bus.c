/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "bus.h"
#include "../common/utils.h"

#define ADISHATZ_DBUS_NAME "org.adishatz.Mps"
#define ADISHATZ_DBUS_PATH "/org/adishatz/Mps"

#define HADESS_DBUS_NAME "net.hadess.PowerProfiles"
#define HADESS_DBUS_PATH "/net/hadess/PowerProfiles"

/* signals */
enum
{
    POWER_SAVING_MODE_CHANGED,
    SCREEN_OFF_POWER_SAVING_CHANGED,
    SCREEN_OFF_SUSPEND_PROCESSES_CHANGED,
    SCREEN_OFF_SUSPEND_SERVICES_CHANGED,
    SCREEN_STATE_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


struct _BusPrivate {
    GDBusConnection *adishatz_connection;
    GDBusConnection *hadess_connection;

    GDBusNodeInfo *adishatz_introspection_data;
    GDBusNodeInfo *hadess_introspection_data;

    guint adishatz_owner_id;
    guint hadess_owner_id;

    PowerProfile power_profile;
};

G_DEFINE_TYPE_WITH_CODE (Bus, bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (Bus))


static const gchar*
get_power_profile_as_string (PowerProfile power_profile) {
    switch (power_profile) {
    case POWER_PROFILE_POWER_SAVER:
        return "power-saver";
    case POWER_PROFILE_PERFORMANCE:
        return "performance";
    case POWER_PROFILE_BALANCED:
    case POWER_PROFILE_LAST:
    default:
        return "balanced";
    }
}


static PowerProfile
get_power_profile_from_string (const gchar *name)
{
    if (g_strcmp0 (name, "power-saver") == 0)
        return POWER_PROFILE_POWER_SAVER;
    if (g_strcmp0 (name, "performance") == 0)
        return POWER_PROFILE_PERFORMANCE;
    return POWER_PROFILE_BALANCED;
}


static const gchar *
get_governor_from_power_profile (const gchar *name) {
    if (g_strcmp0 (name, "power-saver") == 0)
        return "powersave";
    if (g_strcmp0 (name, "performance") == 0)
        return "performance";
    return NULL;
}


static GVariant *
get_profiles_variant (void)
{
    GVariantBuilder builder;
    gint i;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

    for (i = 0; i < POWER_PROFILE_LAST; i++) {
        GVariantBuilder asv_builder;

        g_variant_builder_init (&asv_builder, G_VARIANT_TYPE ("a{sv}"));
        g_variant_builder_add (
            &asv_builder,
            "{sv}",
            "Profile",
            g_variant_new_string (get_power_profile_as_string (i))
        );
        g_variant_builder_add (
            &asv_builder, "{sv}", "Driver", g_variant_new_string ("multiple")
        );
        g_variant_builder_add (&builder, "a{sv}", &asv_builder);
    }

  return g_variant_builder_end (&builder);
}


static void
handle_method_call (GDBusConnection *connection,
                    const gchar *sender,
                    const gchar *object_path,
                    const gchar *interface_name,
                    const gchar *method_name,
                    GVariant *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer user_data)
{
    Bus *self = user_data;

    if (g_strcmp0 (method_name, "HoldProfile") == 0) {
        /*
         * We do not want application to change power profile, on mobile
         * devices, it does not looks like a good idea.
         */
        g_warning ("HoldProfile is not implemented...");
        g_dbus_method_invocation_return_value (
            invocation, g_variant_new ("(u)", 0)
        );
        return;
    }

    if (g_strcmp0 (method_name, "ReleaseProfile") == 0) {
        g_warning ("ReleaseProfile is not implemented...");
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "Set") == 0) {
        const gchar *setting;
        g_autoptr(GVariant) value;

        g_variant_get (parameters, "(&sv)", &setting, &value);
        if (g_strcmp0 (setting, "screen-off-power-saving") == 0) {
            g_signal_emit(
                self,
                signals[SCREEN_OFF_POWER_SAVING_CHANGED],
                0,
                g_variant_get_boolean (value)
            );
        } else if (g_strcmp0 (setting, "screen-off-suspend-processes") == 0) {
            g_signal_emit(
                self,
                signals[SCREEN_OFF_SUSPEND_PROCESSES_CHANGED],
                0,
                g_steal_pointer (&value)
            );
        } else if (g_strcmp0 (setting, "screen-off-suspend-system-services") == 0) {
            g_signal_emit(
                self,
                signals[SCREEN_OFF_SUSPEND_SERVICES_CHANGED],
                0,
                g_steal_pointer (&value)
            );
        }

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );
        return;
    }

    if (g_strcmp0 (method_name, "SimulateScreenOff") == 0) {
        gboolean screen_off;

        g_variant_get (parameters, "(b)", &screen_off);

        g_signal_emit(
            self,
            signals[SCREEN_STATE_CHANGED],
            0,
            !screen_off
        );

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );
        return;
    }
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                     const gchar *sender,
                     const gchar *object_path,
                     const gchar *interface_name,
                     const gchar *property_name,
                     GError **error,
                     gpointer user_data)
{
    Bus *self = user_data;

    if (g_strcmp0 (property_name, "ActiveProfile") == 0)
        return g_variant_new_string (
            get_power_profile_as_string (self->priv->power_profile)
        );

    if (g_strcmp0 (property_name, "Profiles") == 0)
        return get_profiles_variant ();

    /* On mobile devices, we use in kernel mitigation methods */
    if (g_strcmp0 (property_name, "PerformanceDegraded") == 0)
        return g_variant_new_boolean (FALSE);

    return NULL;
}


static gboolean
handle_set_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{
  Bus *self = user_data;

    if (g_strcmp0 (property_name, "ActiveProfile") == 0) {
        const gchar *power_profile = g_variant_get_string (value, NULL);

        self->priv->power_profile = get_power_profile_from_string (
            power_profile
        );

        g_signal_emit(
            self,
            signals[POWER_SAVING_MODE_CHANGED],
            0,
            get_governor_from_power_profile (power_profile)
        );
        return TRUE;
    } else {
        g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                     "No such property: %s", property_name);
        return FALSE;
    }
}


static const GDBusInterfaceVTable adishatz_interface_vtable = {
    handle_method_call,
    handle_get_property,
    handle_set_property
};

static const GDBusInterfaceVTable hadess_interface_vtable = {
    handle_method_call,
    handle_get_property,
    handle_set_property
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
    Bus *self = user_data;
    guint registration_id;
    GDBusNodeInfo *introspection_data;
    const gchar *dbus_path;
    const GDBusInterfaceVTable *vtable;
    gboolean is_adishatz = g_strcmp0 (name, ADISHATZ_DBUS_NAME) == 0;

    if (is_adishatz) {
        dbus_path = ADISHATZ_DBUS_PATH;
        introspection_data = self->priv->adishatz_introspection_data;
        vtable = &adishatz_interface_vtable;
    } else {
        dbus_path = HADESS_DBUS_PATH;
        introspection_data = self->priv->hadess_introspection_data;
        vtable = &hadess_interface_vtable;
    }

    registration_id = g_dbus_connection_register_object (
        connection,
        dbus_path,
        introspection_data->interfaces[0],
        vtable,
        user_data,
        NULL,
        NULL
    );

    if (is_adishatz)
        self->priv->adishatz_connection = g_object_ref (connection);
    else
        self->priv->hadess_connection = g_object_ref (connection);

    g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{}


static void
on_name_lost (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
    g_error ("Cannot own D-Bus name. Verify installation: %s\n", name);
}

static void
bus_set_property (GObject *object,
                        guint property_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
bus_get_property (GObject *object,
                  guint property_id,
                  GValue *value,
                  GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static GDBusNodeInfo *
bus_init_path (const gchar *dbus_name,
               const gchar *xml,
               guint *owner_id,
               gpointer user_data)
{
    Bus *self = user_data;
    GBytes *bytes;
    GDBusNodeInfo *introspection_data;

    bytes = g_resources_lookup_data (
        xml,
        G_RESOURCE_LOOKUP_FLAGS_NONE,
        NULL
    );

    introspection_data = g_dbus_node_info_new_for_xml (
        g_bytes_get_data (bytes, NULL),
        NULL
    );
    g_bytes_unref (bytes);

    g_assert (introspection_data != NULL);

    *owner_id = g_bus_own_name (
        G_BUS_TYPE_SYSTEM,
        dbus_name,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        on_name_acquired,
        on_name_lost,
        self,
        NULL
    );

    return introspection_data;
}

static void
bus_dispose (GObject *bus)
{
    Bus *self = BUS (bus);

    if (self->priv->adishatz_owner_id != 0) {
        g_bus_unown_name (self->priv->adishatz_owner_id);
    }

    if (self->priv->hadess_owner_id != 0) {
        g_bus_unown_name (self->priv->hadess_owner_id);
    }

    g_clear_pointer (
      &self->priv->adishatz_introspection_data, g_dbus_node_info_unref
    );
    g_clear_pointer (
      &self->priv->hadess_introspection_data, g_dbus_node_info_unref
    );
    g_clear_object (&self->priv->adishatz_connection);
    g_clear_object (&self->priv->hadess_connection);

    G_OBJECT_CLASS (bus_parent_class)->dispose (bus);
}


static void
bus_finalize (GObject *bus)
{
    G_OBJECT_CLASS (bus_parent_class)->finalize (bus);
}


static void
bus_class_init (BusClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = bus_set_property;
    object_class->get_property = bus_get_property;
    object_class->dispose = bus_dispose;
    object_class->finalize = bus_finalize;

    signals[POWER_SAVING_MODE_CHANGED] = g_signal_new (
        "power-saving-mode-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_STRING
    );

    signals[SCREEN_OFF_POWER_SAVING_CHANGED] = g_signal_new (
        "screen-off-power-saving-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_BOOLEAN
    );

    signals[SCREEN_OFF_SUSPEND_PROCESSES_CHANGED] = g_signal_new (
        "screen-off-suspend-processes-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_VARIANT
    );

    signals[SCREEN_OFF_SUSPEND_SERVICES_CHANGED] = g_signal_new (
        "screen-off-suspend-services-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_VARIANT
    );

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
bus_init (Bus *self)
{
    self->priv = bus_get_instance_private (self);

    self->priv->adishatz_introspection_data = bus_init_path (
        ADISHATZ_DBUS_NAME,
        "/org/adishatz/Mps/org.adishatz.Mps.xml",
        &self->priv->adishatz_owner_id,
        self
    );

    self->priv->hadess_introspection_data = bus_init_path (
        HADESS_DBUS_NAME,
        "/org/adishatz/Mps/net.hadess.PowerProfiles.xml",
        &self->priv->hadess_owner_id,
        self
    );

    self->priv->power_profile = POWER_PROFILE_BALANCED;
}

/**
 * bus_new:
 * 
 * Creates a new #Bus
 *
 * Returns: (transfer full): a new #Bus
 *
 **/
GObject *
bus_new (void)
{
    GObject *bus;

    bus = g_object_new (
        TYPE_BUS,
        NULL
    );

    return bus;
}

static Bus *default_bus = NULL;
/**
 * bus_get_default:
 *
 * Gets the default #Bus.
 *
 * Return value: (transfer full): the default #Bus.
 */
Bus *
bus_get_default (void)
{
    if (!default_bus) {
        default_bus = BUS (bus_new ());
    }
    return g_object_ref (default_bus);
}

void
bus_screen_state_changed (Bus *self,
                          gboolean enabled)
{
    g_message ("bus_screen_state_changed: %b %p", enabled, self->priv->adishatz_connection);
    g_dbus_connection_emit_signal (
        self->priv->adishatz_connection,
        NULL,
        ADISHATZ_DBUS_PATH,
        ADISHATZ_DBUS_NAME,
        "ScreenStateChanged",
        g_variant_new ("(b)", enabled),
        NULL
    );
}