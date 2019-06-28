
cmake_minimum_required(VERSION 3.5)

# This section turns on and off this feature
option( INCLUDE_LIBSELECT_CB "Include libselect_cb" ON )
if( NOT ${INCLUDE_LIBSELECT_CB} )
    return()
endif()

project(select_cb)

add_library(select_cb select_cb.c)

target_include_directories(select_cb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(${BUILD_SHARED_LIBS})
    install(TARGETS    select_cb       DESTINATION /usr/local/lib )
endif()