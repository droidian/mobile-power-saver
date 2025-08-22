/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <gio/gio.h>

#include "processes.h"
#include "../common/utils.h"

#define MAX_BUFSZ (1024*64*2)
#define PROCPATHLEN 64  // must hold /proc/2000222000/task/2000222000/cmdline

struct _ProcessesPrivate {
    GList *processes;

    GList *cpuset_blacklist;
    GList *cpuset_topapp;
};

G_DEFINE_TYPE_WITH_CODE (
    Processes,
    processes,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Processes)
)

struct Process {
    pid_t pid;
    char *cmdline;
};

static const char*
get_cpuset_path (CpuSet cpuset)
{
    switch (cpuset) {
    case CPUSET_BACKGROUND:
        return "/dev/cpuset/background/tasks";
    case CPUSET_SYSTEM_BACKGROUND:
        return "/dev/cpuset/system-background/tasks";
    case CPUSET_FOREGROUND:
        return "/dev/cpuset/foreground/tasks";
    case CPUSET_TOPAPP:
    default:
        return "/dev/cpuset/top-app/tasks";
    }
}

static void
process_free (gpointer user_data)
{
    struct Process *process = user_data;

    g_free (process->cmdline);
    g_free (process);
}

// From https://gitlab.com/procps-ng/procps
//
static int read_unvectored(char *restrict const  dst,
                           unsigned              sz,
                           const char           *whom,
                           const char           *what,
                           char                  sep)
{
    char path[PROCPATHLEN];
    int fd, len;
    unsigned n = 0;

    if(sz <= 0) return 0;
    if(sz >= INT_MAX) sz = INT_MAX-1;
    dst[0] = '\0';

    len = snprintf(path, sizeof(path), "%s/%s", whom, what);
    if(len <= 0 || (size_t)len >= sizeof(path)) return 0;
    fd = open(path, O_RDONLY);
    if(fd==-1) return 0;

    for(;;){
        ssize_t r = read(fd,dst+n,sz-n);
        if(r==-1){
            if(errno==EINTR) continue;
            break;
        }
        if(r<=0) break;  // EOF
        n += r;
        if(n==sz) {      // filled the buffer
            --n;         // make room for '\0'
            break;
        }
    }
    close(fd);
    if(n){
        unsigned i = n;
        while(i && dst[i-1]=='\0') --i; // skip trailing zeroes
        while(i--)
            if(dst[i]=='\n' || dst[i]=='\0') dst[i]=sep;
        if(dst[n-1]==' ') dst[n-1]='\0';
    }
    dst[n] = '\0';
    return n;
}

static gboolean
process_in_list (GList          *names,
                 struct Process *process)
{
    char *name;

    if (g_strcmp0 (process->cmdline, "") == 0)
        return FALSE;

    GFOREACH (names, name) {
        if (g_strrstr (process->cmdline, name) != NULL)
            return TRUE;
    }
    return FALSE;
}

static GList *
get_processes (Processes *self)
{
    GList *processes = NULL;
    g_autoptr (GDir) proc_dir = NULL;
    const char *pid_dir;

    proc_dir = g_dir_open ("/proc", 0, NULL);
    if (proc_dir == NULL) {
        g_warning ("/proc not found");
        return NULL;
    }

    while ((pid_dir = g_dir_read_name (proc_dir)) != NULL) {
        char contents[MAX_BUFSZ] = {0};
        g_autofree char *directory = NULL;

        // Skip non-numeric entries
        if (!g_ascii_isdigit (pid_dir[0]))
            continue;

        directory = g_build_filename (
            "/proc", pid_dir, NULL
        );

        if (read_unvectored(contents, MAX_BUFSZ, directory, "cmdline", ' ')) {
            struct Process *process = g_malloc (sizeof (struct Process));

            process->cmdline = g_strdup (contents);
            sscanf (pid_dir, "%d", &process->pid);

            processes = g_list_prepend (processes, process);
        }
    }

    return processes;
}

static void
processes_dispose (GObject *processes)
{
    G_OBJECT_CLASS (processes_parent_class)->dispose (processes);
}

static void
processes_finalize (GObject *processes)
{
    Processes *self = PROCESSES (processes);

    g_list_free_full (self->priv->processes, process_free);
    g_list_free_full (self->priv->cpuset_blacklist, g_free);
    g_list_free_full (self->priv->cpuset_topapp, g_free);

    G_OBJECT_CLASS (processes_parent_class)->finalize (processes);
}

static void
processes_class_init (ProcessesClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = processes_dispose;
    object_class->finalize = processes_finalize;
}

static void
processes_init (Processes *self)
{
    self->priv = processes_get_instance_private (self);

    self->priv->processes = NULL;
    self->priv->cpuset_blacklist = NULL;
    self->priv->cpuset_topapp = NULL;
}

/**
 * processes_new:
 *
 * Creates a new #Processes
 *
 * Returns: (transfer full): a new #Processes
 *
 **/
