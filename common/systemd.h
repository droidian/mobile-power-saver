/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef SYSTEMD_H
#define SYSTEMD_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_SYSTEMD \
    (systemd_get_type ())
#define SYSTEMD(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_SYSTEMD, Systemd))
#define SYSTEMD_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_SYSTEMD, SystemdClass))
#define IS_SYSTEMD(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_SYSTEMD))
#define IS_SYSTEMD_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_SYSTEMD))
#define SYSTEMD_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_SYSTEMD, SystemdClass))

G_BEGIN_DECLS

typedef struct _Systemd Systemd;
typedef struct _SystemdClass SystemdClass;
typedef struct _SystemdPrivate SystemdPrivate;

struct _Systemd {
    GObject parent;
    SystemdPrivate *priv;
};

struct _SystemdClass {
    GObjectClass parent_class;
};

GType           systemd_get_type            (void) G_GNUC_CONST;

GObject*        systemd_new                 (GBusType bus_type);
void            systemd_start               (Systemd *self,
                                             GList   *services);
void            systemd_stop                (Systemd *self,
                                             GList   *services);
G_END_DECLS

#endif

