/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef FREQ_DEVICE_H
#define FREQ_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_FREQ_DEVICE \
    (freq_device_get_type ())
#define FREQ_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_FREQ_DEVICE, FreqDevice))
#define FREQ_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_FREQ_DEVICE, FreqDeviceClass))
#define IS_FREQ_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_FREQ_DEVICE))
#define IS_FREQ_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_FREQ_DEVICE))
#define FREQ_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_FREQ_DEVICE, FreqDeviceClass))

G_BEGIN_DECLS

typedef struct _FreqDevice FreqDevice;
typedef struct _FreqDeviceClass FreqDeviceClass;
typedef struct _FreqDevicePrivate FreqDevicePrivate;

struct _FreqDevice {
    GObject parent;
    FreqDevicePrivate *priv;
};

struct _FreqDeviceClass {
    GObjectClass parent_class;
    void (*set_sysfs_dir) (FreqDevice *instance);
};

GType           freq_device_get_type            (void) G_GNUC_CONST;

GObject*        freq_device_new                 (void);
void            freq_device_set_sysfs_settings  (FreqDevice  *self,
                                                 const gchar *directory,
                                                 const gchar *governor_node);
void            freq_device_set_name            (FreqDevice  *self,
                                                 const gchar *device_name);
const gchar*    freq_device_get_name            (FreqDevice  *self);
void            freq_device_set_powersave       (FreqDevice  *self,
                                                 gboolean     powersave);
void            freq_device_set_governor        (FreqDevice  *self,
                                                 const gchar *governor);
G_END_DECLS

#endif

