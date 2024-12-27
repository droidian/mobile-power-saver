/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "network_manager.h"
#include "modem.h"
#include "../common/define.h"
#include "../common/utils.h"

#define MODEM_DBUS_NAME                     "org.modem"
#define MODEM_DBUS_PATH                     "/"
#define MODEM_MANAGER_DBUS_INTERFACE        "org.modem.Manager"

struct _ModemPrivate {
    ModemPowersave modem_powersave;
};

G_DEFINE_TYPE_WITH_CODE (
    Modem,
    modem,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Modem)
)

static void
modem_dispose (GObject *modem)
{
    G_OBJECT_CLASS (modem_parent_class)->dispose (modem);
}

static void
modem_finalize (GObject *modem)
{
    G_OBJECT_CLASS (modem_parent_class)->finalize (modem);
}

static void
modem_class_init (ModemClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = modem_dispose;
    object_class->finalize = modem_finalize;
}

static void
modem_init (Modem *self)
{
    self->priv = modem_get_instance_private (self);
}

/**
 * modem_new:
 *
 * Creates a new #Modem
 *
 * Returns: (transfer full): a new #Modem
 *
 **/
GObject *
modem_new (void)
{
    GObject *modem;

    modem = g_object_new (TYPE_MODEM, NULL);

    return modem;
}

/**
 * modem_set_powersave:
 *
 * Set modem devices to powersave
 *
 * @param #Modem
 * @param powersave: True to enable powersave
 * @param modem_powersave: #ModemPowersave flags
 *
 * Returns: TRUE if new settings should be applied
 */
gboolean
modem_set_powersave (Modem          *self,
                     gboolean        powersave,
                     ModemPowersave  modem_powersave)
{
    ModemPowersave modem_powersave_tmp = self->priv->modem_powersave;
    gboolean current_powersave = (
        self->priv->modem_powersave & MODEM_POWERSAVE_ENABLED
    ) == MODEM_POWERSAVE_ENABLED;

    if (powersave) {
        modem_powersave_tmp |= modem_powersave;
    } else {
        modem_powersave_tmp &= ~modem_powersave;
    }

    if (modem_powersave_tmp == self->priv->modem_powersave ||
            current_powersave == powersave)
        return FALSE;

    self->priv->modem_powersave = modem_powersave_tmp;
    if (powersave)
        self->priv->modem_powersave |= MODEM_POWERSAVE_ENABLED;
    else if (self->priv->modem_powersave == MODEM_POWERSAVE_ENABLED)
        self->priv->modem_powersave &= ~MODEM_POWERSAVE_ENABLED;

    g_debug("Modem powersave: %d", self->priv->modem_powersave);

    return TRUE;
}

/**
 * modem_get_powersave:
 *
 * Get modem devices powersaving flags
 *
 * @param #Modem
 *
 * Returns: #ModemPowersave
 */
ModemPowersave
modem_get_powersave (Modem *self)
{
    return self->priv->modem_powersave;

}