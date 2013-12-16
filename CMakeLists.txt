cmake_minimum_required (VERSION 2.8)

project(menugen)

set(CFLAGS -pipe -Wall -pedantic)

if(DEBUG)
    set(CFLAGS ${CFLAGS} -g3 -DDEBUG)
else()
    set(CFLAGS ${CFLAGS} -O3 -g3)
endif()

add_definitions(-std=c99)
add_definitions(${CFLAGS})

file(GLOB SRC source/*.c)

find_package(PkgConfig)
pkg_search_module(GARCON garcon-1) 
pkg_search_module(GTK gtk+-3.0)

include_directories(${GARCON_INCLUDE_DIRS} ${GTK_INCLUDE_DIRS})
link_directories(${GARCON_LIBRARY_DIRS} ${GTK_LIBRARY_DIRS})

add_executable(menugen ${SRC})
target_link_libraries(menugen ${GARCON_LIBRARIES} ${GTK_LIBRARIES})