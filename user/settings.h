/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_SETTINGS \
    (settings_get_type ())
#define SETTINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_SETTINGS, Settings))
#define SETTINGS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_SETTINGS, SettingsClass))
#define IS_SETTINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_SETTINGS))
#define IS_SETTINGS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_SETTINGS))
#define SETTINGS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_SETTINGS, SettingsClass))

G_BEGIN_DECLS

typedef struct _Settings Settings;
typedef struct _SettingsClass SettingsClass;
typedef struct _SettingsPrivate SettingsPrivate;

struct _Settings {
    GObject parent;
    SettingsPrivate *priv;
};

struct _SettingsClass {
    GObjectClass parent_class;
};

GType           settings_get_type                      (void) G_GNUC_CONST;
Settings       *settings_get_default                   (void);
GObject*        settings_new                           (void);
G_END_DECLS

#endif
