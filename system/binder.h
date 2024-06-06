/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef BINDER_H
#define BINDER_H

#include <glib.h>
#include <glib-object.h>

#include "../common/define.h"

#define TYPE_BINDER \
    (binder_get_type ())
#define BINDER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_BINDER, Binder))
#define BINDER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_BINDER, BinderClass))
#define IS_BINDER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_BINDER))
#define IS_BINDER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_BINDER))
#define BINDER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_BINDER, BinderClass))

G_BEGIN_DECLS

typedef struct _Binder Binder;
typedef struct _BinderClass BinderClass;
typedef struct _BinderPrivate BinderPrivate;

struct _Binder {
    GObject parent;
    BinderPrivate *priv;
};

struct _BinderClass {
    GObjectClass parent_class;
};

GType           binder_get_type            (void) G_GNUC_CONST;

GObject*        binder_new                 (void);
void            binder_set_power_profile   (Binder   *self,
                                            PowerProfile power_profile);
void            binder_set_powersave       (Binder  *self,
                                            gboolean powersave);
G_END_DECLS

#endif

