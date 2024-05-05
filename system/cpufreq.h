/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef CPUFREQ_H
#define CPUFREQ_H

#include <glib.h>
#include <glib-object.h>

#define TYPE_CPUFREQ \
    (cpufreq_get_type ())
#define CPUFREQ(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_CPUFREQ, Cpufreq))
#define CPUFREQ_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_CPUFREQ, CpufreqClass))
#define IS_CPUFREQ(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_CPUFREQ))
#define IS_CPUFREQ_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_CPUFREQ))
#define CPUFREQ_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_CPUFREQ, CpufreqClass))

G_BEGIN_DECLS

typedef struct _Cpufreq Cpufreq;
typedef struct _CpufreqClass CpufreqClass;
typedef struct _CpufreqPrivate CpufreqPrivate;

struct _Cpufreq {
    GObject parent;
    CpufreqPrivate *priv;
};

struct _CpufreqClass {
    GObjectClass parent_class;
};

GType           cpufreq_get_type            (void) G_GNUC_CONST;

GObject*        cpufreq_new                 (void);
void            cpufreq_set_powersave       (Cpufreq  *cpufreq,
                                             gboolean  powersave);
void            cpufreq_set_governor        (Cpufreq  *cpufreq,
                                             const gchar* governor);

G_END_DECLS

#endif

