#first setup the includes and link paths
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)

if(MYSQL_INCLUDE_DIR)
    include_directories(${MYSQL_INCLUDE_DIR})
endif(MYSQL_INCLUDE_DIR)
if(SQLITE3_INCLUDE_DIR)
    include_directories(${SQLITE3_INCLUDE_DIR})
endif(SQLITE3_INCLUDE_DIR)

link_directories(${CMAKE_BINARY_DIR}/src/common 
${CMAKE_BINARY_DIR}/src/compat
${CMAKE_BINARY_DIR}/src/tinycdb
${CMAKE_BINARY_DIR}/src/win32)

#this is needed to be compatible with existent code
add_definitions("-DHAVE_CONFIG_H")

if(WITH_FLAG_WALL)
    add_definitions("-Wall")
endif(WITH_FLAG_WALL)

if(WITH_FLAG_ANSIPEDANTIC)
    add_definitions("-pedantic -ansi")
endif(WITH_FLAG_ANSIPEDANTIC)

if(MYSQL_FOUND)
    add_definitions("-DWITH_SQL_MYSQL")
endif(MYSQL_FOUND)
if(SQLITE3_FOUND)
    add_definitions("-DWITH_SQL_SQLITE3")
endif(SQLITE3_FOUND)

add_subdirectory(compat)
add_subdirectory(common)
add_subdirectory(tinycdb)
add_subdirectory(win32)
add_subdirectory(bntrackd)
add_subdirectory(client)
add_subdirectory(bniutils)
add_subdirectory(bnpass)

if(WITH_BNETD)
    add_subdirectory(bnetd)
endif(WITH_BNETD)

if(WITH_D2CS)
    add_subdirectory(d2cs)
endif(WITH_D2CS)

if(WITH_D2DBS)
    add_subdirectory(d2dbs)
endif(WITH_D2DBS)

if(HAVE_PCAP_H AND HAVE_LIBPCAP)
    add_subdirectory(bnpcap)
endif(HAVE_PCAP_H AND HAVE_LIBPCAP)
