cmake --fresh -S . -B build_sample ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md-release ^
  -DVCPKG_INSTALL_OPTIONS=--allow-unsupported ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
cmake --build build_sample --config Release
@REM -DVCPKG_MANIFEST_INSTALL=OFF ^