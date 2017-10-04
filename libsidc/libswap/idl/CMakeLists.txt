
set(CMAKE_C_FLAGS "${BIND_C_FLAGS} -std=gnu99")

build_binding(swap_affine swap_affineidl lib/idl swap_affine.dlm "${IDL_ADD_CFLAGS}" swap)
