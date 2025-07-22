cmake_minimum_required(VERSION 3.31)
project(FoxRenderEngine)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "Build Type = ${CMAKE_BUILD_TYPE}")
add_subdirectory(src)

# warning - if you're building in release and still flagging this ON it will not show full debug logs (VK logs etc...)
# but only execution logs (such as what the application is currently doing where is it at rn etc...)
set(_DEFAULT_ENABLE_TERMINAL OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_DEFAULT_ENABLE_TERMINAL ON)
endif()

if(NOT CMAKE_BUILD_TYPE AND "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
    set(_DEFAULT_ENABLE_TERMINAL ON)
endif()

option(ENABLE_TERMINAL "Attach console window in Application" ${_DEFAULT_ENABLE_TERMINAL})
message(STATUS "ENABLE_TERMINAL = ${ENABLE_TERMINAL}")

target_compile_definitions(application PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
        FOX_STRING_IS_ANSI=1
)

if(MSVC)
    target_compile_options(application PRIVATE "/source-charset:windows-1252")
endif()

if(ENABLE_TERMINAL)
    target_compile_definitions(application PRIVATE ENABLE_TERMINAL)
endif()
