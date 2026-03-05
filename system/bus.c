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

    PowerProfile power_profile;
};

G_DEFINE_TYPE_WITH_CODE (Bus, bus, G_TYPE_OBJECT,
    G_ADD_PRIVATE (Bus))

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
    return FALSE;
}

static const GDBusInterfaceVTable adishatz_interface_vtable = {
    handle_method_call,
    handle_get_property,
    handle_set_property
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
    Bus *self = user_data;
    guint registration_id;

    registration_id = g_dbus_connection_register_object (
        connection,
        ADISHATZ_DBUS_PATH,
        self->priv->adishatz_introspection_data->interfaces[0],
        &adishatz_interface_vtable,
        user_data,
        NULL,
        NULL
    );

    self->priv->adishatz_connection = g_object_ref (connection);

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
