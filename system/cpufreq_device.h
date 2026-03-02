/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef CPUFREQ_DEVICE_H
#define CPUFREQ_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#include "freq_device.h"

#define TYPE_CPUFREQ_DEVICE \
    (cpufreq_device_get_type ())
#define CPUFREQ_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_CPUFREQ_DEVICE, CpufreqDevice))
#define CPUFREQ_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_CPUFREQ_DEVICE, CpufreqDeviceClass))
#define IS_CPUFREQ_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_CPUFREQ_DEVICE))
#define IS_CPUFREQ_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_CPUFREQ_DEVICE))
#define CPUFREQ_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_CPUFREQ_DEVICE, CpufreqDeviceClass))

G_BEGIN_DECLS

typedef struct _CpufreqDevice CpufreqDevice;
typedef struct _CpufreqDeviceClass CpufreqDeviceClass;
typedef struct _CpufreqDevicePrivate CpufreqDevicePrivate;

struct _CpufreqDevice {
    FreqDevice parent;
    CpufreqDevicePrivate *priv;
};

struct _CpufreqDeviceClass {
    FreqDeviceClass parent_class;
};

GType           cpufreq_device_get_type         (void) G_GNUC_CONST;

GObject*        cpufreq_device_new              (void);

G_END_DECLS

#endif

