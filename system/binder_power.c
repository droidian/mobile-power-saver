/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 * Copyright (C) 2023-2024 Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

#include "binder_power.h"
#include "../common/define.h"

enum IDLHint {
    HIDL_POWERSAVE    = 0x00000005,
    HIDL_PERFORMANCE  = 0x00000006,
    AIDL_POWERSAVE    = 1,
    AIDL_PERFORMANCE  = 2
};

G_DEFINE_TYPE_WITH_CODE (
    BinderPower,
    binder_power,
    TYPE_BINDER_CLIENT,
    NULL
)

static int
get_hint (BinderServiceManagerType service_type,
          PowerProfile             power_profile)
{
    if (power_profile == POWER_PROFILE_PERFORMANCE)
        return service_type == BINDER_SERVICE_MANAGER_TYPE_AIDL ?
            AIDL_POWERSAVE : HIDL_POWERSAVE;
    else
        return service_type == BINDER_SERVICE_MANAGER_TYPE_AIDL ?
            AIDL_PERFORMANCE: HIDL_PERFORMANCE;
}

static void
hidl_set_power_profile (BinderClient *self,
                        PowerProfile  power_profile) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request (self->client);
    GBinderWriter writer;
    enum IDLHint hint = get_hint (
        BINDER_SERVICE_MANAGER_TYPE_AIDL, power_profile
    );
    int interactive = hint == HIDL_POWERSAVE ? 0 : 1;

    /* interactive mode */
    gbinder_local_request_init_writer (req, &writer);
    gbinder_writer_append_bool (&writer, interactive);
    gbinder_client_transact_sync_reply (self->client, 1, req, &status);
    gbinder_local_request_unref (req);

    /* hints */
    req = gbinder_client_new_request (self->client);
    gbinder_local_request_init_writer (req, &writer);
    gbinder_writer_append_int32 (&writer, (uint32_t) hint);
    gbinder_writer_append_int32 (&writer, interactive);
    gbinder_client_transact_sync_reply (self->client, 2, req, &status);
    gbinder_local_request_unref (req);
}

static void
aidl_set_power_profile (BinderClient *self,
                        PowerProfile  power_profile) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request (self->client);
    GBinderWriter writer;
    enum IDLHint hint = get_hint (
        BINDER_SERVICE_MANAGER_TYPE_AIDL, power_profile
    );
    enum IDLHint other = (hint == AIDL_POWERSAVE) ?
        AIDL_PERFORMANCE : AIDL_POWERSAVE;
    int interactive = hint == AIDL_POWERSAVE ? 0 : 1;

    if (power_profile == POWER_PROFILE_PERFORMANCE) {
        /* interactive mode */
        gbinder_local_request_init_writer(req, &writer);
        gbinder_writer_append_bool(&writer, interactive);
        /* boost for 5 minutes */
        gbinder_writer_append_int32(&writer, 300000); //
        gbinder_client_transact_sync_reply(self->client, 3, req, &status);
        gbinder_local_request_unref(req);
    }

    /* disable other modes */
    req = gbinder_client_new_request(self->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) other);
    gbinder_writer_append_int32(&writer, 0);
    gbinder_client_transact_sync_reply(self->client, 1, req, &status);
    gbinder_local_request_unref(req);

    /* set requested mode */
    req = gbinder_client_new_request(self->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) hint);
    gbinder_writer_append_int32(&writer, 1);
    gbinder_client_transact_sync_reply(self->client, 1, req, &status);
    gbinder_local_request_unref(req);
}

static void
binder_power_dispose (GObject *binder_power)
{
    G_OBJECT_CLASS (binder_power_parent_class)->dispose (binder_power);
}

static void
binder_power_finalize (GObject *binder_power)
{
    G_OBJECT_CLASS (binder_power_parent_class)->finalize (binder_power);
}

static void
binder_power_init_binder (BinderClient *self,
                          const gchar  *hidl_service,
                          const gchar  *hidl_client,
                          const gchar  *aidl_service,
                          const gchar  *aidl_client)
{
    BINDER_CLIENT_CLASS (binder_power_parent_class)->init_binder (
        self,
        hidl_service,
        hidl_client,
        aidl_service,
        aidl_client
    );
}

static void
binder_power_set_power_profile (BinderClient *self,
                                PowerProfile  power_profile) {
    if (self->type == BINDER_SERVICE_MANAGER_TYPE_HIDL) {
        hidl_set_power_profile (self, power_profile);
    } else {
        aidl_set_power_profile (self, power_profile);
    }
}

static void
binder_power_class_init (BinderPowerClass *klass)
{
    GObjectClass *object_class;
    BinderClientClass *binder_client_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = binder_power_dispose;
    object_class->finalize = binder_power_finalize;

    binder_client_class = BINDER_CLIENT_CLASS (klass);
    binder_client_class->init_binder = binder_power_init_binder;
    binder_client_class->set_power_profile = binder_power_set_power_profile;
}

static void
binder_power_init (BinderPower *self)
{
    BinderClientClass *klass = BINDER_CLIENT_GET_CLASS (self);

    klass->init_binder (
        BINDER_CLIENT (self),
        "android.hardware.power@1.0::IPower/default",
        "android.hardware.power@1.0::IPower",
        "android.hardware.power.IPower/default",
        "android.hardware.power.IPower"
    );
}

/**
 * binder_power_new:
 *
 * Creates a new #BinderPower

 * Returns: (transfer full): a new #BinderPower
 *
 **/
GObject *
binder_power_new (void)
{
    GObject *binder_power;

    binder_power = g_object_new (TYPE_BINDER_POWER, NULL);

    return binder_power;
}