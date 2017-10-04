
project(SIDC_lib C)

execute_process(COMMAND perl -MExtUtils::Embed -e ccopts OUTPUT_VARIABLE PERL_ADD_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND php-config --includes OUTPUT_VARIABLE PHP_ADD_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND python-config --cflags OUTPUT_VARIABLE PYTHON_ADD_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
set(IDL_ADD_CFLAGS " ")

if(NOT PERL_ADD_CFLAGS)
    message(FATAL_ERROR "Cannot find Perl-dev")
endif()

if(NOT PHP_ADD_CFLAGS)
    message(FATAL_ERROR "Cannot find PHP-dev")
endif()

if(NOT PYTHON_ADD_CFLAGS)
    message(FATAL_ERROR "Cannot find Python-dev")
endif()

# OS X clang 5.1 & Python 2.7.5
#if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
#    set(PYTHON_ADD_CFLAGS "${PYTHON_ADD_CFLAGS} -Wno-error=unused-command-line-argument-hard-error-in-future")
#endif()

# out of P2SC system (no real send to LMAT)
add_definitions(-DSTANDALONE)

macro(build_binding bound target destination addfile ADD_CFLAGS libs)
    add_library(${target} MODULE ${bound}.c)
    set_target_properties(${target}
        PROPERTIES
            COMPILE_FLAGS ${ADD_CFLAGS}
            OUTPUT_NAME ${bound}
            PREFIX ""
            INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/${destination})

    target_link_libraries(${target} ${libs})

    if(APPLE)
        set_target_properties(${target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif(APPLE)

    install(TARGETS ${target} DESTINATION ${destination})
    install(FILES ${addfile} DESTINATION ${destination})
endmacro()

macro(build_idl_runtime target runtime routine files)
    set(COMPILE_CSH ${CMAKE_CURRENT_BINARY_DIR}/${target}_compile.csh)
    configure_file(${CMAKE_SOURCE_DIR}/libidl/ssw_compile.csh ${COMPILE_CSH})

    add_custom_command(OUTPUT ${runtime} COMMAND ${COMPILE_CSH} DEPENDS ${files} ${COMPILE_CSH})
    add_custom_target(${target} ALL DEPENDS ${runtime})
endmacro()

add_subdirectory(send2LMAT)
add_subdirectory(send2LMAT/bindings)
add_subdirectory(libp2sc)
add_subdirectory(libswap)

add_subdirectory(libsidc_msg)

