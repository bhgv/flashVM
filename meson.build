# Native subset: Meson/Ninja run real direct CC/ar recipes, without CMake.
project('flashvm_native_build', version: '0.1531', meson_version: '>=0.54.0')
bash = find_program('bash')
env_program = find_program('env')
native_root = get_option('native_output_root')
if native_root == ''
  native_root = join_paths(meson.current_build_dir(), 'products')
endif
builder = join_paths(meson.current_source_dir(), 'tools', 'build-native.sh')
# run_target is intentional: the shared native backend always rebuilds products.
# The complete VM/FIR/IDE build remains available through CMake 3.10.
foreach component : ['mk', 'iyacc', 'cocor', 'as1', 'as2', 'as3', 'limbo', 'compilers', 'linker']
  run_target(component, command: [env_program, 'BUILD_ROOT=' + native_root, bash, builder, component])
endforeach
