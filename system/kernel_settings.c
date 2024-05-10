/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "kernel_settings.h"
#include "../common/utils.h"


struct _KernelSettingsPrivate {
    unsigned short int placeholder;
};


G_DEFINE_TYPE_WITH_CODE (
    KernelSettings,
    kernel_settings,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (KernelSettings)
)


static void
kernel_settings_dispose (GObject *kernel_settings)
{
    KernelSettings *self = KERNEL_SETTINGS (kernel_settings);

    g_free (self->priv);

    G_OBJECT_CLASS (kernel_settings_parent_class)->dispose (kernel_settings);
}

static void
kernel_settings_class_init (KernelSettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = kernel_settings_dispose;
}

static void
kernel_settings_init (KernelSettings *self)
{
    self->priv = kernel_settings_get_instance_private (self);

    /* Disable Adreno bus control */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/bus_split", "0"
    );

    /* Disable Adreno NAP */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/force_no_nap", "1"
    );

    /* Do not keep bus on when screen is off */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/force_bus_on", "0"
    );

    /* Do not keep clock on when screen is off */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/force_clk_on", "0"
    );

    /* Do not keep regulators on when screen is on */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/force_rail_on", "0"
    );

    /* On fork, do not give more priority to child than parent */
    write_to_file (
        "/proc/sys/kernel/sched_child_runs_first", "0"
    );

    /* Disable IRQ debugging */
    write_to_file (
        "/sys/module/spurious/parameters/noirqdebug", "Y"
    );

    /* Hints to the kernel how much CPU time it should be allowed
     * to use to handle perf sampling events
     */
    write_to_file (
        "/proc/sys/kernel/perf_cpu_time_max_percent", "20"
    );

    /* For non conservative boost: default kernel value */
    write_to_file (
        "/proc/sys/kernel/sched_min_task_util_for_colocation", "35"
    );
    /* For conservative boost: default kernel value */
    write_to_file (
        "/proc/sys/kernel/sched_min_task_util_for_boost", "51"
    );

    /* CAF's hispeed boost and predicted load features aren't any good. */
    write_to_file (
        "/proc/sys/kernel/sched_conservative_pl", "0"
    );

    /* Disable kernel debug */
    write_to_file (
        "/sys/kernel/debug/debug_enabled", "N"
    );

    /* Disable vidc fw debug */
    write_to_file (
        "/sys/kernel/debug/msm_vidc/fw_debug_mode", "0"
    );

    /* self-tests disabled */
    write_to_file (
        "/sys/module/cryptomgr/parameters/notests", "Y"
    );

    /* Do not automatically load TTY Line Disciplines */
    write_to_file (
        "/proc/sys/dev/tty/ldisc_autoload", "0"
    );

    /* Disable expedited RCU */
    write_to_file (
        "/sys/kernel/rcu_normal", "1"
    );
    write_to_file (
        "/sys/kernel/rcu_expedited", "0"
    );

    /* Disable unnecessary printk logging */
    write_to_file (
        "/proc/sys/kernel/printk_devkmsg", "off"
    );

    /* Update /proc/stat less often to reduce jitter */
    write_to_file (
        "/proc/sys/vm/stat_interval", "120"
    );
}

/**
 * kernel_settings_new:
 *
 * Creates a new #KernelSettings
 *
 * Returns: (transfer full): a new #KernelSettings
 *
 **/
GObject *
kernel_settings_new (void)
{
    GObject *kernel_settings;

    kernel_settings = g_object_new (TYPE_KERNEL_SETTINGS, NULL);

    return kernel_settings;
}

/**
 * kernel_settings_set_powersave:
 *
 * Set kernel_settings devices to powersave
 *
 * @param #KernelSettings
 * @param powersave: True to enable powersave
 */
void
kernel_settings_set_powersave (KernelSettings  *kernel_settings, gboolean  powersave) {
    if (powersave) {
        /* https://www.fatalerrors.org/a/schedtune-learning-notes.html */
        write_to_file (
            "/sys/fs/cgroup/schedtune/schedtune.boost", "0"
        );
        write_to_file (
            "/sys/fs/cgroup/schedtune/schedtune.prefer_idle", "0"
        );
        write_to_file (
            "/proc/sys/kernel/sched_boost", "0"
        );

        /* Do not move big tasks from little cluster to big cluster */
        write_to_file (
            "/proc/sys/kernel/sched_walt_rotate_big_tasks", "0"
        );

        /* Reduce memory management power usage */
        write_to_file (
            "/proc/sys/vm/swappiness", "5"
        );
        write_to_file (
            "/proc/sys/vm/dirty_background_ratio", "50"
        );
        write_to_file (
            "/proc/sys/vm/dirty_ratio", "90"
        );
        write_to_file (
            "/proc/sys/vm/dirty_writeback_centisecs", "60000"
        );
        write_to_file (
            "/proc/sys/vm/dirty_expire_centisecs", "60000"
        );

        /* Enable laptop mode */
        write_to_file (
            "/proc/sys/vm/laptop_mode", "5"
        );

        /* Disable LPM predictions */
        write_to_file (
            "/sys/module/lpm_levels/parameters/lpm_prediction", "N"
        );
    } else {
        /* https://lwn.net/Articles/706374/ */
        write_to_file (
            "/sys/fs/cgroup/schedtune/schedtune.boost", "10"
        );
        write_to_file (
            "/sys/fs/cgroup/schedtune/schedtune.prefer_idle", "1"
        );
        write_to_file (
            "/proc/sys/kernel/sched_boost", "1"
        );

        /* Move big tasks from little cluster to big cluster */
        write_to_file (
            "/proc/sys/kernel/sched_walt_rotate_big_tasks", "1"
        );

        /* Default kernel value */
        write_to_file (
            "/proc/sys/vm/swappiness", "60"
        );
        write_to_file (
            "/proc/sys/vm/dirty_background_ratio", "10"
        );
        write_to_file (
            "/proc/sys/vm/dirty_ratio", "20"
        );
        write_to_file (
            "/proc/sys/vm/dirty_writeback_centisecs", "500"
        );
        write_to_file (
            "/proc/sys/vm/dirty_expire_centisecs", "3000"
        );

        /* Disable laptop mode */
        write_to_file (
            "/proc/sys/vm/laptop_mode", "0"
        );

        /* Enable LPM predictions */
        write_to_file (
            "/sys/module/lpm_levels/parameters/lpm_prediction", "Y"
        );
    }
}