# Installation

This section describes how to download and install the Arm Toolchain for Linux.

### Note

The recommended installation method requires system administrator (`root`)
assistance. Non-root installation is also possible, you can read about it
further.

## Recommended installation method

The first step is to configure your Linux package manager to be able to fetch
packages from Arm. You do this step only once.

From the available options, select the packages repository that matches your
installed Linux distribution. This adds the Arm toolchain repository to the
package manager:

* Ubuntu 22.04

```
$ curl "https://developer.arm.com/packages/arm-toolchains:ubuntu-22/jammy/Release.key" | sudo gpg --dearmor -o /usr/share/keyrings/obs-oss-arm-com.gpg

$ echo "deb [signed-by=/usr/share/keyrings/obs-oss-arm-com.gpg] https://developer.arm.com/packages/arm-toolchains:ubuntu-22/jammy/ ./" | sudo tee /etc/apt/sources.list.d/obs-oss-arm-com.list

$ sudo apt update
```

* Ubuntu 24.04

```
$ curl "https://developer.arm.com/packages/arm-toolchains:ubuntu-24/noble/Release.key" | sudo gpg --dearmor -o /usr/share/keyrings/obs-oss-arm-com.gpg

$ echo "deb [signed-by=/usr/share/keyrings/obs-oss-arm-com.gpg] https://developer.arm.com/packages/arm-toolchains:ubuntu-24/noble/ ./" | sudo tee /etc/apt/sources.list.d/obs-oss-arm-com.list

$ sudo apt update
```

* Red Hat Enterprise Linux 8

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:rhel-8/el8/arm-toolchains:rhel-8.repo
```

* Red Hat Enterprise Linux 9

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:rhel-9/el9/arm-toolchains:rhel-9.repo
```

* Red Hat Enterprise Linux 10

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:rhel-10/el10/arm-toolchains:rhel-10.repo
```

* Amazon Linux 2023

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:amzn-2023/al2023/arm-toolchains:amzn-2023.repo
```

* SUSE Linux Enterprise Server 15

```
$ sudo zypper ar -f https://developer.arm.com/packages/arm-toolchains:sles-15/sl15/arm-toolchains:sles-15.repo
```

Select the installation command appropriate for your Linux distribution. This
installs the Arm Toolchain for Linux, along with the Arm Performance Libraries,
which is a required dependency:

* Ubuntu systems

```
$ sudo apt install arm-toolchain-for-linux
```

* Red Hat Enterprise Linux and Amazon Linux systems

```
$ sudo dnf install arm-toolchain-for-linux
```

* SUSE Linux Enterprise Server systems

```
$ sudo zypper install arm-toolchain-for-linux
```

## Non-root installation method

You can install the Arm Toolchain for Linux, along with the Arm Performance
Libraries in any directory that you wish to specify. For this, the auxiliary
installation script can be used. This script does not require system
administrator (`root`) assistance:

```
$ bash <(curl -L https://developer.arm.com/-/cdn-downloads/permalink/Arm-Toolchain-for-Linux/Package/user_install.sh) --yes <installation_directory>
```

The `<installation_directory>` path can be either absolute or relative.
For example, you can invoke a local directory installation:

```
$ bash <(curl -L https://developer.arm.com/-/cdn-downloads/permalink/Arm-Toolchain-for-Linux/Package/user_install.sh) --yes .
```

After completion, you can see the `opt` directory tree unpacked into your local
directory.
