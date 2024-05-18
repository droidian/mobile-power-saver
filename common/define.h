/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#define CPUFREQ_POLICIES_DIR "/sys/devices/system/cpu/cpufreq/"
#define DEVFREQ_DIR "/sys/class/devfreq/"
#define CGROUPS_APPS_FREEZE_DIR "/sys/fs/cgroup/user.slice/user-%d.slice/user@%d.service/app.slice"
#define CGROUPS_USER_SERVICES_FREEZE_DIR "/sys/fs/cgroup/user.slice/user-%d.slice/user@%d.service/session.slice"
#define CGROUPS_SYSTEM_SERVICES_FREEZE_DIR "/sys/fs/cgroup/system.slice"
