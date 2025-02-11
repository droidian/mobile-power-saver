/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "kernel_settings.h"
#include "../common/utils.h"

/* struct _KernelSettingsPrivate { */
/* }; */

G_DEFINE_TYPE_WITH_CODE (
    KernelSettings,
    kernel_settings,
    G_TYPE_OBJECT,
    /* G_ADD_PRIVATE (KernelSettings) */
)

static void
kernel_settings_dispose (GObject *kernel_settings)
{
    G_OBJECT_CLASS (kernel_settings_parent_class)->dispose (kernel_settings);
}

static void
kernel_settings_finalize (GObject *kernel_settings)
{
    G_OBJECT_CLASS (kernel_settings_parent_class)->finalize (kernel_settings);
}

static void
kernel_settings_class_init (KernelSettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = kernel_settings_dispose;
    object_class->finalize = kernel_settings_finalize;
}

static void
kernel_settings_init (KernelSettings *self)
{
    self->priv = kernel_settings_get_instance_private (self);

    /* Splits the memory bus usage between different components to optimize power consumption */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/bus_split", "1"
    );

    /* Do not isable Adreno NAP */
    write_to_file (
        "/sys/class/kgsl/kgsl-3d0/force_no_nap", "0"
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

    /* Enable LPM predictions */
    write_to_file (
        "/sys/module/lpm_levels/parameters/lpm_prediction", "Y"
    );

    /* Enable LPM IPI predictions */
    write_to_file (
        "/sys/module/lpm_levels/parameters/lpm_ipi_prediction", "Y"
    );

    /* Disable IPv6 Router Advertisements */
    write_to_file (
        "/proc/sys/net/ipv6/conf/all/accept_ra", "0"
    );

    /* Disable IPv6 Duplicate Address Detection */
    write_to_file (
        "/proc/sys/net/ipv6/conf/all/accept_dad", "0"
    );

    /* Reduce IPv6 Neighbor Discovery Frequency */
    write_to_file (
        "/proc/sys/net/ipv6/neigh/default/gc_stale_time", "120"
    );

    /* Disable IPv4 Source Address Selection */
    write_to_file (
        "/proc/sys/net/ipv4/ip_nonlocal_bind", "0"
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
kernel_settings_set_powersave (KernelSettings *kernel_settings,
                               gboolean        powersave)
{
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

        /* Hysteresis for Low Power Mode */
        write_to_file (
            "/sys/module/lpm_levels/parameters/bias_hyst", "15"
        );

        /* qcom service locator */
        write_to_file (
            "/sys/module/service_locator/parameters/enable", "0"
        );

        /* Jiffies Until First Frequency Quanta Sampling */
        write_to_file (
            "/sys/module/rcutree/parameters/jiffies_till_first_fqs", "1000"
        );

        /* Jiffies Till Next Frequency Quanta Sampling */
        write_to_file (
            "/sys/module/rcutree/parameters/jiffies_till_next_fqs", "1000"
        );

        /* Priority of RCPU Kernel Threads */
        write_to_file (
            "/sys/module/rcutree/parameters/kthread_prio", "1000"
        );

        /* Whether RCPU stall detection is suppressed for CPUs */
        write_to_file (
            "/sys/module/rcupdate/parameters/rcu_cpu_stall_suppress", "1"
        );

        /* Determines how frequently the kernel polls block devices  */
        write_to_file (
            "/sys/module/block/parameters/events_dfl_poll_msecs", "60000"
        );

        /* Prevents frequent kernel network wakeups */
        write_to_file (
            "/proc/sys/net/ipv4/tcp_fastopen", "0"
        );

        /* Reduce TCP Retransmission Timeouts */
        write_to_file (
            "/proc/sys/net/ipv4/tcp_retries2", "60"
        );

        /* Reduce TCP Time-Wait */
        write_to_file (
            "/proc/sys/net/ipv4/tcp_fin_timeout", "5"
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

        /* Hysteresis for Low Power Mode */
        write_to_file (
            "/sys/module/lpm_levels/parameters/bias_hyst", "0"
        );

        /* qcom service locator */
        write_to_file (
            "/sys/module/service_locator/parameters/enable", "1"
        );

        /* Jiffies Until First Frequency Quanta Sampling */
        write_to_file (
            "/sys/module/rcutree/parameters/jiffies_till_first_fqs", "1"
        );

        /* Jiffies Till Next Frequency Quanta Sampling */
        write_to_file (
            "/sys/module/rcutree/parameters/jiffies_till_next_fqs", "1"
        );

        /* Priority of RCPU Kernel Threads */
        write_to_file (
            "/sys/module/rcutree/parameters/kthread_prio", "1"
        );

        /* Whether RCPU stall detection is suppressed for CPUs */
        write_to_file (
            "/sys/module/rcupdate/parameters/rcu_cpu_stall_suppress", "0"
        );

        /* Determines how frequently the kernel polls block devices  */
        write_to_file (
            "/sys/module/block/parameters/events_dfl_poll_msecs", "2000"
        );

        /* Restore frequent kernel network wakeups */
        write_to_file (
            "/proc/sys/net/ipv4/tcp_fastopen", "1027"
        );

        /* Restore TCP Retransmission Timeouts */
        write_to_file (
            "/proc/sys/net/ipv4/tcp_retries2", "15"
        );

        /* Restore TCP Time-Wait */
        write_to_file (
            "/proc/sys/net/ipv4/tcp_fin_timeout", "60"
        );
    }
}