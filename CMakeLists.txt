cmake_minimum_required(VERSION 2.8.12)
project(SIDC_SWHV C)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -pedantic -Wall -W -pipe -std=gnu11 -O3")

set(SIDC_PROJECT_NAME   ${CMAKE_PROJECT_NAME})
set(SIDC_INSTALL_LIB    ${CMAKE_INSTALL_PREFIX}/lib)
set(SIDC_INSTALL_DATA   ${CMAKE_INSTALL_PREFIX}/data)
set(SIDC_INSTALL_PERL   ${CMAKE_INSTALL_PREFIX}/perl)
set(SIDC_INSTALL_PYTHON ${CMAKE_INSTALL_PREFIX}/python)
set(SIDC_PYTHON_LIB     ${CMAKE_INSTALL_PREFIX}/pythonlib)

set(SIDC_SUPPORT        ${CMAKE_INSTALL_PREFIX}/support)
set(SIDC_SUPPORT_INC    ${SIDC_SUPPORT}/include)
set(SIDC_SUPPORT_LIB    ${SIDC_SUPPORT}/lib)

set(CMAKE_INSTALL_RPATH ${SIDC_INSTALL_LIB} ${SIDC_SUPPORT_LIB})

macro(sidc_install_lib target)
    set_target_properties(${target} PROPERTIES VERSION 1.0 SOVERSION 1.0 INSTALL_NAME_DIR ${SIDC_INSTALL_LIB})
    install(TARGETS ${target} EXPORT ${SIDC_PROJECT_NAME} DESTINATION ${SIDC_INSTALL_LIB})
    install(EXPORT ${SIDC_PROJECT_NAME} DESTINATION ${SIDC_INSTALL_LIB})
endmacro()

macro(include_before ...)
    include_directories(BEFORE ${ARGV})
endmacro()

include_before(${SIDC_SUPPORT_INC})
link_directories(${SIDC_SUPPORT_LIB})

include(FindPkgConfig)
pkg_check_modules(PKG REQUIRED glib-2.0>=2.22 freetype2 libpng)
include_directories(${PKG_INCLUDE_DIRS})
link_directories(${PKG_LIBRARY_DIRS})

set(SUPPORT  ${CMAKE_CURRENT_SOURCE_DIR}/support)

set(CFITSIO  ${SUPPORT}/cfitsio/cfitsio-4.3.0)
set(GENX     ${SUPPORT}/genx)
set(JPEG     ${SUPPORT}/jpeg/jpeg-8d)
set(OPENJPEG ${SUPPORT}/openjpeg/libopenjpeg-2012-03-08)

add_subdirectory(support)

set(LIBSIDC libsidc)
include_directories(${LIBSIDC}/send2LMAT ${LIBSIDC}/libp2sc ${LIBSIDC}/libswap)
add_subdirectory(libsidc)

add_subdirectory(fits2img)
