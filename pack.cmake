pack_native_module(signac)
pack_dir(shaders)
pack_dir(viewer)
create_launcher(viewer viewer/viewer)

pack_disable(CORE_EQUALIZER)
pack_disable(CORE_UI)
