/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "devfreq_device.h"
#include "../common/define.h"

/* struct _DevfreqDevicePrivate { */
/* }; */

G_DEFINE_TYPE_WITH_CODE (
    DevfreqDevice,
    devfreq_device,
    TYPE_FREQ_DEVICE,
    /* G_ADD_PRIVATE (DevfreqDevice) */
)

static void
devfreq_device_dispose (GObject *devfreq_device)
{
    G_OBJECT_CLASS (devfreq_device_parent_class)->dispose (devfreq_device);
}

static void
devfreq_device_finalize (GObject *devfreq_device)
{
    G_OBJECT_CLASS (devfreq_device_parent_class)->finalize (devfreq_device);
}

static void
devfreq_device_class_init (DevfreqDeviceClass *klass)
{
    GObjectClass *freq_device_class;

    freq_device_class = G_OBJECT_CLASS (klass);
    freq_device_class->dispose = devfreq_device_dispose;
    freq_device_class->finalize = devfreq_device_finalize;
}

static void
devfreq_device_init (DevfreqDevice *self)
{
    self->priv = devfreq_device_get_instance_private (self);

    freq_device_set_sysfs_settings (
        FREQ_DEVICE (self), DEVFREQ_DIR, "governor"
    );
}

/**
 * devfreq_device_new:
 *
 * Creates a new #DevfreqDevice

 * Returns: (transfer full): a new #DevfreqDevice
 *
 **/
GObject *
devfreq_device_new (void)
{
    GObject *devfreq_device;

    devfreq_device = g_object_new (TYPE_DEVFREQ_DEVICE, NULL);

    return devfreq_device;
}