/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "cpufreq.h"
#include "cpufreq_device.h"
#include "../common/define.h"
#include "../common/utils.h"

struct _CpufreqPrivate {
    GList *cpufreq_devices;
};

G_DEFINE_TYPE_WITH_CODE (
    Cpufreq,
    cpufreq,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Cpufreq)
)

static void
cpufreq_detect_devices (Cpufreq *self)
{
    g_autoptr(GDir) policies_dir = NULL;
    const char *policy_dir;

    policies_dir = g_dir_open (CPUFREQ_POLICIES_DIR, 0, NULL);
    if (policies_dir == NULL) {
        g_warning ("No cpufreq sysfs dir: %s", CPUFREQ_POLICIES_DIR);
        return;
    }

    while ((policy_dir = g_dir_read_name (policies_dir)) != NULL) {
        CpufreqDevice *cpufreq_device = CPUFREQ_DEVICE (cpufreq_device_new ());
        g_autofree gchar *filename = g_build_filename (
            CPUFREQ_POLICIES_DIR, policy_dir, "scaling_governor", NULL
        );

        if (!g_file_test (filename, G_FILE_TEST_EXISTS))
            continue;

        freq_device_set_name (FREQ_DEVICE (cpufreq_device), policy_dir);

        self->priv->cpufreq_devices = g_list_prepend (
            self->priv->cpufreq_devices, cpufreq_device
        );
    }
}

static void
cpufreq_dispose (GObject *cpufreq)
{
    G_OBJECT_CLASS (cpufreq_parent_class)->dispose (cpufreq);
}

static void
cpufreq_finalize (GObject *cpufreq)
{
    Cpufreq *self = CPUFREQ (cpufreq);

    g_list_free_full (self->priv->cpufreq_devices, g_object_unref);

    G_OBJECT_CLASS (cpufreq_parent_class)->finalize (cpufreq);
}

static void
cpufreq_class_init (CpufreqClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = cpufreq_dispose;
    object_class->finalize = cpufreq_finalize;
}

static void
cpufreq_init (Cpufreq *self)
{
    self->priv = cpufreq_get_instance_private (self);

    self->priv->cpufreq_devices = NULL;

    cpufreq_detect_devices (self);
}

/**
 * cpufreq_new:
 *
 * Creates a new #Cpufreq
 *
 * Returns: (transfer full): a new #Cpufreq
 *
 **/
GObject *
cpufreq_new (void)
{
    GObject *cpufreq;

    cpufreq = g_object_new (TYPE_CPUFREQ, NULL);

    return cpufreq;
}

/**
 * cpufreq_set_powersave:
 *
 * Set cpufreq devices to powersave
 *
 * @param #Cpufreq
 * @param powersave: True to enable powersave
 */
void
cpufreq_set_powersave (Cpufreq  *cpufreq,
                       gboolean  powersave) {
    CpufreqDevice *cpufreq_device;

    GFOREACH (cpufreq->priv->cpufreq_devices, cpufreq_device)
        freq_device_set_powersave (FREQ_DEVICE (cpufreq_device), powersave);
}

/**
 * cpufreq_set_governor:
 *
 * Set cpufreq devices governor
 *
 * @param #Cpufreq
 * @param governor: new governor to set
 */
void
cpufreq_set_governor (Cpufreq     *cpufreq,
                      const gchar *governor) {
    CpufreqDevice *cpufreq_device;

    GFOREACH (cpufreq->priv->cpufreq_devices, cpufreq_device)
        freq_device_set_governor (FREQ_DEVICE (cpufreq_device), governor);
}