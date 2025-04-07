/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef NETWORK_MANAGER_MODEM_H
#define NETWORK_MANAGER_MODEM_H

#include <glib.h>
#include <glib-object.h>

typedef enum
{
    BANDWIDTH_LOW,
    BANDWIDTH_MEDIUM
} Bandwidth;

#define TYPE_NETWORK_MANAGER_MODEM \
    (network_manager_modem_get_type ())
#define NETWORK_MANAGER_MODEM(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_NETWORK_MANAGER_MODEM, NetworkManagerModem))
#define NETWORK_MANAGER_MODEM_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_NETWORK_MANAGER_MODEM, NetworkManagerModemClass))
#define IS_NETWORK_MANAGER_MODEM(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_NETWORK_MANAGER_MODEM))
#define IS_NETWORK_MANAGER_MODEM_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_NETWORK_MANAGER_MODEM))
#define NETWORK_MANAGER_MODEM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_NETWORK_MANAGER_MODEM, NetworkManagerModemClass))

G_BEGIN_DECLS

typedef struct _NetworkManagerModem NetworkManagerModem;
typedef struct _NetworkManagerModemClass NetworkManagerModemClass;
typedef struct _NetworkManagerModemPrivate NetworkManagerModemPrivate;

struct _NetworkManagerModem {
    GObject parent;
    NetworkManagerModemPrivate *priv;
};

struct _NetworkManagerModemClass {
    GObjectClass parent_class;
};

GType           network_manager_modem_get_type         (void) G_GNUC_CONST;

GObject*        network_manager_modem_new              (void);
void            network_manager_modem_start_monitoring (NetworkManagerModem *self);
void            network_manager_modem_stop_monitoring  (NetworkManagerModem *self);
gboolean        network_manager_modem_data_used        (NetworkManagerModem *self);
G_END_DECLS

#endif
