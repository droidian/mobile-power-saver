/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef DEVFREQ_H
#define DEVFREQ_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_DEVFREQ \
    (devfreq_get_type ())
#define DEVFREQ(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_DEVFREQ, Devfreq))
#define DEVFREQ_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_DEVFREQ, DevfreqClass))
#define IS_DEVFREQ(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_DEVFREQ))
#define IS_DEVFREQ_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_DEVFREQ))
#define DEVFREQ_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_DEVFREQ, DevfreqClass))

G_BEGIN_DECLS

typedef struct _Devfreq Devfreq;
typedef struct _DevfreqClass DevfreqClass;
typedef struct _DevfreqPrivate DevfreqPrivate;

struct _Devfreq {
    GObject parent;
    DevfreqPrivate *priv;
};

struct _DevfreqClass {
    GObjectClass parent_class;
};

GType           devfreq_get_type            (void) G_GNUC_CONST;

GObject*        devfreq_new                 (void);
void            devfreq_blacklist           (Devfreq    *self,
                                             const char *device_name);
void            devfreq_set_powersave       (Devfreq     *self,
                                             gboolean     powersave);
void            devfreq_set_governor        (Devfreq    *self,
                                             const char *governor);
G_END_DECLS

#endif

