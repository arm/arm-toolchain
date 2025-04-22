## Installation

This document describes how to download and install the Arm Toolchain for Linux.

### Pre-installation step

The first step is to configure your Linux package manager to be able to fetch
packages from Arm. This step only needs to be performed once.

From the options below, select the packages repository matching with your
installed Linux distribution.

#### Ubuntu 22.04

```
$ curl "https://developer.arm.com/packages/arm-toolchains:ubuntu-22/jammy/Release.key" | sudo gpg --dearmor -o /usr/share/keyrings/obs-oss-arm-com.gpg

$ echo "deb [signed-by=/usr/share/keyrings/obs-oss-arm-com.gpg] https://developer.arm.com/packages/arm-toolchains:ubuntu-22/jammy/ ./" | sudo tee /etc/apt/sources.list.d/obs-oss-arm-com.list

$ sudo apt update
```

#### Ubuntu 24.04

```
$ curl "https://developer.arm.com/packages/arm-toolchains:ubuntu-24/noble/Release.key" | sudo gpg --dearmor -o /usr/share/keyrings/obs-oss-arm-com.gpg

$ echo "deb [signed-by=/usr/share/keyrings/obs-oss-arm-com.gpg] https://developer.arm.com/packages/arm-toolchains:ubuntu-24/noble/ ./" | sudo tee /etc/apt/sources.list.d/obs-oss-arm-com.list

$ sudo apt update
```

#### Red Hat Entrprise Linux 8

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:rhel-8/el8/arm-toolchains:rhel-8.repo
```

#### Red Hat Entrprise Linux 9

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:rhel-9/el9/arm-toolchains:rhel-9.repo
```

#### Amazon Linux 2023

```
$ sudo dnf install 'dnf-command(config-manager)'

$ sudo dnf config-manager -y --add-repo https://developer.arm.com/packages/arm-toolchains:amzn-2023/al2023/arm-toolchains:amzn-2023.repo
```

#### SUSE Linux Enterprise Server 15

```
$ sudo zypper ar -f https://developer.arm.com/packages/arm-toolchains:sles-15/sl15/arm-toolchains:sles-15.repo
```

### Installation step

Please select the command below appropriate for your Linux distribution. This
will install Arm Toolchain For Linux, along with Arm Performance Libraries,
which is a required dependency.

#### Ubuntu systems

```
$ sudo apt install arm-toolchain-for-linux
```

#### Red Hat Enterprise Linux and Amazon Linux systems

```
$ sudo dnf install arm-toolchain-for-linux
```

#### SUSE Linux Enterprise Server systems

```
$ sudo zypper install arm-toolchain-for-linux
```
