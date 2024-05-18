/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef LOGIND_H
#define LOGIND_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_LOGIND \
    (logind_get_type ())
#define LOGIND(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_LOGIND, Logind))
#define LOGIND_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_LOGIND, LogindClass))
#define IS_LOGIND(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_LOGIND))
#define IS_LOGIND_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_LOGIND))
#define LOGIND_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_LOGIND, LogindClass))

G_BEGIN_DECLS

typedef struct _Logind Logind;
typedef struct _LogindClass LogindClass;
typedef struct _LogindPrivate LogindPrivate;

struct _Logind {
    GObject parent;
    LogindPrivate *priv;
};

struct _LogindClass {
    GObjectClass parent_class;
};

GType           logind_get_type            (void) G_GNUC_CONST;

GObject*        logind_new                 (void);
Logind*         logind_get_default         (void);

G_END_DECLS

#endif

