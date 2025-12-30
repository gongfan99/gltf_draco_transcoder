#!/bin/bash
set -e

echo "Installing dependencies..."
yum install -y git curl zip unzip tar pkgconfig

echo "Cloning vcpkg..."
git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg

echo "Bootstrapping vcpkg..."
cd /opt/vcpkg
./bootstrap-vcpkg.sh

echo "vcpkg installation complete."
