#!/bin/sh
set -e

echo "Installing dependencies for vcpkg..."
if command -v apk >/dev/null 2>&1; then
    # Alpine (musllinux)
    apk add --no-cache \
        git curl zip unzip tar pkgconf \
        build-base cmake ninja \
        linux-headers bash
elif command -v yum >/dev/null 2>&1; then
    # Manylinux (RHEL/Alma)
    yum install -y git curl zip unzip tar pkgconfig
fi

echo "Cloning vcpkg..."
git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg

echo "Bootstrapping vcpkg..."
cd /opt/vcpkg
./bootstrap-vcpkg.sh

echo "vcpkg installation complete."
