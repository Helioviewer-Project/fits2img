
add_library(sidc_log SHARED	sidc_log.c)

target_link_libraries(sidc_log ${PKG_LIBRARIES})
sidc_install_lib(sidc_log)

add_executable(test_log test_log.c)
target_link_libraries(test_log sidc_log)
