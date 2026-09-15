#!/bin/bash
set -e -u

dnf install -y \
    cairo-devel \
    fribidi-devel \
    gobject-introspection-devel \
    libffi-devel \
    libmount-devel

curl -L -O https://download.gnome.org/sources/glib/2.60/glib-2.60.7.tar.xz
tar -xf glib-2.60.7.tar.xz glib-2.60.7/

curl -L -O https://github.com/harfbuzz/harfbuzz/releases/download/2.6.8/harfbuzz-2.6.8.tar.xz
tar -xf harfbuzz-2.6.8.tar.xz harfbuzz-2.6.8/

curl -L -O https://download.gnome.org/sources/pango/1.44/pango-1.44.7.tar.xz
tar -xf pango-1.44.7.tar.xz pango-1.44.7/

export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig/
export LD_LIBRARY_PATH=/usr/local/lib64/

mkdir -p staging
STAGING="$PWD/staging"

# Pango needs to be built with an older version of meson (0.58.2) so set up a
# virtual environment to install it using pip
python3 -m venv "$STAGING/.venv"
source "$STAGING/.venv/bin/activate"
pip3 install setuptools meson==0.58.2

cd glib-2.60.7
meson setup builddir --prefix=/usr/local --wrap-mode=nofallback
ninja -C builddir
# Need to install packages twice:
# Once so it's available to the Pango build
ninja -C builddir install
# And once so it goes in the installation tarball
DESTDIR="$STAGING" ninja -C builddir install
cd ..

cd harfbuzz-2.6.8
meson setup builddir --prefix=/usr/local --wrap-mode=nofallback
ninja -C builddir
# Same installation pattern for harfbuzz:
ninja -C builddir install
DESTDIR="$STAGING" ninja -C builddir install
cd ..

cd pango-1.44.7
meson setup builddir --prefix=/usr/local --wrap-mode=nofallback
ninja -C builddir
# Pango only needs to be installed once, though
DESTDIR="$STAGING" ninja -C builddir install
cd ..

cd staging
tar -cJf "../Pango-Linux-$(uname -m).tar.xz" .