GObject *
processes_new (void)
{
    GObject *processes;

    processes = g_object_new (TYPE_PROCESSES, NULL);

    return processes;
}

/**
 * processes_update:
 *
 * Update processes list
 *
 * @param #Processes
 */
void
processes_update (Processes *self) {
    g_list_free_full (self->priv->processes, g_free);
    self->priv->processes = get_processes (self);
}

/**
 * processes_suspend:
 *
 * Suspend processes in list
 *
 * @param #Processes
 * @param processes: processes list
 */
void
processes_suspend (Processes *self,
                   GList     *processes) {

    struct Process *process;

    g_return_if_fail (self->priv->processes != NULL);
    g_return_if_fail (processes != NULL);

    GFOREACH (self->priv->processes, process)
        if (process_in_list (processes, process))
            kill (process->pid, SIGSTOP);
}

/**
 * processes_resume:
 *
 * resume processes
 *
 * @param #Processes
 * @param processes: processes list
 *
 */
void
processes_resume (Processes *self,
                  GList     *processes) {

    struct Process *process;

    g_return_if_fail (self->priv->processes != NULL);
    g_return_if_fail (processes != NULL);

    GFOREACH (self->priv->processes, process)
        if (process_in_list (processes, process))
            kill (process->pid, SIGCONT);
}

/**
 * processes_names_set_cpuset:
 *
 * Move processes to cpuset
 *
 * @param #Processes
 * @param names: processes list
 * @param #CpuSet
 *
 */
void  processes_set_cpuset (Processes *self,
                            GList     *processes,
                            CpuSet     cpuset) {
    struct Process *process;
    const char *cpuset_path;

    g_return_if_fail (self->priv->processes != NULL);
    g_return_if_fail (processes != NULL);

    cpuset_path = get_cpuset_path (cpuset);

    GFOREACH (self->priv->processes, process) {
        if (process_in_list (processes, process)) {
            g_autofree char *pid_str = g_strdup_printf("%d", process->pid);

            write_to_file (cpuset_path, pid_str);
        }
    }
}

/**
 * processes_set_cgroup_cpuset:
 *
 * Move service to background cpuset
 *
 * @param #Processes
 * @param cgroup_path: cgroup path
 * @param items: items in cgroup
 * @param #CpuSet
 *
 */
void  processes_set_cgroup_cpuset (Processes  *self,
                                   const char *cgroup_path,
                                   GList      *items,
                                   CpuSet      cpuset) {
    const char *item;

    g_return_if_fail (items != NULL);

    GFOREACH (items, item) {
        GList *pids = NULL;
        pid_t *pid;
        const char *name;
        const char *cpuset_path = NULL;
        char *cgroup_procs = NULL;
        GList *slices = NULL;

        GFOREACH_SUB (self->priv->cpuset_blacklist, name) {
            if (g_strrstr (item, name) != NULL) {
                goto end_loop;
            }
        }

        slices = get_cgroup_slices (cgroup_path);
        GFOREACH_SUB (slices, name) {
            cgroup_procs = g_build_filename (
                name, item, "cgroup.procs", NULL
            );
            if (g_file_test (cgroup_procs, G_FILE_TEST_EXISTS))
                break;
            g_free (cgroup_procs);
            cgroup_procs = NULL;
        }

        if (cgroup_procs == NULL)
            continue;

        if (cpuset == CPUSET_FOREGROUND) {
            GFOREACH_SUB (self->priv->cpuset_topapp, name) {
                if (g_strcmp0 (item, name) == 0) {
                    cpuset_path = get_cpuset_path (CPUSET_TOPAPP);
                    break;
                }
            }
        }

        if (cpuset_path == NULL)
            cpuset_path = get_cpuset_path (cpuset);

        pids = get_cgroup_pids (cgroup_procs);
        GFOREACH_SUB (pids, pid) {
            g_autofree char *pid_str = g_strdup_printf("%d", *pid);

            write_to_file (cpuset_path, pid_str);
        }
        g_list_free_full (pids, g_free);
        g_list_free_full (slices, g_free);
        g_free (cgroup_procs);
end_loop:
    }
}


/**
 * processes_cpuset_set_blacklist:
 *
 * Set cpuset blacklist
 *
 * @param #Processes
 * @param blacklist: cgroup blacklist
 *
 */
void
processes_cpuset_set_blacklist (Processes *self,
                                GList     *blacklist)
{
    g_list_free_full (self->priv->cpuset_blacklist, g_free);

    self->priv->cpuset_blacklist = blacklist;
}

/**
 * processes_cpuset_set_topapp:
 *
 * Set top-app cpuset
 *
 * @param #Processes
 * @param blacklist: cgroup blacklist
 *
 */
void
processes_cpuset_set_topapp (Processes *self,
                             GList     *topapp)
{
    g_list_free_full (self->priv->cpuset_topapp, g_free);

    self->priv->cpuset_topapp = topapp;
}
