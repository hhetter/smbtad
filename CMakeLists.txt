cmake_minimum_required(VERSION 2.6)
PROJECT(smbtad C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(APPLICATION_NAME "smbtad")
set(APPLICATION_VERSION "1.0")

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS "-Wall -D_FORTIFY_SOURCE=2")
#SET(CMAKE_VERBOSE_MAKEFILE ON)

#
# check for libdbi
#
include(FindLibDBI.cmake)
FIND_PACKAGE(LibDBI)
if (NOT LIBDBI_INCLUDE_DIR)
	MESSAGE( FATAL_ERROR "ERROR: you will need the libdbi development files installed.")
ENDIF()
if (LIBDBI_INCLUDE_DIR)
	INCLUDE_DIRECTORIES(${LIBDBI_INCLUDE_DIR})
ENDIF()
#
# check for talloc
#
include(FindTalloc.cmake)
FIND_PACKAGE(Talloc)
if (NOT TALLOC_INCLUDE_DIR)
        MESSAGE( FATAL_ERROR "ERROR: you will need talloc development
                 files installed.")
ENDIF()
 
if (TALLOC_INCLUDE_DIR)
       INCLUDE_DIRECTORIES(${TALLOC_INCLUDE_DIR})
ENDIF()

# compile on SunOS
if (CMAKE_SYSTEM_NAME STREQUAL "SunOS" )
       ADD_DEFINITIONS(-DSOLARIS)
       SET(Libraries
               ${Libraries}
               socket
               #fuer asprintf:
               iberty
       )
endif()

#
# check for pthreads
#
FIND_PACKAGE(Threads)
if (NOT CMAKE_USE_PTHREADS_INIT)
        MESSAGE( FATAL_ERROR "ERROR: you will need pthread development files installed.")
ENDIF()



#
# include iniparser on demand
#
include(FindIniparser.cmake)
FIND_PACKAGE(Iniparser)
if (NOT INIPARSER_INCLUDE_DIR)
        MESSAGE( STATUS "----> FYI: Unable to find iniparser libraries on your system")
        MESSAGE( STATUS "---->      compiling my own version and link it statically.")
        add_custom_target(
                buildiniparser ALL
                COMMAND "make"
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/iniparser3.0b"
                COMMENT "Building iniparser."
                VERBATIM )
        ADD_LIBRARY(iniparser STATIC IMPORTED)
        set_target_properties(iniparser PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/iniparser3.0b/libiniparser.a)
ENDIF()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_symlink.cmake "exec_program(${CMAKE_COMMAND} ARGS -E create_symlink /etc/init.d/smbtad /usr/sbin/rcsmbtad)")


INCLUDE_DIRECTORIES( ${TALLOC_INCLUDE_DIR} )

SET(CurrentExe "smbtad")
ADD_EXECUTABLE(${CurrentExe}
	src/main.c 
	src/configuration.c 
	src/help.c 
	src/daemon.c
	src/connection_list.c 
	src/network.c 
	src/protocol.c
	src/cache.c
	src/query_list.c
	src/database.c
	src/crypto/aes.c
	src/crypto/rijndael-alg-fst.c
	src/monitor-list.c)
TARGET_LINK_LIBRARIES(${CurrentExe} ${INIPARSER_LIBRARIES} ${TALLOC_LIBRARIES} ${Libraries} iniparser ${CMAKE_THREAD_LIBS_INIT} dl)
INSTALL(TARGETS smbtad DESTINATION bin)

IF (CMAKE_INSTALL_RCFILE)
	INSTALL(PROGRAMS dist/smbtad DESTINATION /etc/init.d/)
	INSTALL(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_symlink.cmake)
ENDIF()
