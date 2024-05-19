/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef FREEZER_H
#define FREEZER_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_FREEZER \
    (freezer_get_type ())
#define FREEZER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_FREEZER, Freezer))
#define FREEZER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_FREEZER, FreezerClass))
#define IS_FREEZER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_FREEZER))
#define IS_FREEZER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_FREEZER))
#define FREEZER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_FREEZER, FreezerClass))

G_BEGIN_DECLS

typedef struct _Freezer Freezer;
typedef struct _FreezerClass FreezerClass;
typedef struct _FreezerPrivate FreezerPrivate;

struct _Freezer {
    GObject parent;
    FreezerPrivate *priv;
};

struct _FreezerClass {
    GObjectClass parent_class;
};

GType           freezer_get_type            (void) G_GNUC_CONST;

GObject*        freezer_new                 (void);
void            freezer_suspend_processes   (Freezer *freezer,
                                             GList   *names);
void            freezer_resume_processes    (Freezer *freezer,
                                             GList   *names);
G_END_DECLS

#endif

