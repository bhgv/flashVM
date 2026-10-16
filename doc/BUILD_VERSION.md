# Development version and automatic CMake refresh

`VERSION.txt` identifies the development snapshot. From v0.408 the root project
reads it through `cmake/ProjectVersion.cmake`, which also registers it as a
configuration dependency. Editing only that file therefore makes an ordinary
`cmake --build` refresh `PROJECT_VERSION` and the generated build metadata.
A separate manual configure is no longer required for a version-only edit.
This does not change runtime ABI, serialization versions or compatibility status.

The dependency uses the
[CMake 3.10 directory property](https://github.com/Kitware/CMake/blob/v3.10.0/Help/prop_dir/CMAKE_CONFIGURE_DEPENDS.rst),
consistent with this project's minimum CMake version.

`flashvm_build_version_refresh` uses the production module in an isolated,
compiler-free project: configure/build 1.2.3, edit only its VERSION.txt to 1.2.4,
run an ordinary build, verify the refreshed version, then build again to verify
stability. The real checkout's VERSION.txt is untouched. The fixture inherits
the root generator, make program and any generator platform/toolset arguments.
Python 3, enabled tests and a native build are required for CTest registration.

```sh
ctest --test-dir build -C Release -R "^flashvm_build_version_refresh$" --output-on-failure
```

This checks version dependency regeneration; it does not certify a foreign
platform or replace runtime/release acceptance. All roadmap stages remain open.
