/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 * Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

#include "binder_client.h"
#include "../common/define.h"

struct _BinderClientPrivate {
    GBinderServiceManager *service_manager;
    GBinderRemoteObject *remote_object;
};

G_DEFINE_TYPE_WITH_CODE (
    BinderClient,
    binder_client,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (BinderClient)
)

static gboolean
init_binder_client (BinderClient *self,
                    const gchar  *device,
                    const gchar  *service,
                    const gchar  *client)
{
    if (!g_file_test (GBINDER_DEFAULT_HWBINDER, G_FILE_TEST_EXISTS))
        return FALSE;

    self->priv->service_manager = gbinder_servicemanager_new(
        GBINDER_DEFAULT_HWBINDER
    );

   if (self->priv->service_manager == NULL)
       return FALSE;

    self->priv->remote_object = gbinder_servicemanager_get_service_sync(
        self->priv->service_manager,
        service,
        NULL
    );

    if (self->priv->remote_object == NULL)
        return FALSE;

    self->client = gbinder_client_new(
        self->priv->remote_object,
        client
    );

    if (self->client == NULL) {
        if (self->priv->remote_object != NULL)
            gbinder_remote_object_unref(self->priv->remote_object);
        return FALSE;
    }

    return TRUE;
}

static void
init_binder (BinderClient *self,
             const gchar  *hidl_service,
             const gchar  *hidl_client,
             const gchar  *aidl_service,
             const gchar  *aidl_client)
{
    if (init_binder_client (self,
                            GBINDER_DEFAULT_HWBINDER,
                            hidl_service,
                            hidl_client)) {
        self->type = BINDER_SERVICE_MANAGER_TYPE_HIDL;
    } else if (init_binder_client (self,
                                   GBINDER_DEFAULT_BINDER,
                                   aidl_service,
                                   aidl_client)) {
        self->type = BINDER_SERVICE_MANAGER_TYPE_AIDL;
    }
}
static void
binder_client_dispose (GObject *binder_client)
{
    G_OBJECT_CLASS (binder_client_parent_class)->dispose (binder_client);
}

static void
binder_client_finalize (GObject *binder_client)
{
    BinderClient *self = BINDER_CLIENT (binder_client);

    if (self->client != NULL)
        gbinder_client_unref(self->client);

    if (self->priv->remote_object != NULL)
        gbinder_remote_object_unref(self->priv->remote_object);

    if (self->priv->service_manager != NULL)
        gbinder_servicemanager_unref(self->priv->service_manager);

    G_OBJECT_CLASS (binder_client_parent_class)->finalize (binder_client);
}

static void
binder_client_class_init (BinderClientClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = binder_client_dispose;
    object_class->finalize = binder_client_finalize;

    klass->init_binder = init_binder;
}

static void
binder_client_init (BinderClient *self)
{
    self->priv = binder_client_get_instance_private (self);

    self->client = NULL;
    self->priv->remote_object = NULL;
    self->priv->service_manager = NULL;
}

/**
 * binder_client_new:
 *
 * Creates a new #BinderClient

 * Returns: (transfer full): a new #BinderClient
 *
 **/
GObject *
binder_client_new (void)
{
    GObject *binder_client;

    binder_client = g_object_new (TYPE_BINDER_CLIENT, NULL);

    return binder_client;
}

/**
 * binder_client_set_power_profile:
 *
 * Set binder client power profile
 *
 * @param #BinderClient
 * @param power_profile: Power profile to enable
 *
 */
void
binder_client_set_power_profile (BinderClient *self,
                                 PowerProfile  power_profile)
{
    BinderClientClass *klass = BINDER_CLIENT_GET_CLASS (self);

    if (self->client == NULL)
        return;

    klass->set_power_profile (BINDER_CLIENT (self), power_profile);
}