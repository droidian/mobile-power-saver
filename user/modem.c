/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

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