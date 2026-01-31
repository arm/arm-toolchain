FROM rockylinux/rockylinux:8

RUN dnf install -y epel-release && \
    dnf upgrade -y && \
    crb enable && \
    dnf install -y \
        xz \
        git \
        cmake \
        ninja-build \
        bzip2 \
        gcc-c++ \
        python39 \
        qemu-kvm \
        meson \
        patch \
        glibc-headers && \
    dnf clean all && \
    rm -rf /var/cache/dnf
