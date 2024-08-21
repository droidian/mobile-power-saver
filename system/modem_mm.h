/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef MODEM_MM_H
#define MODEM_MM_H

#include <glib.h>
#include <glib-object.h>

#include "modem.h"

#define TYPE_MODEM_MM \
    (modem_mm_get_type ())
#define MODEM_MM(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_MODEM_MM, ModemMM))
#define MODEM_MM_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_MODEM_MM, ModemMMClass))
#define IS_MODEM_MM(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_MODEM_MM))
#define IS_MODEM_MM_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_MODEM_MM))
#define MODEM_MM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_MODEM_MM, ModemMMClass))

G_BEGIN_DECLS

typedef struct _ModemMM ModemMM;
typedef struct _ModemMMClass ModemMMClass;
typedef struct _ModemMMPrivate ModemMMPrivate;

struct _ModemMM {
    Modem parent;
    ModemMMPrivate *priv;
};

struct _ModemMMClass {
    ModemClass parent_class;
};

GType           modem_mm_get_type      (void) G_GNUC_CONST;

GObject*        modem_mm_new           (void);

G_END_DECLS

#endif
