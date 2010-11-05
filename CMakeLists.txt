cmake_minimum_required(VERSION 2.6)
PROJECT(smbtad C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(APPLICATION_NAME "smbtad")
set(APPLICATION_VERSION "1.0")

set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME 
 "${APPLICATION_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES ".git/;CMakeFiles/")
include(CPack)
#include(CPackConfig.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/FindSqlite3.cmake)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS_DEBUG "-g -Wall")
#SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(SQLITE3_MIN_VERSION "3.7.0")
FIND_PACKAGE(Sqlite3 REQUIRED)
SET(CMAKE_INSTALL_PREFIX /usr)
SET(Libraries
#list libraries here
sqlite3
pthread
rt
dl
iniparser
talloc
)


SET(PLATFORM_LINKFLAGS "-pthread")

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


ADD_CUSTOM_TARGET(createLinks COMMAND ${CMAKE_COMMAND} -E create_symlink /etc/init.d/smbtad /usr/sbin/rcsmbtad)


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
	src/monitor-list.c
	src/sendlist.c
	src/throughput-list.c)
ADD_DEPENDENCIES(${CurrentExe} createLinks)

TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
INSTALL(TARGETS smbtad DESTINATION bin)
INSTALL(PROGRAMS dist/smbtad DESTINATION /etc/init.d/)

