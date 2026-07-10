# Installation

This section describes how to download and install the Arm Toolchain for Linux.

### Note

The system package manager installation method requires system administrator
(`root`) assistance. To install without `root` privileges, see
[Non-root installation method](#non-root-installation-method).

## Recommended installation method using system package manager

You can install and upgrade Arm Toolchain for Linux using Linux system package
managers and the Arm Toolchains package repositories. Installing the
`arm-toolchain-for-linux` package also installs the Arm Performance Libraries,
which are a required dependency.

### Arm Toolchains Linux package repository signing key change July 2026

Arm regularly reviews and strengthens its repository management processes.
As a result, Arm has revoked and replaced the Arm Toolchains repository signing
key used up to July 2026.

Current installations of Arm Toolchain for Linux continue to work, although
package manager update or upgrade operations may fail. Existing package users
must install the latest `arm-toolchains-repository` package for their
distribution, as shown below.

For more information, see the
[Arm Toolchains repository installation guide](https://learn.arm.com/install-guides/arm-toolchains-repository).

### Amazon Linux 2023

To configure the Arm Toolchains repository on Amazon Linux 2023:

```
$ sudo dnf install https://developer.arm.com/packages/arm-toolchains/amazonlinux/al2023/aarch64/arm-toolchains-repository-2-2.al2023.noarch.rpm
```

Install the latest Arm Toolchain for Linux release:

```
$ sudo dnf install arm-toolchain-for-linux
```

Upgrade an existing package-manager installation:

```
$ sudo dnf upgrade arm-toolchain-for-linux
```

### Red Hat Enterprise Linux 8, 9, and 10

To configure the Arm Toolchains repository on Red Hat Enterprise Linux 8:

```
$ sudo dnf install https://developer.arm.com/packages/arm-toolchains/rhel/el8/aarch64/arm-toolchains-repository-2-2.el8.noarch.rpm
```

To configure the Arm Toolchains repository on Red Hat Enterprise Linux 9:

```
$ sudo dnf install https://developer.arm.com/packages/arm-toolchains/rhel/el9/aarch64/arm-toolchains-repository-2-2.el9.noarch.rpm
```

To configure the Arm Toolchains repository on Red Hat Enterprise Linux 10:

```
$ sudo dnf install https://developer.arm.com/packages/arm-toolchains/rhel/el10/aarch64/arm-toolchains-repository-2-2.el10.noarch.rpm
```

Install the latest Arm Toolchain for Linux release:

```
$ sudo dnf install arm-toolchain-for-linux
```

Upgrade an existing package-manager installation:

```
$ sudo dnf upgrade arm-toolchain-for-linux
```

### SUSE Linux Enterprise Server 15 and 16

To configure the Arm Toolchains repository on SUSE Linux Enterprise Server 15:

```
$ sudo rpm -U https://developer.arm.com/packages/arm-toolchains/sles/sles15/aarch64/arm-toolchains-repository-2-2.sles15.noarch.rpm
```

To configure the Arm Toolchains repository on SUSE Linux Enterprise Server 16:

```
$ sudo rpm -U https://developer.arm.com/packages/arm-toolchains/sles/sles16/aarch64/arm-toolchains-repository-2-2.sles16.noarch.rpm
```

Install the latest Arm Toolchain for Linux release:

```
$ sudo zypper install arm-toolchain-for-linux
```

Upgrade an existing package-manager installation:

```
$ sudo zypper up arm-toolchain-for-linux
```


### Ubuntu Linux 22.04 and 24.04

To configure the Arm Toolchains repository on Ubuntu 22.04:

```
$ curl -O https://developer.arm.com/packages/arm-toolchains/ubuntu/pool/arm-toolchains-repository_2-2~jammy_all.deb
$ sudo dpkg -i arm-toolchains-repository_2-2~jammy_all.deb
$ sudo apt update
```

To configure the Arm Toolchains repository on Ubuntu 24.04:

```
$ curl -O https://developer.arm.com/packages/arm-toolchains/ubuntu/pool/arm-toolchains-repository_2-2~noble_all.deb
$ sudo dpkg -i arm-toolchains-repository_2-2~noble_all.deb
$ sudo apt update
```

Install the latest Arm Toolchain for Linux release:

```
$ sudo apt install arm-toolchain-for-linux
```

Upgrade an existing package-manager installation:

```
$ sudo apt upgrade arm-toolchain-for-linux
```

## Non-root installation method

You can install Arm Toolchain for Linux and Arm Performance Libraries in a
directory you choose without administrator (`root`) assistance by using the
auxiliary installation script.

First, download the installation script:

```
$ curl -L -o user_install.sh https://developer.arm.com/-/cdn-downloads/permalink/Arm-Toolchain-for-Linux/Package/user_install.sh
```

Run the script and specify the installation directory:

```
$ bash user_install.sh --yes <installation_directory>
```

The `<installation_directory>` path can be absolute or relative. For example,
to install into the current directory:

```
$ bash user_install.sh --yes .
```

For convenience, you can also download and run the script in a single command:

```
$ bash <(curl -L https://developer.arm.com/-/cdn-downloads/permalink/Arm-Toolchain-for-Linux/Package/user_install.sh) --yes <installation_directory>
```

For example, to install into the current directory:

```
$ bash <(curl -L https://developer.arm.com/-/cdn-downloads/permalink/Arm-Toolchain-for-Linux/Package/user_install.sh) --yes .
```

After completion, the `opt` directory tree is unpacked into the installation directory.
