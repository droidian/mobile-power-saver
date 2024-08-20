/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <glib.h>
#include <glib-object.h>

typedef enum
{
    MODEM_USAGE_LOW,
    MODEM_USAGE_MEDIUM,
    MODEM_USAGE_HIGH
} ModemUsage;

#define TYPE_NETWORK_MANAGER \
    (network_manager_get_type ())
#define NETWORK_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_NETWORK_MANAGER, NetworkManager))
#define NETWORK_MANAGER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_NETWORK_MANAGER, NetworkManagerClass))
#define IS_NETWORK_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_NETWORK_MANAGER))
#define IS_NETWORK_MANAGER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_NETWORK_MANAGER))
#define NETWORK_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_NETWORK_MANAGER, NetworkManagerClass))

G_BEGIN_DECLS

typedef struct _NetworkManager NetworkManager;
typedef struct _NetworkManagerClass NetworkManagerClass;
typedef struct _NetworkManagerPrivate NetworkManagerPrivate;

struct _NetworkManager {
    GObject parent;
    NetworkManagerPrivate *priv;
};

struct _NetworkManagerClass {
    GObjectClass parent_class;
};

GType           network_manager_get_type               (void) G_GNUC_CONST;

GObject*        network_manager_new                    (void);
void            network_manager_start_modem_monitoring (NetworkManager *self);
void            network_manager_stop_modem_monitoring  (NetworkManager *self);
ModemUsage      network_manager_get_modem_usage        (NetworkManager *self);
G_END_DECLS

#endif

