cmake_minimum_required(VERSION 2.6)
PROJECT(stad C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(APPLICATION_NAME "stad2")
set(APPLICATION_VERSION "0.0.1")

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME 
 "${APPLICATION_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES ".git/;CMakeFiles/")
include(CPack)
#include(CPackConfig.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/FindSqlite3.cmake)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS_DEBUG "-g")
#SET(CMAKE_VERBOSE_MAKEFILE ON)
FIND_PACKAGE(Sqlite3 REQUIRED)
SET(CMAKE_INSTALL_PREFIX /usr)
SET(Libraries
#list libraries here
sqlite3
pthread
rt
dl
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




SET(CurrentExe "stad2")
ADD_EXECUTABLE(${CurrentExe} 
	src/main.c 
	src/configuration.c 
	src/help.c 
	src/daemon.c
	src/connection_list.c 
	src/network.c 
	src/log.c
	src/protocol.c )
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})




INSTALL(TARGETS stad2 DESTINATION bin)

