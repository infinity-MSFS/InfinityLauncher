cmake_minimum_required(VERSION 3.29)

include(cmake/util.cmake)
include(cmake/colors.cmake)
include(FetchContent)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VCPKG_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-debug")
    message("${Blue}Creating vcpkg at: ${Green}${VCPKG_SOURCE_PATH}${ColorReset}")
else ()
    set(VCPKG_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-release")
    message("${Blue}Creating vcpkg at: ${Green}${VCPKG_SOURCE_PATH}${ColorReset}")
endif ()

set(ENV{VCPKG_ENABLE_MANIFEST_MODE} "OFF")
set(ENV{VCPKG_ROOT} "${VCPKG_SOURCE_PATH}")
if (NOT EXISTS "${VCPKG_SOURCE_PATH}")

    FetchContent_Declare(
            vcpkg
            GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
            GIT_TAG 2024.10.21
            SOURCE_DIR ${VCPKG_SOURCE_PATH}
    )
    FetchContent_MakeAvailable(vcpkg)

    if (WIN32)
        set(BOOTSTRAP_COMMAND "${VCPKG_SOURCE_PATH}/bootstrap-vcpkg.bat")

    else ()
        set(BOOTSTRAP_COMMAND "${VCPKG_SOURCE_PATH}/bootstrap-vcpkg.sh")

    endif ()

    if (WIN32)
        if (NOT EXISTS "${VCPKG_SOURCE_PATH}/vcpkg.exe")
            message("${Green}Bootstrapping vcpkg...${ColorReset}")
            execute_process(
                    COMMAND ${BOOTSTRAP_COMMAND}
                    WORKING_DIRECTORY ${VCPKG_SOURCE_PATH}
            )
        endif ()
    else ()
        if (NOT EXISTS "${VCPKG_SOURCE_PATH}/vcpkg")
            message("${Green}Bootstrapping vcpkg...${ColorReset}")
            execute_process(
                    COMMAND ${BOOTSTRAP_COMMAND}
                    WORKING_DIRECTORY ${VCPKG_SOURCE_PATH}
            )
        endif ()
    endif ()

endif ()

if (WIN32)
    set(VCPKG_TRIPLET "x64-windows-static")
else ()
    set(VCPKG_TRIPLET "x64-linux-static")
endif ()

message("${Blue}Installing OpenSSL${ColorReset}${Green}")
execute_process(
        COMMAND ${VCPKG_SOURCE_PATH}/vcpkg install openssl
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

message("${Blue}Installing Vulkan${ColorReset}${Green}")
execute_process(
        COMMAND ${VCPKG_SOURCE_PATH}/vcpkg install vulkan
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

message("${Blue}Installing Curl${ColorReset}${Green}")
execute_process(
        COMMAND ${VCPKG_SOURCE_PATH}/vcpkg install curl
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

message("${Blue}Installing MiniZip${ColorReset}${Green}")
execute_process(
        COMMAND ${VCPKG_SOURCE_PATH}/vcpkg install minizip
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

message("${Blue}Installing Zoe${ColorReset}${Green}")
execute_process(
        COMMAND ${VCPKG_SOURCE_PATH}/vcpkg install zoe
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)
    message("${Blue}Installing WinToast${ColorReset}${Green}")
    execute_process(
            COMMAND ${VCPKG_SOURCE_PATH}/vcpkg install wintoast
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif ()


set(CMAKE_TOOLCHAIN_FILE "${VCPKG_SOURCE_PATH}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

project(InfinityLauncher)

set(CMAKE_CXX_STANDARD 23)

message("${Blue}Using toolchain file: ${Green}${CMAKE_TOOLCHAIN_FILE}")
find_package(Vulkan REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(unofficial-minizip CONFIG REQUIRED)
find_package(zoe CONFIG REQUIRED PATHS "${VCPKG_SOURCE_PATH}/packages/zoe_${VCPKG_TRIPLET}/")
if (WIN32)
    find_package(unofficial-wintoast REQUIRED)
endif ()

print_system_dependencies_info()

message(STATUS "${BoldMagenta}Checking build Environment: ${ColorReset} ")

message("${Blue}Project: ${Green}${CMAKE_PROJECT_NAME}${ColorReset}")
message("${Blue}Release Profile: ${Green}${CMAKE_BUILD_TYPE}${ColorReset}")
message("${Blue}Compiler: ${Green}${CMAKE_CXX_COMPILER_ID}${ColorReset}")
message("${Blue}Compiler Version: ${Green}${CMAKE_CXX_COMPILER_VERSION}${ColorReset}")
message("${Blue}Fetching Remote Dependencies${Green}")

FetchContent_Declare(GLFW GIT_REPOSITORY https://github.com/TheCherno/glfw.git)
FetchContent_Declare(Boxer GIT_REPOSITORY https://github.com/aaronmjacobs/Boxer.git)
FetchContent_Declare(Infinity GIT_REPOSITORY https://github.com/infinity-MSFS/InfinityLauncherDependencies.git)
if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    FetchContent_Declare(MiniKDENotify GIT_REPOSITORY https://github.com/rikka-gh/MiniKDENotify.git)

    FetchContent_MakeAvailable(GLFW Boxer Infinity MiniKDENotify)
    option(MINIKDENOTIFY_BUILD_EXAMPLES OFF)
else ()
    FetchContent_MakeAvailable(GLFW Boxer Infinity)
endif ()

message("${Blue}Fetched:${Green}GLFW ${Blue}set to:${Green}${glfw_SOURCE_DIR}${ColorReset}")
message("${Blue}Fetched:${Green}Boxer ${Blue}set to:${Green}${boxer_SOURCE_DIR}${ColorReset}")
message("${Blue}Fetched:${Green}InfinityDeps ${Blue}set to:${Green}${infinity_SOURCE_DIR}${ColorReset}")
if (UNIX)
    message("${Blue}Fetched:${Green}MiniKdeNotify ${Blue}set to:${Green}${minikdenotify_SOURCE_DIR}${ColorReset}")
endif ()

if (WIN32)
    message("${Green}Detected Windows${ColorReset}")
    add_compile_definitions(INFINITY_WINDOWS)
elseif (UNIX)
    message(STATUS "${BoldMagenta}Checking Linux specific build Environment: ${ColorReset} ")
    message("${Yellow}The Package Builder functionality is only available on Windows. This will create a build that omits its functionality.${ColorReset}")

    if (INFINITY_USE_X11 AND INFINITY_USE_WAYLAND)
        message(FATAL_ERROR "Cannot build for both X11 and Wayland. Only one can be selected.")
    elseif (INFINITY_USE_X11)
        message("${Blue}Display Protocol: ${Green}X11${ColorReset}")
        add_compile_definitions(INFINITY_X11)
    elseif (INFINITY_USE_WAYLAND)
        message("${Blue}Display Protocol: ${Green}Wayland${ColorReset}")
        add_compile_definitions(INFINITY_WAYLAND)
    else ()
        message(FATAL_ERROR "Must select either X11 or Wayland. Use -DINFINITY_USE_X11=ON -DINFINITY_USE_WAYLAND=OFF or -DINFINITY_USE_X11=OFF -DINFINITY_USE_WAYLAND=ON for Linux build.")
    endif ()
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()

message(STATUS "${BoldMagenta}Environment configuration complete")
message("${Cyan}")

message("${Blue}Gathering Infinity Launcher Source Files")
add_executable(InfinityLauncher
        # -- Main Source File --
        src/main.cpp
        # -- Backend Source Files --
        src/Backend/Application/Application.cpp
        src/Backend/Application/Application.hpp
        src/Backend/Image/Image.cpp
        src/Backend/Image/Image.hpp
        src/Backend/Layer/Layer.hpp
        src/Backend/UIHelpers/UiHelpers.cpp
        src/Backend/UIHelpers/UiHelpers.hpp
        src/Backend/VulkanManager/VulkanManager.cpp
        src/Backend/VulkanManager/VulkanManager.hpp
        # -- Util Source Files --
        src/Util/State/State.hpp
        # -- Frontend Source Files --
        src/Frontend/Theme/Theme.cpp
        src/Frontend/Theme/Theme.hpp
)

message("${Blue}Gathering ImGui Source Files")
file(GLOB IMGUI_SOURCES
        ${infinity_SOURCE_DIR}/src/imgui/
        ${infinity_SOURCE_DIR}/src/imgui/imgui.cpp
        ${infinity_SOURCE_DIR}/src/imgui/imgui_draw.cpp
        ${infinity_SOURCE_DIR}/src/imgui/imgui_widgets.cpp
        ${infinity_SOURCE_DIR}/src/imgui/imgui_tables.cpp
        ${infinity_SOURCE_DIR}/src/imgui/misc/cpp/imgui_stdlib.cpp
        ${infinity_SOURCE_DIR}/src/imgui/backends/imgui_impl_glfw.cpp
        ${infinity_SOURCE_DIR}/src/imgui/backends/imgui_impl_vulkan.cpp
)
message("${Blue}Gathering Stb Image Source Files")
file(GLOB STB_IMAGE ${infinity_SOURCE_DIR}/include/stb_image/stb_image.h)
message("${Blue}Gathering JSON Source Files")
file(GLOB JSON_SOURCES ${infinity_SOURCE_DIR}/include/Json/*.hpp)
target_sources(InfinityLauncher PRIVATE ${STB_IMAGE} ${IMGUI_SOURCES} ${JSON_SOURCES})

message("${Blue}Setting Link Paths")
if (UNIX)
    target_include_directories(InfinityLauncher PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${infinity_SOURCE_DIR}/include
            ${infinity_SOURCE_DIR}/src/imgui
            ${ZOE_INCLUDE_DIR}
            ${MINIZIP_INCLUDE_DIR}
            ${minikdenotify_SOURCE_DIR}/include
            ${DBUS_INCLUDE_DIRS}
    )
else ()
    target_include_directories(InfinityLauncher PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${infinity_SOURCE_DIR}/include
            ${infinity_SOURCE_DIR}/src/imgui
            ${ZOE_INCLUDE_DIR}
            ${MINIZIP_INCLUDE_DIR}
    )
endif ()

target_compile_definitions(InfinityLauncher PRIVATE
        $<$<CONFIG:Release>:RELEASE_DIST>
)

if (WIN32)
    message("${Blue}Setting Up Windows Console Configuration")
    target_link_options(InfinityLauncher PRIVATE
            $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
            $<$<NOT:$<CONFIG:Release>>:/SUBSYSTEM:CONSOLE>
    )
    message("${Blue}Setting Up Zoe Dynamic Link")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ZOE_LIB ${VCPKG_SOURCE_PATH}/packages/zoe_x64-windows/debug/lib/libZoe-d.lib)
        add_custom_command(TARGET InfinityLauncher POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${VCPKG_SOURCE_PATH}/packages/zoe_x64-windows/debug/bin/libZoe-d.dll
                $<TARGET_FILE_DIR:InfinityLauncher>
        )
    else ()
        set(ZOE_LIB ${VCPKG_SOURCE_PATH}/packages/zoe_x64-windows/lib/libZoe.lib)
        add_custom_command(TARGET InfinityLauncher POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${VCPKG_SOURCE_PATH}/packages/zoe_x64-windows/bin/libZoe.dll
                $<TARGET_FILE_DIR:InfinityLauncher>
        )
    endif ()

    set(TOAST_LIB unofficial::wintoast::wintoast)

    message("${Blue}Setting Up OpenSSL Dynamic Link")
    add_custom_command(TARGET InfinityLauncher POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${infinity_SOURCE_DIR}/bin/libcrypto-3-x64.dll
            $<TARGET_FILE_DIR:InfinityLauncher>
    )
else ()

    if (CMAKE_BUILD_TYPE MATCHES DEBUG)
        set(ZOE_LIB ${VCPKG_SOURCE_PATH}/packages/zoe_x64-linux/debug/lib/liblibZoe-d.a)
    else ()
        set(ZOE_LIB ${VCPKG_SOURCE_PATH}/packages/zoe_x64-linux/lib/liblibZoe.a)
    endif ()
endif ()
message("${Blue}Configuring Linker")
target_link_libraries(InfinityLauncher PRIVATE Vulkan::Vulkan glfw Boxer CURL::libcurl OpenSSL::SSL OpenSSL::Crypto unofficial::minizip::minizip ${ZOE_LIB} ${TOAST_LIB})