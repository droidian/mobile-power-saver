/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "devfreq.h"
#include "devfreq_device.h"
#include "../common/define.h"
#include "../common/utils.h"

struct _DevfreqPrivate {
    GList *devfreq_devices;
};

G_DEFINE_TYPE_WITH_CODE (
    Devfreq,
    devfreq,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (Devfreq)
)

static void
detect_devices (Devfreq *self)
{
    g_autoptr(GDir) devfreq_dir = NULL;
    const char *device_dir;

    devfreq_dir = g_dir_open (DEVFREQ_DIR, 0, NULL);
    if (devfreq_dir == NULL) {
        g_warning ("No devfreq sysfs dir: %s", DEVFREQ_DIR);
        return;
    }

    while ((device_dir = g_dir_read_name (devfreq_dir)) != NULL) {
        DevfreqDevice *devfreq_device = DEVFREQ_DEVICE (devfreq_device_new ());
        g_autofree gchar *filename = g_build_filename (
            DEVFREQ_DIR, device_dir, "governor", NULL
        );

        if (!g_file_test (filename, G_FILE_TEST_EXISTS))
            continue;

        freq_device_set_name (FREQ_DEVICE (devfreq_device), device_dir);

        self->priv->devfreq_devices = g_list_prepend (
            self->priv->devfreq_devices, devfreq_device
        );
    }
}

static void
devfreq_dispose (GObject *devfreq)
{
    G_OBJECT_CLASS (devfreq_parent_class)->dispose (devfreq);
}

static void
devfreq_finalize (GObject *devfreq)
{
    Devfreq *self = DEVFREQ (devfreq);

    g_list_free_full (self->priv->devfreq_devices, g_object_unref);

    G_OBJECT_CLASS (devfreq_parent_class)->finalize (devfreq);
}

static void
devfreq_class_init (DevfreqClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = devfreq_dispose;
    object_class->finalize = devfreq_finalize;
}

static void
devfreq_init (Devfreq *self)
{
    self->priv = devfreq_get_instance_private (self);

    self->priv->devfreq_devices = NULL;

    detect_devices (self);
}

/**
 * devfreq_new:
 *
 * Creates a new #Devfreq
 *
 * Returns: (transfer full): a new #Devfreq
 *
 **/
GObject *
devfreq_new (void)
{
    GObject *devfreq;

    devfreq = g_object_new (TYPE_DEVFREQ, NULL);

    return devfreq;
}

/**
 * devfreq_set_powersave:
 *
 * Set devfreq devices to powersave
 *
 * @param #Devfreq
 * @param powersave: True to enable powersave
 */
void
devfreq_set_powersave (Devfreq  *devfreq,
                       gboolean  powersave) {
    DevfreqDevice *devfreq_device;

    GFOREACH (devfreq->priv->devfreq_devices, devfreq_device)
        freq_device_set_powersave (FREQ_DEVICE (devfreq_device), powersave);
}

/**
 * devfreq_set_governor:
 *
 * Set devfreq devices governor
 *
 * @param #Devfreq
 * @param governor: new governor to set
 */
void
devfreq_set_governor (Devfreq     *devfreq,
                      const gchar *governor) {
    DevfreqDevice *devfreq_device;

    GFOREACH (devfreq->priv->devfreq_devices, devfreq_device)
        freq_device_set_governor (FREQ_DEVICE (devfreq_device), governor);
}
