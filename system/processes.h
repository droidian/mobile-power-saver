/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef PROCESSES_H
#define PROCESSES_H

#include <glib.h>
#include <glib-object.h>
#include "../common/define.h"

#define TYPE_PROCESSES \
    (processes_get_type ())
#define PROCESSES(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_PROCESSES, Processes))
#define PROCESSES_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_PROCESSES, ProcessesClass))
#define IS_PROCESSES(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_PROCESSES))
#define IS_PROCESSES_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_PROCESSES))
#define PROCESSES_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_PROCESSES, ProcessesClass))

G_BEGIN_DECLS

typedef struct _Processes Processes;
typedef struct _ProcessesClass ProcessesClass;
typedef struct _ProcessesPrivate ProcessesPrivate;

struct _Processes {
    GObject parent;
    ProcessesPrivate *priv;
};

struct _ProcessesClass {
    GObjectClass parent_class;
};

GType           processes_get_type                     (void) G_GNUC_CONST;

GObject*        processes_new                          (void);
void            processes_update                       (Processes  *self);
void            processes_suspend                      (Processes  *self,
                                                        GList      *processes);
void            processes_resume                       (Processes  *self,
                                                        GList      *processes);
void            processes_set_cpuset                   (Processes  *self,
                                                        GList      *processes,
                                                        CpuSet     cpuset);
void            processes_set_cgroup_cpuset            (Processes  *self,
                                                        const char *cgroup_path,
                                                        GList      *items,
                                                        CpuSet      cpuset);
void            processes_cpuset_set_blacklist         (Processes  *self,
                                                        GList      *blacklist);
void            processes_cpuset_set_topapp            (Processes  *self,
                                                        GList      *topapp);

G_END_DECLS

#endif

