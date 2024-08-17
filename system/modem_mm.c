/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>
#include <libmm-glib.h>

#include "network_manager.h"
#include "modem_mm.h"
#include "../common/utils.h"

struct _ModemMMPrivate {
    GDBusConnection *connection;
    MMManager *manager;

    GList *modems;
};

G_DEFINE_TYPE_WITH_CODE (
    ModemMM,
    modem_mm,
    TYPE_MODEM,
    G_ADD_PRIVATE (ModemMM)
)

static void
on_modem_added (MMManager *modem_manager,
                MMObject  *modem_object,
                gpointer   user_data)
{
    ModemMM *self = MODEM_MM (user_data);
    MMModem *modem;

    modem = mm_object_peek_modem(modem_object);
    if (modem) {
        self->priv->modems = g_list_append (self->priv->modems, modem);
    }
}

static void
on_modem_removed (MMManager *modem_manager,
                  MMObject  *modem_object,
                  gpointer   user_data)
{
    ModemMM *self = MODEM_MM (user_data);
    MMModem *modem;
    const gchar *path;

    path = mm_object_get_path(modem_object);

    GFOREACH (self->priv->modems, modem) {
        if (g_strcmp0 (mm_modem_get_path (modem), path) == 0) {
            self->priv->modems = g_list_remove (self->priv->modems, modem);
            g_clear_object (&modem);
            break;
        }
    }
}

static void
modem_mm_set_powersave (Modem    *self,
                        gboolean  powersave)
{
    ModemMM *this = MODEM_MM (self);
    MMModem *modem;

    GFOREACH (this->priv->modems, modem) {
        MMModemModeCombination *modes = NULL;
        guint n_modes;

        if (mm_modem_get_supported_modes (modem, &modes, &n_modes)) {
            guint allowed, preferred;
            g_autoptr (GError) error = NULL;

            if (powersave) {
                allowed = modes[n_modes - 1].allowed;
                preferred = modes[n_modes - 1].preferred;
            } else {
                allowed = modes[0].allowed;
                preferred = modes[0].preferred;
            }

            g_message ("Modem mode: %u %u", allowed, preferred);

            mm_modem_set_current_modes_sync (
                modem, allowed, preferred, NULL, &error
            );
            if (error != NULL) {
                g_warning ("Can't set modem mode: %s", error->message);
            }

            g_free (modes);
        }
    }
}

static void
modem_mm_apply_powersave (Modem *self)
{
    gboolean powersave = (
        modem_get_powersave (MODEM (self)) & MODEM_POWERSAVE_ENABLED
    ) == MODEM_POWERSAVE_ENABLED;

    modem_mm_set_powersave (self, powersave);
}

static void
modem_mm_reset_powersave (Modem *self)
{
    modem_mm_set_powersave (self, FALSE);
}


static void
modem_mm_dispose (GObject *modem_mm)
{
    ModemMM *self = MODEM_MM (modem_mm);

    g_clear_object (&self->priv->connection);
    g_clear_object (&self->priv->manager);

    G_OBJECT_CLASS (modem_mm_parent_class)->dispose (modem_mm);
}

static void
modem_mm_finalize (GObject *modem_mm)
{
    ModemMM *self = MODEM_MM (modem_mm);

    g_list_free (self->priv->modems);

    G_OBJECT_CLASS (modem_mm_parent_class)->finalize (modem_mm);
}

static void
modem_mm_class_init (ModemMMClass *klass)
{
    GObjectClass *object_class;
    ModemClass *modem_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = modem_mm_dispose;
    object_class->finalize = modem_mm_finalize;

    modem_class = MODEM_CLASS (klass);
    modem_class->apply_powersave = modem_mm_apply_powersave;
    modem_class->reset_powersave = modem_mm_reset_powersave;
}

static void
modem_mm_init (ModemMM *self)
{
    GList *modems, *l;
    g_autoptr (GError) error = NULL;

    self->priv = modem_mm_get_instance_private (self);
    self->priv->modems = NULL;

    self->priv->connection = g_bus_get_sync (
        G_BUS_TYPE_SYSTEM, NULL, &error
    );

    if (error != NULL) {
        g_warning ("Can't connect to DBus: %s", error->message);
        return;
    }

    self->priv->manager = mm_manager_new_sync (
        self->priv->connection,
        G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning ("Can't connect to ModemManager: %s", error->message);
        return;
    }

    g_signal_connect(
        self->priv->manager,
        "object-added",
        G_CALLBACK(on_modem_added),
        self);
    g_signal_connect(
        self->priv->manager,
        "object-removed",
        G_CALLBACK(on_modem_removed),
        self);

    modems = g_dbus_object_manager_get_objects(
        G_DBUS_OBJECT_MANAGER(self->priv->manager)
    );

    for (l = modems; l; l = g_list_next(l))
        on_modem_added(self->priv->manager, MM_OBJECT(l->data), self);

    g_list_free_full(modems, (GDestroyNotify) g_object_unref);
}

/**
 * modem_mm_new:
 *
 * Creates a new #ModemMM
 *
 * Returns: (transfer full): a new #ModemMM
 *
 **/
GObject *
modem_mm_new (void)
{
    GObject *modem_mm;

    modem_mm = g_object_new (TYPE_MODEM_MM, NULL);

    return modem_mm;
}