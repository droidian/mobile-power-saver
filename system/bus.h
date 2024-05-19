/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef BUS_H
#define BUS_H

#include <glib.h>
#include <glib-object.h>

typedef enum {
  POWER_PROFILE_POWER_SAVER,
  POWER_PROFILE_BALANCED,
  POWER_PROFILE_PERFORMANCE,
  POWER_PROFILE_LAST
} PowerProfile;

#define TYPE_BUS (bus_get_type ())

#define BUS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_BUS, Bus))
#define BUS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_BUS, BusClass))
#define IS_BUS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_BUS))
#define IS_BUS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_BUS))
#define BUS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_BUS, BusClass))

G_BEGIN_DECLS

typedef struct _Bus Bus;
typedef struct _BusClass BusClass;
typedef struct _BusPrivate BusPrivate;

struct _Bus {
    GObject parent;
    BusPrivate *priv;
};

struct _BusClass {
    GObjectClass parent_class;
};

GType       bus_get_type             (void) G_GNUC_CONST;
GObject*    bus_new                  (void);
Bus        *bus_get_default          (void);
void        bus_screen_state_changed (Bus      *self,
                                      gboolean  enabled);
void        bus_free_default         (void);

G_END_DECLS

#endif
