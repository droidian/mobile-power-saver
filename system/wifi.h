/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef WIFI_H
#define WIFI_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_WIFI \
    (wifi_get_type ())
#define WIFI(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_WIFI, WiFi))
#define WIFI_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_WIFI, WiFiClass))
#define IS_WIFI(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_WIFI))
#define IS_WIFI_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_WIFI))
#define WIFI_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_WIFI, WiFiClass))

G_BEGIN_DECLS

typedef struct _WiFi WiFi;
typedef struct _WiFiClass WiFiClass;
typedef struct _WiFiPrivate WiFiPrivate;

struct _WiFi {
    GObject parent;
    WiFiPrivate *priv;
};

struct _WiFiClass {
    GObjectClass parent_class;
};

GType           wifi_get_type      (void) G_GNUC_CONST;

GObject*        wifi_new           (void);
void            wifi_set_powersave (WiFi     *wifi,
                                    gboolean  powersave);

G_END_DECLS

#endif

