## **How to Add Overrides for Mobile Power Saver (MPS)**

Mobile Power Saver (MPS) uses GLib schemas and overrides to manage power-saving behaviors on various devices and operating environments. Overrides allow you to customize MPS behavior for specific devices, use cases, or environments such as GNOME, Phosh, or device-specific setups like Miatoll.

### **1. Understanding the Schema**

The MPS schema (`org.adishatz.Mps`) defines various settings to control power-saving features, including:
- Bluetooth power saving.
- Suspension of services and apps.

Each key in the schema can be overridden using a GLib override file (`*.override`).

---

### **2. Structure of an Override File**

An override file is a simple text file specifying new values for the schema keys. The file uses the format:

```
[<schema-id>]
<key-name>=<value>
```

For example, an override to disable specific services from being suspended:
```ini
[org.adishatz.Mps]
suspend-user-services=['pulseaudio.service']
```

---

### **3. Creating an Override File**

To create an override:
1. **Locate the correct directory**:
   Override files should be placed in `/usr/share/glib-2.0/schemas/` for system-wide changes or `~/.local/share/glib-2.0/schemas/` for user-specific overrides.

2. **Name the file**:
   Use a descriptive name ending in `.override`, such as `org.adishatz.Mps.gnome.override` or `org.adishatz.Mps.miatoll.override`.

3. **Write the content**:
   Use the schema keys and their values to specify the desired configuration.

---

### **4. Example Overrides**

#### **GNOME Environment Example**
This configuration prevents suspension of certain apps and excludes an app from Bluetooth power saving:
```ini
[org.adishatz.Mps]
bluetooth-power-saving-blacklist=['io.gitlab.azymohliad.WatchMate']
suspend-apps-blacklist=['org.gnome.Calls', 'sm.puri.Chatty', 'org.gnome.clocks', 'org.kop316.antispam']
```

#### **Phosh Environment Example**
This configuration enables service suspension but exempts critical user and system services:
```ini
[org.adishatz.Mps]
suspend-user-services['pulseaudio.service']
suspend-system-services=['sshd.service']
suspend-system-bluetooth-services=['bluebinder.service', 'bluetooth.service']
suspend-user-bluetooth-services=['mpris-proxy.service']
cpuset-topapp=['sm.puri.Phosh.service']
```

---

### **5. Deploying the Override**

After creating your override file, deploy it by:
1. **Placing the file in the appropriate directory**:
   - System-wide: `/usr/share/glib-2.0/schemas/`
   - User-specific: `~/.local/share/glib-2.0/schemas/`

2. **Recompiling the schema**:
   Run the following command to apply the changes:
   ```bash
   glib-compile-schemas /usr/share/glib-2.0/schemas/
   ```
   (Replace with `~/.local/share/glib-2.0/schemas/` for user-specific overrides.)

---

### **6. Best Practices for Writing Overrides**

1. **Base Overrides on the Default Schema**:
   Review the default settings in the `org.adishatz.Mps` schema to ensure compatibility.
   
2. **Follow Environment-Specific Needs**:
   Tailor overrides for GNOME, Phosh, or device-specific requirements. For instance:
   - GNOME: Focus on app-based power-saving behavior.
   - Phosh/Droidian: Manage user and system services carefully.
   - Devices: Handle hardware-specific services (Android vendor services on Droidian for example)

3. **Validate Syntax**:
   Ensure your override file uses valid syntax. Use tools like `glib-compile-schemas` to catch errors.

4. **Test Changes**:
   After applying the overrides, test the device to confirm that the settings work as expected.

---

### **7. Troubleshooting**

- **Override Not Applying**:
  - Ensure the override file is placed in the correct directory.
  - Run `glib-compile-schemas` to recompile the schemas.

- **Services Not Behaving as Expected**:
  - Double-check service names in the blacklist.
  - Confirm the services are managed by MPS.

- **Performance Issues**:
  - Review and fine-tune the CPU set and service suspension settings.

---

### **8. Further Customizations**

To customize MPS even further:
- Create device-specific service blacklists (`suspend-user-services-blacklist`, `suspend-system-services-blacklist`).
- Manage processes using `suspend-processes` or `cpuset-background-processes`.
- Add apps or services to the Bluetooth power-saving or suspension lists.
