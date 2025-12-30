#!/bin/bash
set -e

echo "Installing dependencies..."
yum install -y git curl zip unzip tar pkgconfig

echo "Cloning vcpkg..."
git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg

echo "Bootstrapping vcpkg..."
cd /opt/vcpkg
./bootstrap-vcpkg.sh

echo "Setting VCPKG_ROOT..."
export VCPKG_ROOT=/opt/vcpkg
echo "VCPKG_ROOT=$VCPKG_ROOT" >> $GITHUB_ENV

echo "vcpkg installation complete."
