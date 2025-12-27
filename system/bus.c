/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "bus.h"
#include "config.h"
#include "../common/define.h"
#include "../common/utils.h"

#define ADISHATZ_DBUS_NAME "org.adishatz.Mps"
#define ADISHATZ_DBUS_PATH "/org/adishatz/Mps"

#ifdef UPOWER_ENABLED
#define UPOWERPP_DBUS_NAME "org.freedesktop.UPower.PowerProfiles"
#define UPOWERPP_DBUS_PATH "/org/freedesktop/UPower/PowerProfiles"
#endif

/* signals */
enum
{
    BUS_SETTING_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _BusPrivate {
    GDBusConnection *adishatz_connection;
    GDBusNodeInfo *adishatz_introspection_data;
    guint adishatz_owner_id;

#ifdef UPOWER_ENABLED
    guint upowerpp_owner_id;
    GDBusNodeInfo *upowerpp_introspection_data;
    GDBusConnection *upowerpp_connection;
#endif

    PowerProfile power_profile;
};

G_DEFINE_TYPE_WITH_CODE (Bus, bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (Bus))

#ifdef UPOWER_ENABLED
static const char*
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
get_power_profile_from_string (const char *name)
{
    if (g_strcmp0 (name, "power-saver") == 0)
        return POWER_PROFILE_POWER_SAVER;
    if (g_strcmp0 (name, "performance") == 0)
        return POWER_PROFILE_PERFORMANCE;
    return POWER_PROFILE_BALANCED;
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
#endif

static void
handle_method_call (GDBusConnection       *connection,
                    const char           *sender,
                    const char           *object_path,
                    const char           *interface_name,
                    const char           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
    Bus *self = user_data;

#ifdef UPOWER_ENABLED
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
#endif

    if (g_strcmp0 (method_name, "Set") == 0) {
        const char *setting;
        g_autoptr (GVariant) value;

        g_variant_get (parameters, "(&sv)", &setting, &value);
        g_signal_emit(
            self,
            signals[BUS_SETTING_CHANGED],
            0,
            g_variant_new ("(&sv)", setting, g_steal_pointer (&value))
        );

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );
    } else if (g_strcmp0 (method_name, "StopDozing") == 0) {
        g_dbus_connection_emit_signal (
            self->priv->adishatz_connection,
            NULL,
            ADISHATZ_DBUS_PATH,
            ADISHATZ_DBUS_NAME,
            "StopDozing",
            NULL,
            NULL
        );

        g_dbus_method_invocation_return_value (
            invocation, NULL
        );
    }
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                     const char     *sender,
                     const char     *object_path,
                     const char     *interface_name,
                     const char     *property_name,
                     GError         **error,
                     gpointer         user_data)
{
#ifdef UPOWER_ENABLED
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

    if (g_strcmp0 (property_name, "Version") == 0)
        return g_variant_new_string (PACKAGE_VERSION);
#endif
    return NULL;
}

static gboolean
handle_set_property (GDBusConnection  *connection,
                     const char       *sender,
                     const char       *object_path,
                     const char       *interface_name,
                     const char       *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{
#ifdef UPOWER_ENABLED
    Bus *self = user_data;

    if (g_strcmp0 (property_name, "ActiveProfile") == 0) {
        const char *power_profile = g_variant_get_string (value, NULL);

        self->priv->power_profile = get_power_profile_from_string (
            power_profile
        );
        return TRUE;
    }
#endif
    g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                     "No such property: %s", property_name);
    return FALSE;
}

static const GDBusInterfaceVTable adishatz_interface_vtable = {
    handle_method_call,
    handle_get_property,
    handle_set_property
};

#ifdef UPOWER_ENABLED
static const GDBusInterfaceVTable upowerpp_interface_vtable = {
    handle_method_call,
    handle_get_property,
    handle_set_property
};
#endif

static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
    Bus *self = user_data;
    guint registration_id;
    GDBusNodeInfo *introspection_data;
    const char *dbus_path;
    const GDBusInterfaceVTable *vtable;
    gboolean is_adishatz = g_strcmp0 (name, ADISHATZ_DBUS_NAME) == 0;

    if (is_adishatz) {
        dbus_path = ADISHATZ_DBUS_PATH;
        introspection_data = self->priv->adishatz_introspection_data;
        vtable = &adishatz_interface_vtable;
    }
#ifdef UPOWER_ENABLED
    else {
        dbus_path = UPOWERPP_DBUS_PATH;
        introspection_data = self->priv->upowerpp_introspection_data;
        vtable = &upowerpp_interface_vtable;
    }
#endif
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
#ifdef UPOWER_ENABLED
    else
        self->priv->upowerpp_connection = g_object_ref (connection);
#endif
    g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{}

static void
on_name_lost (GDBusConnection *connection,
              const char      *name,
              gpointer         user_data)
{
    g_error ("Cannot own D-Bus name. Verify installation: %s\n", name);
}

static GDBusNodeInfo *
bus_init_path (const char *dbus_name,
               const char *xml,
               guint      *owner_id,
               gpointer    user_data)
{
    Bus *self = user_data;
    g_autoptr (GBytes) bytes = NULL;
    GDBusNodeInfo *introspection_data;

    bytes = g_resources_lookup_data (
        xml,
        G_RESOURCE_LOOKUP_FLAGS_NONE,
        NULL
    );

    if (!bytes) {
        g_error("Failed to lookup resource: %s", xml);
        return NULL;
    }

    introspection_data = g_dbus_node_info_new_for_xml (
        g_bytes_get_data (bytes, NULL),
        NULL
    );

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
    g_clear_pointer (
      &self->priv->adishatz_introspection_data, g_dbus_node_info_unref
    );
    g_clear_object (&self->priv->adishatz_connection);

#ifdef UPOWER_ENABLED
    if (self->priv->upowerpp_owner_id != 0) {
        g_bus_unown_name (self->priv->upowerpp_owner_id);
    }
    g_clear_pointer (
      &self->priv->upowerpp_introspection_data, g_dbus_node_info_unref
    );
    g_clear_object (&self->priv->upowerpp_connection);
#endif

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
    object_class->dispose = bus_dispose;
    object_class->finalize = bus_finalize;

    signals[BUS_SETTING_CHANGED] = g_signal_new (
        "bus-setting-changed",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL, NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_VARIANT
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
    self->priv->adishatz_connection = NULL;

#ifdef UPOWER_ENABLED
    self->priv->upowerpp_introspection_data = bus_init_path (
        UPOWERPP_DBUS_NAME,
        "/org/adishatz/Mps/org.freedesktop.UPower.PowerProfiles.xml",
        &self->priv->upowerpp_owner_id,
        self
    );
    self->priv->power_profile = POWER_PROFILE_BALANCED;
    self->priv->upowerpp_connection = NULL;
#endif
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
    if (default_bus == NULL) {
        default_bus = BUS (bus_new ());
    }
    return g_object_ref (default_bus);
}

/**
 * bus_free_default:
 *
 * Free the default #Bus.
 *
 */
void
bus_free_default (void)
{
    if (default_bus != NULL) {
        g_clear_object (&default_bus);
        default_bus = NULL;
    }
}

void
bus_screen_state_changed (Bus      *self,
                          gboolean  enabled)
{
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
