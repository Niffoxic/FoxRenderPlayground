# --- Project Paths (User Configurable) ---

# Project source directory (readonly internal)
set(PROJECT_SOURCE "${CMAKE_SOURCE_DIR}")

# Output folder for binaries
set(OUTPUT_BASE "${CMAKE_BINARY_DIR}/FoxRenderPlayground" CACHE PATH "Base output directory for all build artifacts")

# Shader paths
set(SHADER_INPUT_PATH "shaders" CACHE PATH "Input directory for raw shader source files")
set(SHADER_OUTPUT_PATH "compiled_shaders" CACHE PATH "Output directory for compiled SPIR-V shaders")

# Vulkan SDK path (optional override)
set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}" CACHE PATH "Path to Vulkan SDK root directory (defaults to env: VULKAN_SDK)")

# Write resource path file (auto-generated)
file(WRITE "${CMAKE_SOURCE_DIR}/resource_paths.txt"
        "# AUTO_GENERATED_FILE_PLEASE_IGNORE\n"
        "OUTPUT_BASE=${OUTPUT_BASE}\n"
        "PROJECT_SOURCE=${PROJECT_SOURCE}\n"
        "SHADER_INPUT_PATH=${SHADER_INPUT_PATH}\n"
        "SHADER_OUTPUT_PATH=${SHADER_OUTPUT_PATH}\n"
        "VULKAN_SDK=${VULKAN_SDK_PATH}\n"
)

# --- Output Directories ---
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

# ========================= Link GLM =========================
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(glm)

# ========================= Linked GLM =========================

# ========================= Linked stb =========================

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)
# ========================= Linked stb =========================

# Main Stuff
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
add_executable(application WIN32 main.cpp ${SOURCE_FILES})

# Rename executable based on build type
set_target_properties(application PROPERTIES
        OUTPUT_NAME_DEBUG "playground-debug"
        OUTPUT_NAME_RELEASE "playground"
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
        ${glm_SOURCE_DIR}
        ${stb_SOURCE_DIR}
)

set(SHADER_COMPILER "${PROJECT_SOURCE}/setup/compile_shaders.py")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

file(GLOB_RECURSE SHADER_INPUTS
        "${PROJECT_SOURCE}/${SHADER_INPUT_PATH}/*.vert"
        "${PROJECT_SOURCE}/${SHADER_INPUT_PATH}/*.frag"
)

add_custom_command(
        OUTPUT "${OUTPUT_BASE}/${SHADER_OUTPUT_PATH}/.timestamp"  # dummy file for tracking
        COMMAND "${Python3_EXECUTABLE}" "${SHADER_COMPILER}"
        --input "${PROJECT_SOURCE}/${SHADER_INPUT_PATH}"
        --output "${OUTPUT_BASE}/${SHADER_OUTPUT_PATH}"
        --vulkan "${VULKAN_SDK_PATH}"
        COMMAND ${CMAKE_COMMAND} -E touch "${OUTPUT_BASE}/${SHADER_OUTPUT_PATH}/.timestamp"
        DEPENDS ${SHADER_INPUTS}
        COMMENT "Auto-compiling shaders due to changes"
)

add_custom_target(CompileShaders ALL
        DEPENDS "${OUTPUT_BASE}/${SHADER_OUTPUT_PATH}/.timestamp"
)
