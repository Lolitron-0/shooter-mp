cmake_minimum_required(VERSION 3.15)
project(shooter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if(NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if(NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES
        OFF
        CACHE BOOL "" FORCE)
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

set(BUILD_STATIC_LIBS ON)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(thirdparty/GameNetworkingSockets)

add_subdirectory(thirdparty/entt)

set(JSON_BuildTests
    OFF
    CACHE INTERNAL "")
add_subdirectory(thirdparty/json)

find_package(
  Boost
  COMPONENTS program_options
  REQUIRED)

add_subdirectory(shared)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(entrypoint)
