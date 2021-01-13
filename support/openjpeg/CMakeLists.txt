
add_definitions(-DNDEBUG)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -funroll-loops -ffast-math")

include_before(${OPENJPEG} ${CMAKE_CURRENT_SOURCE_DIR})
aux_source_directory(${OPENJPEG} OPENJPEG_SRCS)
add_library(openjpeg STATIC opj_extra.c ${OPENJPEG_SRCS})

target_link_libraries(openjpeg m)
sidc_install_lib(openjpeg)

set(OPENJP15 ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg-1.5.1)
set(OPENJP15_SRCS
  ${OPENJP15}/bio.c
  ${OPENJP15}/cio.c
  ${OPENJP15}/dwt.c
  ${OPENJP15}/event.c
  ${OPENJP15}/image.c
  ${OPENJP15}/j2k.c
  ${OPENJP15}/j2k_lib.c
  ${OPENJP15}/jp2.c
  ${OPENJP15}/jpt.c
  ${OPENJP15}/mct.c
  ${OPENJP15}/mqc.c
  ${OPENJP15}/openjpeg.c
  ${OPENJP15}/pi.c
  ${OPENJP15}/raw.c
  ${OPENJP15}/t1.c
  ${OPENJP15}/t2.c
  ${OPENJP15}/tcd.c
  ${OPENJP15}/tgt.c
  ${OPENJP15}/cidx_manager.c
  ${OPENJP15}/phix_manager.c
  ${OPENJP15}/ppix_manager.c
  ${OPENJP15}/thix_manager.c
  ${OPENJP15}/tpix_manager.c
)
add_library(openjp15 STATIC ${OPENJP15_SRCS})
#this removes all other include directories for target
set_target_properties(openjp15 PROPERTIES INCLUDE_DIRECTORIES ${OPENJP15})
sidc_install_lib(openjp15)

set(OPENJP2 ${CMAKE_CURRENT_SOURCE_DIR}/openjp2)
set(OPENJP2_SRCS
  ${OPENJP2}/bio.c
  ${OPENJP2}/cio.c
  ${OPENJP2}/dwt.c
  ${OPENJP2}/event.c
  ${OPENJP2}/image.c
  ${OPENJP2}/invert.c
  ${OPENJP2}/j2k.c
  ${OPENJP2}/jp2.c
  ${OPENJP2}/mct.c
  ${OPENJP2}/mqc.c
  ${OPENJP2}/openjpeg.c
  ${OPENJP2}/opj_clock.c
  ${OPENJP2}/pi.c
  ${OPENJP2}/raw.c
  ${OPENJP2}/t1.c
  ${OPENJP2}/t2.c
  ${OPENJP2}/tcd.c
  ${OPENJP2}/tgt.c
  ${OPENJP2}/function_list.c
)
add_library(openjp2 STATIC ${OPENJP2_SRCS})
#this removes all other include directories for target
set_target_properties(openjp2 PROPERTIES INCLUDE_DIRECTORIES ${OPENJP2})
sidc_install_lib(openjp2)
