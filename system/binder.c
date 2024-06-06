/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 * Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

#include "binder.h"
#include "binder_power.h"
#include "binder_radio.h"

struct _BinderPrivate {
    BinderClient *power_client;
    BinderClient *radio_client;
};

G_DEFINE_TYPE_WITH_CODE (
    Binder,
    binder,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Binder)
)

static void
binder_dispose (GObject *binder)
{
    Binder *self = BINDER (binder);

    g_clear_object (&self->priv->power_client);

    G_OBJECT_CLASS (binder_parent_class)->dispose (binder);
}

static void
binder_finalize (GObject *binder)
{
    G_OBJECT_CLASS (binder_parent_class)->finalize (binder);
}

static void
binder_class_init (BinderClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = binder_dispose;
    object_class->finalize = binder_finalize;
}

static void
binder_init (Binder *self)
{
    self->priv = binder_get_instance_private (self);

    self->priv->power_client = BINDER_CLIENT (binder_power_new ());
    self->priv->radio_client = BINDER_CLIENT (binder_radio_new ());
}

/**
 * binder_new:
 *
 * Creates a new #Binder
 *
 * Returns: (transfer full): a new #Binder
 *
 **/
GObject *
binder_new (void)
{
    GObject *binder;

    binder = g_object_new (TYPE_BINDER, NULL);

    return binder;
}

/**
 * binder_set_power_profile:
 *
 * Set binder power profile
 *
 * @param #Binder
 * @param power_profile: Power profile to enable
 *
 */
void
binder_set_power_profile (Binder  *self,
                          PowerProfile power_profile)
{
}

/**
 * binder_set_powersave:
 *
 * Set binder to powersave mode
 *
 * @param #Binder
 * @param powersave: TRUE to enable powersave mode
 *
 */
void
binder_set_powersave (Binder  *self,
                      gboolean powersave)
{
    binder_client_set_power_profile (
        self->priv->power_client, POWER_PROFILE_POWER_SAVER
    );
    binder_client_set_power_profile (
        self->priv->radio_client, POWER_PROFILE_POWER_SAVER
    );
}