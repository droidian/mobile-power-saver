/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 * Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

#include "binder.h"

enum Hints {
    BINDER_INTERACTION = 0x00000002,
    BINDER_POWERSAVE = 0x00000005,
    BINDER_PERFORMANCE = 0x00000006
};


struct _BinderPrivate {
    GBinderServiceManager *service_manager;
    GBinderRemoteObject *remote_object;
    GBinderClient *client;
};


G_DEFINE_TYPE_WITH_CODE (
    Binder,
    binder,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Binder)
)


static void
binder_set_modes(GBinderClient* client,
                 const gint interactive,
                 const enum Hints hint)
{
    gint status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    // interactive mode
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, interactive);
    gbinder_client_transact_sync_reply(client, 1, req, &status);
    gbinder_local_request_unref(req);

    // hints
    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) hint);
    gbinder_writer_append_int32(&writer, interactive);
    gbinder_client_transact_sync_reply(client, 2, req, &status);
    gbinder_local_request_unref(req);
}


static void
binder_dispose (GObject *binder)
{
    G_OBJECT_CLASS (binder_parent_class)->dispose (binder);
}


static void
binder_finalize (GObject *binder)
{
    Binder *self = BINDER (binder);

    if (self->priv->client != NULL)
        gbinder_client_unref(self->priv->client);

    if (self->priv->remote_object != NULL)
        gbinder_remote_object_unref(self->priv->remote_object);

    if (self->priv->service_manager != NULL)
        gbinder_servicemanager_unref(self->priv->service_manager);

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

    self->priv->client = NULL;
    self->priv->remote_object = NULL;

    self->priv->service_manager = gbinder_servicemanager_new("/dev/hwbinder");
    if (self->priv->service_manager != NULL)
        self->priv->remote_object = gbinder_servicemanager_get_service_sync(
            self->priv->service_manager,
            "android.hardware.power@1.0::IPower/default",
            NULL
        );

    if (self->priv->remote_object != NULL)
        self->priv->client = gbinder_client_new(
            self->priv->remote_object,
            "android.hardware.power@1.0::IPower"
        );

    if (self->priv->client == NULL)
        g_warning ("Can't setup binder!");
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
 * binder_set_powersave:
 *
 * Set binder to powersave mode
 *
 * @param #Binder
 * @param powersave: TRUE to enable powersave mode
 *
 */
void
binder_set_powersave (Binder *self, gboolean powersave)
{
    if (powersave) {
        binder_set_modes(self->priv->client, 0, BINDER_POWERSAVE);
    } else {
        binder_set_modes(self->priv->client, 1, BINDER_PERFORMANCE);
    }
}