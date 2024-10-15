/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <gio/gio.h>

#include "freq_device.h"
#include "../common/utils.h"

struct _FreqDevicePrivate {
    char *sysfs_dir;
    char *device_name;
    char *governor_node;

    char *default_governor;
    char *current_governor;
};

G_DEFINE_TYPE_WITH_CODE (
    FreqDevice,
    freq_device,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (FreqDevice)
)

static void
set_governor (FreqDevice *freq_device,
              const char *governor)
{
    g_autofree char *filename = g_build_filename (
        freq_device->priv->sysfs_dir,
        freq_device->priv->device_name,
        freq_device->priv->governor_node,
        NULL
    );

    g_message ("%s -> %s", filename, governor);

    write_to_file (filename, governor);
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

    g_free (self->priv->default_governor);
    g_free (self->priv->current_governor);
    g_free (self->priv->device_name);
    g_free (self->priv->governor_node);
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
    self->priv->governor_node = NULL;
    self->priv->default_governor = NULL;
    self->priv->current_governor = NULL;
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
 * @governor_node: sysfs governor node
 *
 * Returns: (transfer full): a new #FreqDevice
 *
 **/
void
freq_device_set_sysfs_settings (FreqDevice *self,
                                const char *directory,
                                const char *governor_node)
{
    if (self->priv->sysfs_dir != NULL)
        g_free (self->priv->sysfs_dir);

    self->priv->sysfs_dir = g_strdup (directory);
    self->priv->governor_node = g_strdup (governor_node);
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
    g_autofree char *contents = NULL;

    g_autofree char *filename = g_build_filename (
        self->priv->sysfs_dir, device_name, self->priv->governor_node, NULL
    );

    self->priv->device_name = g_strdup (device_name);

    if (g_file_get_contents (filename, &contents, NULL, NULL)) {
        contents = g_strchomp (contents);

        if (self->priv->default_governor != NULL)
            g_free (self->priv->default_governor);

        self->priv->default_governor = g_steal_pointer (&contents);
        g_message("default governor: %s -> %s",
                  filename,
                  self->priv->default_governor);
    }
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
    if (powersave)
        set_governor (self, "powersave");
    else if (self->priv->current_governor != NULL)
        set_governor (self, self->priv->current_governor);
    else
        set_governor (self, self->priv->default_governor);
}

/**
 * freq_device_set_governor:
 *
 * Set freq device governor
 *
 * @param #FreqDevice
 * @param governor: new governor to set
 */
void
freq_device_set_governor (FreqDevice *self,
                          const char *governor)
{
    if (self->priv->current_governor != NULL)
        g_free (self->priv->current_governor);

    if (governor == NULL)
        self->priv->current_governor = g_strdup (self->priv->default_governor);
    else
        self->priv->current_governor = g_strdup (governor);
    set_governor (self, self->priv->current_governor);
}