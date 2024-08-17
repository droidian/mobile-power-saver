/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef MODEM_H
#define MODEM_H

#include <glib.h>
#include <glib-object.h>

typedef enum
{
    MODEM_POWERSAVE_NONE    = 0,
    MODEM_POWERSAVE_ENABLED = 1 << 0,
    MODEM_POWERSAVE_WIFI    = 1 << 1,
    MODEM_POWERSAVE_DOZING  = 1 << 2
} ModemPowersave;

#define TYPE_MODEM \
    (modem_get_type ())
#define MODEM(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_MODEM, Modem))
#define MODEM_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_MODEM, ModemClass))
#define IS_MODEM(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_MODEM))
#define IS_MODEM_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_MODEM))
#define MODEM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_MODEM, ModemClass))

G_BEGIN_DECLS

typedef struct _Modem Modem;
typedef struct _ModemClass ModemClass;
typedef struct _ModemPrivate ModemPrivate;

struct _Modem {
    GObject parent;
    ModemPrivate *priv;
};

struct _ModemClass {
    GObjectClass parent_class;
    void (*apply_powersave) (Modem *self);
};

GType           modem_get_type        (void) G_GNUC_CONST;

GObject*        modem_new             (void);
gboolean        modem_set_powersave   (Modem          *self,
                                       gboolean        powersave,
                                       ModemPowersave  modem_powersave);
ModemPowersave  modem_get_powersave   (Modem          *self);
void            modem_reset_powersave (Modem          *self);

G_END_DECLS

#endif

