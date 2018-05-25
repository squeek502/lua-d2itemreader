cmake_minimum_required(VERSION 2.6)
project(lua-d2itemreader C)

if(UNIX)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(WIN32)
  find_package (Lua REQUIRED)
else()
  # Only the include dir matters on UNIX
  find_package (Lua)
  if(NOT LUA_INCLUDE_DIR)
    message( FATAL_ERROR "Variable `LUA_INCLUDE_DIR' expected to be defined.")
  endif()
endif()

add_library(d2itemreader MODULE d2itemreader.c)
set_target_properties( d2itemreader PROPERTIES PREFIX "" )
target_link_libraries( d2itemreader "${PROJECT_SOURCE_DIR}/../d2itemreader/build/libd2itemreader.a" )
target_include_directories( d2itemreader PRIVATE "../d2itemreader/src" )
target_include_directories( d2itemreader PRIVATE ${LUA_INCLUDE_DIR} )

if(WIN32)
  target_compile_definitions( d2itemreader PRIVATE _CRT_SECURE_NO_WARNINGS )
  # Only link against the Lua library on Windows
  target_link_libraries( d2itemreader ${LUA_LIBRARIES} )
endif()

# Needed to avoid getting undefined references to liblua
if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -undefined dynamic_lookup")
endif()