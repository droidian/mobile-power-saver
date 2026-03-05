/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "freq_device.h"
#include "../common/utils.h"

struct _FreqDevicePrivate {
    char *sysfs_dir;
    char *device_name;

    const char* cur_node;
    const char *min_node;
    const char *max_node;

    char *min_freq;
    char *max_freq;
};

G_DEFINE_TYPE_WITH_CODE (
    FreqDevice,
    freq_device,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (FreqDevice)
)

static void
set_freq (FreqDevice *freq_device,
          const char *node,
          const char *freq)
{
    g_autofree char *filename = g_build_filename (
        freq_device->priv->sysfs_dir,
        freq_device->priv->device_name,
        freq_device->priv->cur_node,
        NULL
    );

    g_message ("%s -> %s", filename, freq);

    write_to_file (filename, freq);
}

static void
freq_device_dispose (GObject *freq_device)
{
    G_OBJECT_CLASS (freq_device_parent_class)->dispose (freq_device);
}

static void
freq_device_finalize (GObject *freq_device)
{
    FreqDevice *self = FREQ_DEVICE (freq_device);

    g_free (self->priv->min_freq);
    g_free (self->priv->max_freq);
    g_free (self->priv->device_name);
    g_free (self->priv->sysfs_dir);

    G_OBJECT_CLASS (freq_device_parent_class)->finalize (freq_device);
}

static void
freq_device_class_init (FreqDeviceClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = freq_device_dispose;
    object_class->finalize = freq_device_finalize;
}

static void
freq_device_init (FreqDevice *self)
{
    self->priv = freq_device_get_instance_private (self);

    self->priv->device_name = NULL;
    self->priv->sysfs_dir = NULL;
    self->priv->min_freq = NULL;
    self->priv->max_freq = NULL;
}

/**
 * freq_device_new:
 *
 * Creates a new #FreqDevice

 * Returns: (transfer full): a new #FreqDevice
 *
 **/
GObject *
freq_device_new (void)
{
    GObject *freq_device;

    freq_device = g_object_new (TYPE_FREQ_DEVICE, NULL);

    return freq_device;
}

/**
 * freq_device_set_sysfs_settings:
 *
 * Set #FreqDevice policy directory
 *
 * @self: #FreqDevice
 * @sys_dir: path to freq device policy dir
 * @min_node: min node name
 * @max_node: max node name
 *
 * Returns: (transfer full): a new #FreqDevice
 *
 **/
void
freq_device_set_sysfs_settings (FreqDevice *self,
                                const char *directory,
                                const char *cur_node,
                                const char *min_node,
                                const char *max_node)
{
    if (self->priv->sysfs_dir != NULL)
        g_free (self->priv->sysfs_dir);

    self->priv->sysfs_dir = g_strdup (directory);
    self->priv->cur_node = cur_node;
    self->priv->min_node = min_node;
    self->priv->max_node = max_node;
}

/**
 * freq_device_set_name:
 *
 * Set #FreqDevice device name
 *
 * @self: #FreqDevice
 * @device_name: device name
 *
 **/
void
freq_device_set_name (FreqDevice *self,
                      const char *device_name)
{
    g_autofree char *max, *min = NULL;
    g_autofree char *max_file = g_build_filename (
        self->priv->sysfs_dir, device_name, self->priv->max_node, NULL
    );
    g_autofree char *min_file = g_build_filename (
        self->priv->sysfs_dir, device_name, self->priv->min_node, NULL
    );

    g_return_if_fail (self->priv->device_name == NULL);
    g_return_if_fail (device_name != NULL);

    self->priv->device_name = g_strdup (device_name);

    if (g_file_get_contents (max_file, &max, NULL, NULL)) {
        max = g_strchomp (max);

        if (self->priv->max_freq != NULL)
            g_free (self->priv->max_freq);

        self->priv->max_freq = g_steal_pointer (&max);
    }

    if (g_file_get_contents (min_file, &min, NULL, NULL)) {
        min = g_strchomp (min);

        if (self->priv->min_freq != NULL)
            g_free (self->priv->min_freq);

        self->priv->min_freq = g_steal_pointer (&min);
    }

    g_message("max: %s -> %s", max_file, self->priv->max_freq);
    g_message("min: %s -> %s", min_file, self->priv->min_freq);

}

/**
 * freq_device_get_name:
 *
 * Get #FreqDevice device name
 *
 * @self: #FreqDevice
 *
 * Returns: device name
 *
 **/
const char*
freq_device_get_name (FreqDevice  *self)
{
    return self->priv->device_name;
}

/**
 * freq_device_set_powersave:
 *
 * Set freq device to powersave
 *
 * @param #FreqDevice
 * @param powersave: True to enable powersave
 */
void
freq_device_set_powersave (FreqDevice *self,
                           gboolean    powersave)
{
    if (powersave) {
        set_freq (
            self,
            self->priv->cur_node,
            self->priv->min_freq
        );
    } else {
        set_freq (
            self,
            self->priv->cur_node,
            self->priv->max_freq
        );
    }
}
