/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef MPRIS_H
#define MPRIS_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_MPRIS (mpris_get_type ())

#define MPRIS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_MPRIS, Mpris))
#define MPRIS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_MPRIS, MprisClass))
#define IS_MPRIS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_MPRIS))
#define IS_MPRIS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_MPRIS))
#define MPRIS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_MPRIS, MprisClass))

G_BEGIN_DECLS

typedef struct _Mpris Mpris;
typedef struct _MprisClass MprisClass;
typedef struct _MprisPrivate MprisPrivate;

struct _Mpris {
    GObject parent;
    MprisPrivate *priv;
};

struct _MprisClass {
    GObjectClass parent_class;
};

GType       mpris_get_type       (void) G_GNUC_CONST;
Mpris      *mpris_get_default    (void);
GObject*    mpris_new            (void);
gboolean    mpris_can_freeze     (Mpris *self,
                                  const gchar *app_scope);

G_END_DECLS

#endif
