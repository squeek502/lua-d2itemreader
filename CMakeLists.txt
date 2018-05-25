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

add_subdirectory(d2itemreader EXCLUDE_FROM_ALL)

add_library(lua_d2itemreader MODULE lua_d2itemreader.c)
set_target_properties( lua_d2itemreader PROPERTIES OUTPUT_NAME "d2itemreader" PREFIX "" )
target_link_libraries( lua_d2itemreader d2itemreader )
target_include_directories( lua_d2itemreader PRIVATE "d2itemreader/src" )
target_include_directories( lua_d2itemreader PRIVATE ${LUA_INCLUDE_DIR} )

if(WIN32)
  target_compile_definitions( lua_d2itemreader PRIVATE _CRT_SECURE_NO_WARNINGS )
  # Only link against the Lua library on Windows
  target_link_libraries( lua_d2itemreader ${LUA_LIBRARIES} )
endif()

# Needed to avoid getting undefined references to liblua
if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -undefined dynamic_lookup")
endif()