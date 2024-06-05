/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#define CPUFREQ_POLICIES_DIR "/sys/devices/system/cpu/cpufreq/"
#define CPUFREQ_DIR ""
#define DEVFREQ_DIR "/sys/class/devfreq/"
#define CGROUPS_APPS_FREEZE_DIR "/sys/fs/cgroup/user.slice/user-%d.slice/user@%d.service/app.slice"
#define CGROUPS_USER_SERVICES_FREEZE_DIR "/sys/fs/cgroup/user.slice/user-%d.slice/user@%d.service/session.slice"
#define CGROUPS_SYSTEM_SERVICES_FREEZE_DIR "/sys/fs/cgroup/system.slice"

typedef enum {
  POWER_PROFILE_POWER_SAVER,
  POWER_PROFILE_BALANCED,
  POWER_PROFILE_PERFORMANCE,
  POWER_PROFILE_LAST
} PowerProfile;

