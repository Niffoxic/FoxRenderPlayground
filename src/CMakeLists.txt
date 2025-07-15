# Source Cmake
include(FetchContent)

# bro if you dont have vulkan installed I'm not gonna automate it!, too much work (DO IT MANUALLY AND BY YOURSELF)
find_package(Vulkan REQUIRED)

# ========================= Link Imgui =========================
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

# Labour Stuff hehehe
target_link_libraries(application PRIVATE Vulkan::Vulkan)
target_compile_definitions(application PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_include_directories(
        application
        PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR} # Its just what I prefer
        imgui_lib
)
