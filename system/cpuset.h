/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef CPUSET_H
#define CPUSET_H

#include <glib.h>
#include <glib-object.h>
#include "../common/define.h"

#define TYPE_CPUSET \
    (cpuset_get_type ())
#define CPUSET(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_CPUSET, Cpuset))
#define CPUSET_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_CPUSET, CpusetClass))
#define IS_CPUSET(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_CPUSET))
#define IS_CPUSET_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_CPUSET))
#define CPUSET_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_CPUSET, CpusetClass))

G_BEGIN_DECLS

typedef struct _Cpuset Cpuset;
typedef struct _CpusetClass CpusetClass;
typedef struct _CpusetPrivate CpusetPrivate;

struct _Cpuset {
    GObject parent;
    CpusetPrivate *priv;
};

struct _CpusetClass {
    GObjectClass parent_class;
};

GType           cpuset_get_type                     (void) G_GNUC_CONST;

GObject*        cpuset_new                          (void);
void            cpuset_set_powersave                (Cpuset   *self,
                                                     gboolean  powersave);

G_END_DECLS

#endif

