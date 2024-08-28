/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef MODEM_OFONO_H
#define MODEM_OFONO_H

#include <glib.h>
#include <glib-object.h>

#include "modem.h"

#define TYPE_MODEM_OFONO \
    (modem_ofono_get_type ())
#define MODEM_OFONO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_MODEM_OFONO, ModemOfono))
#define MODEM_OFONO_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_MODEM_OFONO, ModemOfonoClass))
#define IS_MODEM_OFONO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_MODEM_OFONO))
#define IS_MODEM_OFONO_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_MODEM_OFONO))
#define MODEM_OFONO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_MODEM_OFONO, ModemOfonoClass))

G_BEGIN_DECLS

typedef struct _ModemOfono ModemOfono;
typedef struct _ModemOfonoClass ModemOfonoClass;
typedef struct _ModemOfonoPrivate ModemOfonoPrivate;

struct _ModemOfono {
    Modem parent;
    ModemOfonoPrivate *priv;
};

struct _ModemOfonoClass {
    ModemClass parent_class;
};

GType           modem_ofono_get_type      (void) G_GNUC_CONST;

GObject*        modem_ofono_new           (void);

G_END_DECLS

#endif

