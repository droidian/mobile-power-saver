/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef MANAGER_H
#define MANAGER_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_MANAGER \
    (manager_get_type ())
#define MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_MANAGER, Manager))
#define MANAGER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_MANAGER, ManagerClass))
#define IS_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_MANAGER))
#define IS_MANAGER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_MANAGER))
#define MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_MANAGER, ManagerClass))

G_BEGIN_DECLS

typedef struct _Manager Manager;
typedef struct _ManagerClass ManagerClass;
typedef struct _ManagerPrivate ManagerPrivate;

struct _Manager {
    GObject parent;
    ManagerPrivate *priv;
};

struct _ManagerClass {
    GObjectClass parent_class;
};

GType           manager_get_type            (void) G_GNUC_CONST;

GObject*        manager_new                 (void);

G_END_DECLS

#endif

