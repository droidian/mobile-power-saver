/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef KERNEL_SETTINGS_H
#define KERNEL_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_KERNEL_SETTINGS \
    (kernel_settings_get_type ())
#define KERNEL_SETTINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_KERNEL_SETTINGS, KernelSettings))
#define KERNEL_SETTINGS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_KERNEL_SETTINGS, KernelSettingsClass))
#define IS_KERNEL_SETTINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_KERNEL_SETTINGS))
#define IS_KERNEL_SETTINGS_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_KERNEL_SETTINGS))
#define KERNEL_SETTINGS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_KERNEL_SETTINGS, KernelSettingsClass))

G_BEGIN_DECLS

typedef struct _KernelSettings KernelSettings;
typedef struct _KernelSettingsClass KernelSettingsClass;
typedef struct _KernelSettingsPrivate KernelSettingsPrivate;

struct _KernelSettings {
    GObject parent;
    KernelSettingsPrivate *priv;
};

struct _KernelSettingsClass {
    GObjectClass parent_class;
};

GType           kernel_settings_get_type      (void) G_GNUC_CONST;

GObject*        kernel_settings_new           (void);
void            kernel_settings_set_powersave (KernelSettings *kernel_settings,
                                               gboolean        powersave);

G_END_DECLS

#endif

