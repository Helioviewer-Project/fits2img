
execute_process(COMMAND mysql_config --include OUTPUT_VARIABLE MSQL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND mysql_config --libs    OUTPUT_VARIABLE MSQL_LIBS   OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pg_config --includedir OUTPUT_VARIABLE PSQL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pg_config --libdir     OUTPUT_VARIABLE PSQL_LIBS   OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT MSQL_CFLAGS OR NOT MSQL_LIBS)
    message(FATAL_ERROR "Cannot find MySQL-dev")
endif()

if(NOT PSQL_CFLAGS OR NOT PSQL_LIBS)
    message(FATAL_ERROR "Cannot find PostgreSQL-dev")
endif()

aux_source_directory(src/db            ZDB_SRCS)
aux_source_directory(src/db/mysql      ZDB_MSQL_SRCS)
aux_source_directory(src/db/postgresql ZDB_PSQL_SRCS)
aux_source_directory(src/db/sqlite     ZDB_LSQL_SRCS)
aux_source_directory(src/exceptions    ZEXCEPTIONS_SRCS)
aux_source_directory(src/net           ZNET_SRCS)
aux_source_directory(src/system        ZSYSTEM_SRCS)
aux_source_directory(src/util          ZUTIL_SRCS)

include_before(src src/db src/db/sqlite src/exceptions src/net src/util)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O3 -funsigned-char")

if(MAINTAINER)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -Wno-pointer-sign -Wno-unused -Wno-cast-qual -Wno-c++-compat")
endif(MAINTAINER)

add_definitions(
    -DSTDC_HEADERS=1
    -DHAVE_SYS_TYPES_H=1
    -DHAVE_SYS_STAT_H=1
    -DHAVE_STDLIB_H=1
    -DHAVE_STRING_H=1
    -DHAVE_MEMORY_H=1
    -DHAVE_STRINGS_H=1
    -DHAVE_INTTYPES_H=1
    -DHAVE_STDINT_H=1
    -DHAVE_UNISTD_H=1
    -DHAVE_DLFCN_H=1
    -DHAVE_FDATASYNC=1
    -DHAVE_USLEEP=1
    -DHAVE_LOCALTIME_R=1
    -DHAVE_GMTIME_R=1
#   -DNDEBUG
    -DSQLITE_DEFAULT_CACHE_SIZE=4096
    -DSQLITE_DEFAULT_FILE_FORMAT=1
    -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_DEFAULT_PAGE_SIZE=4096
#   -DSQLITE_THREADSAFE=0
    -DSQLITE_TEMP_STORE=3
    -DSQLITE_DISABLE_DIRSYNC=1
#   -DSQLITE_OMIT_AUTOINIT=1
    -DSQLITE_OMIT_BUILTIN_TEST=1
    -DSQLITE_OMIT_DEPRECATED=1
    -DSQLITE_OMIT_LOAD_EXTENSION=1
#   -DSQLITE_OMIT_SHARED_CACHE=1
    -DSQLITE_OMIT_TRACE=1
    -DSQLITE_OMIT_UTF16=1)

add_library(zdb SHARED
    ${ZDB_SRCS}
    ${ZDB_MSQL_SRCS}
    ${ZDB_PSQL_SRCS}

#    ${ZDB_LSQL_SRCS}
    zdbsqlite.c

    ${ZEXCEPTIONS_SRCS}
    ${ZNET_SRCS}
    ${ZSYSTEM_SRCS}
    ${ZUTIL_SRCS})

set(COMPILE_FLAGS "${MSQL_CFLAGS} -I${PSQL_CFLAGS}")
set_target_properties(zdb PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})

set(LINK_FLAGS "${MSQL_LIBS} -L${PSQL_LIBS} -lpq")
target_link_libraries(zdb ${LINK_FLAGS} pthread)

sidc_install_lib(zdb)
