@echo off

@REM set CIBW_ENVIRONMENT=CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-release -DVCPKG_INSTALL_OPTIONS=--allow-unsupported -DVCPKG_MANIFEST_MODE=OFF"
set CIBW_ENVIRONMENT=CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static-md-release -DVCPKG_INSTALL_OPTIONS=--allow-unsupported"

echo CIBW_ENVIRONMENT is set to: %CIBW_ENVIRONMENT%

python -m cibuildwheel --output-dir wheelhouse