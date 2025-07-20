# Build Location
set(PROJECT_SOURCE "${CMAKE_SOURCE_DIR}")
set(OUTPUT_BASE "${CMAKE_BINARY_DIR}/FoxRenderEngine")

file(WRITE "${CMAKE_SOURCE_DIR}/resource_paths.txt"
        "OUTPUT_BASE=${OUTPUT_BASE}\n"
        "PROJECT_SOURCE=${PROJECT_SOURCE}\n"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BASE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BASE}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BASE}/libs/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BASE}/libs/Release")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BASE}/libs/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BASE}/libs/Release")

find_package(Vulkan REQUIRED)

# ========================= Link Imgui =========================
include(FetchContent)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
)
set(IMGUI_BACKENDS
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR}/backends)
# ========================= Linked Imgui =========================

# Main Stuff
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
add_executable(application WIN32 main.cpp ${SOURCE_FILES})

# Rename executable based on build type
set_target_properties(application PROPERTIES
        OUTPUT_NAME_DEBUG "engine-debug"
        OUTPUT_NAME_RELEASE "engine"
)

# Labour Stuff hehehe
target_link_libraries(application PRIVATE Vulkan::Vulkan)
target_compile_definitions(application PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_include_directories(
        application
        PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR} # Its just what I prefer
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils # same reason I dont like utils then this then that....
        imgui_lib
)
