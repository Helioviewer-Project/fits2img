
add_library(send2LMAT STATIC send2LMAT.c)
set_target_properties(send2LMAT PROPERTIES COMPILE_DEFINITIONS SEND2LMAT_DIR="${CMAKE_INSTALL_PREFIX}/LMAT")
target_link_libraries(send2LMAT ${PKG_LIBRARIES})
sidc_install_lib(send2LMAT)
