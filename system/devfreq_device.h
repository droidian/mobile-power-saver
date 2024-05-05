/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef DEVFREQ_DEVICE_H
#define DEVFREQ_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#include "freq_device.h"

#define TYPE_DEVFREQ_DEVICE \
    (devfreq_device_get_type ())
#define DEVFREQ_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_DEVFREQ_DEVICE, DevfreqDevice))
#define DEVFREQ_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_DEVFREQ_DEVICE, DevfreqDeviceClass))
#define IS_DEVFREQ_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_DEVFREQ_DEVICE))
#define IS_DEVFREQ_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_DEVFREQ_DEVICE))
#define DEVFREQ_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_DEVFREQ_DEVICE, DevfreqDeviceClass))

G_BEGIN_DECLS

typedef struct _DevfreqDevice DevfreqDevice;
typedef struct _DevfreqDeviceClass DevfreqDeviceClass;
typedef struct _DevfreqDevicePrivate DevfreqDevicePrivate;

struct _DevfreqDevice {
    FreqDevice parent;
    DevfreqDevicePrivate *priv;
};

struct _DevfreqDeviceClass {
    FreqDeviceClass parent_class;
};

GType           devfreq_device_get_type         (void) G_GNUC_CONST;

GObject*        devfreq_device_new              (void);

G_END_DECLS

#endif
