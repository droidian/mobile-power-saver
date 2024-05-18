/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef SERVICES_H
#define SERVICES_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_SERVICES \
    (services_get_type ())
#define SERVICES(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_SERVICES, Services))
#define SERVICES_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_SERVICES, ServicesClass))
#define IS_SERVICES(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_SERVICES))
#define IS_SERVICES_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_SERVICES))
#define SERVICES_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_SERVICES, ServicesClass))

G_BEGIN_DECLS

typedef struct _Services Services;
typedef struct _ServicesClass ServicesClass;
typedef struct _ServicesPrivate ServicesPrivate;

struct _Services {
    GObject parent;
    ServicesPrivate *priv;
};

struct _ServicesClass {
    GObjectClass parent_class;
};

GType           services_get_type            (void) G_GNUC_CONST;

GObject*        services_new                 (GBusType bus_type);
void            services_freeze              (Services *self,
                                              GList   *services);
void            services_unfreeze            (Services *self,
                                              GList   *services);
G_END_DECLS

#endif

