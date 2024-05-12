/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef DOZING_H
#define DOZING_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_DOZING \
    (dozing_get_type ())
#define DOZING(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_DOZING, Dozing))
#define DOZING_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_DOZING, DozingClass))
#define IS_DOZING(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_DOZING))
#define IS_DOZING_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_DOZING))
#define DOZING_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_DOZING, DozingClass))

G_BEGIN_DECLS

typedef struct _Dozing Dozing;
typedef struct _DozingClass DozingClass;
typedef struct _DozingPrivate DozingPrivate;

struct _Dozing {
    GObject parent;
    DozingPrivate *priv;
};

struct _DozingClass {
    GObjectClass parent_class;
};

GType           dozing_get_type            (void) G_GNUC_CONST;

GObject*        dozing_new                 (void);
void            dozing_start               (Dozing  *dozing);
void            dozing_stop                (Dozing  *dozing);
G_END_DECLS

#endif

