cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE WinEcho_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE WinEcho_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library (WinEcho SHARED ${WinEcho_SOURCES} ${WinEcho_HEADERS})
target_include_directories(WinEcho PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(WinEcho PRIVATE "WINECHO_BUILD")