/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef BINDER_CLIENT_H
#define BINDER_CLIENT_H

#include <glib.h>
#include <glib-object.h>

#include "../common/define.h"

#define TYPE_BINDER_CLIENT \
    (binder_client_get_type ())
#define BINDER_CLIENT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_BINDER_CLIENT, BinderClient))
#define BINDER_CLIENT_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_BINDER_CLIENT, BinderClientClass))
#define IS_BINDER_CLIENT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_BINDER_CLIENT))
#define IS_BINDER_CLIENT_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_BINDER_CLIENT))
#define BINDER_CLIENT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_BINDER_CLIENT, BinderClientClass))

G_BEGIN_DECLS

typedef struct _BinderClient BinderClient;
typedef struct _BinderClientClass BinderClientClass;
typedef struct _BinderClientPrivate BinderClientPrivate;

struct _BinderClient {
    GObject parent;

    GBinderClient *client;
    BinderServiceManagerType type;

    BinderClientPrivate *priv;
};

struct _BinderClientClass {
    GObjectClass parent_class;

    void (*init_binder)       (BinderClient *self,
                               const gchar *hidl_service,
                               const gchar *hidl_client,
                               const gchar *aidl_service,
                               const gchar *aidl_client);

    void (*set_power_profile) (BinderClient *self,
                               PowerProfile  power_profile);
};

GType           binder_client_get_type            (void) G_GNUC_CONST;

GObject*        binder_client_new                 (void);
void            binder_client_set_power_profile   (BinderClient *self,
                                                   PowerProfile  power_profile);
G_END_DECLS

#endif

