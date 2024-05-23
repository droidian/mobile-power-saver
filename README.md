# Mobile Power Saver

Mobile Power Saver enables power saving on mobile devices:
- Provides power-profile-daemon basic API compatibility
- Puts devices in low power when screen is off
- Suspends services/processes when screen is off
- Freezes applications (only when screen is off for now)

## Settings ##

![Power Settings](https://adishatz.org/data/mps_battery.png)
![App Settings](https://adishatz.org/data/mps_app_suspend.png)

These are only examples that need to be tweaked depending on device/OS!

- Suspend processes:

`$ gsettings set org.adishatz.Mps screen-off-suspend-processes "['vendor.nxp.hardware.nfc@1.2-service']"`

- Suspend system services:

`$ gsettings set org.adishatz.Mps screen-off-suspend-system-services "['cups.service', 'nfcd.service']"`

- Suspend user services:

`$ gsettings set org.adishatz.Mps screen-off-suspend-user-services "['gvfs-afc-volume-monitor.service', 'org.gnome.SettingsDaemon.Rfkill.service', 'org.gnome.SettingsDaemon.Sharing.service', 'org.gnome.SettingsDaemon.Color.service', 'gvfs-mtp-volume-monitor.service', 'gvfs-udisks2-volume-monitor.service', 'org.gnome.SettingsDaemon.Housekeeping.service', 'org.gnome.SettingsDaemon.Wacom.service', 'org.gnome.SettingsDaemon.Smartcard.service', 'org.gnome.SettingsDaemon.PrintNotifications.service', 'org.gnome.SettingsDaemon.MediaKeys.service', 'org.gnome.SettingsDaemon.A11ySettings.service', 'gnome-keyring-daemon.service', 'gcr-ssh-agent.service', 'org.droidian.Flashlightd.service']"`

## Depends on

- `glib2`
- `meson`
- `ninja`

## Building from Git

```bash
$ meson builddir --prefix=/usr

$ sudo ninja -C builddir install
```
