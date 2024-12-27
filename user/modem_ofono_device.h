/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef MODEM_OFONO_DEVICE_H
#define MODEM_OFONO_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#include "modem.h"

#define TYPE_MODEM_OFONO_DEVICE \
    (modem_ofono_device_get_type ())
#define MODEM_OFONO_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_MODEM_OFONO_DEVICE, ModemOfonoDevice))
#define MODEM_OFONO_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_MODEM_OFONO_DEVICE, ModemOfonoDeviceClass))
#define IS_MODEM_OFONO_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_MODEM_OFONO_DEVICE))
#define IS_MODEM_OFONO_DEVICE_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_MODEM_OFONO_DEVICE))
#define MODEM_OFONO_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_MODEM_OFONO_DEVICE, ModemOfonoDeviceClass))

G_BEGIN_DECLS

typedef struct _ModemOfonoDevice ModemOfonoDevice;
typedef struct _ModemOfonoDeviceClass ModemOfonoDeviceClass;
typedef struct _ModemOfonoDevicePrivate ModemOfonoDevicePrivate;

struct _ModemOfonoDevice {
    Modem parent;
    ModemOfonoDevicePrivate *priv;
};

struct _ModemOfonoDeviceClass {
    ModemClass parent_class;
};

GType           modem_ofono_device_get_type        (void) G_GNUC_CONST;

GObject*        modem_ofono_device_new             (const char      *path);
const char*    modem_ofono_device_get_path         (ModemOfonoDevice *self);
void            modem_ofono_device_apply_powersave (ModemOfonoDevice *self,
                                                    gboolean          powersave);
G_END_DECLS

#endif


