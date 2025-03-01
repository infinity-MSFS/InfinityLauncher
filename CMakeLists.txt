cmake_minimum_required(VERSION 3.29)

include(cmake/util.cmake)
include(cmake/colors.cmake)
include(cmake/key_loading.cmake)
include(FetchContent)


if (WIN32)
    set(VCPKG_TRIPLET "x64-windows-static")
else ()
    set(VCPKG_TRIPLET "x64-linux-static")
endif ()
if (NOT VCPKG_ROOT)
    set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif ()
if (VCPKG_ROOT)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message("${Blue}Using Toolchain file: ${Green}${CMAKE_TOOLCHAIN_FILE}${ColorReset}")
else ()
    message(FATAL_ERROR "VCPKG_ROOT is not set in the environment!")
endif ()


load_keys()

project(InfinityLauncher)

set(CMAKE_CXX_STANDARD 23 REQUIRED)

message("${Blue}Using toolchain file: ${Green}${CMAKE_TOOLCHAIN_FILE}")
find_package(ZLIB REQUIRED)
find_package(Vulkan REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(msgpack-cxx CONFIG REQUIRED)
find_package(unofficial-minizip CONFIG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(zoe CONFIG REQUIRED)
if (WIN32)
    find_package(unofficial-wintoast REQUIRED)
endif ()

if (UNIX)
    set(GLFW_BUILD_WAYLAND ON)
    find_package(glfw3 CONFIG REQUIRED)

endif ()

print_system_dependencies_info()

message(STATUS "${BoldMagenta}Checking build Environment: ${ColorReset} ")

message("${Blue}Project: ${Green}${CMAKE_PROJECT_NAME}${ColorReset}")
message("${Blue}Release Profile: ${Green}${CMAKE_BUILD_TYPE}${ColorReset}")
message("${Blue}Compiler: ${Green}${CMAKE_CXX_COMPILER_ID}${ColorReset}")
message("${Blue}Compiler Version: ${Green}${CMAKE_CXX_COMPILER_VERSION}${ColorReset}")
message("${Blue}Fetching Remote Dependencies${Green}")

if (WIN32)
    FetchContent_Declare(GLFW GIT_REPOSITORY https://github.com/TheCherno/glfw.git)
endif ()
FetchContent_Declare(Boxer GIT_REPOSITORY https://github.com/aaronmjacobs/Boxer.git)
FetchContent_Declare(Infinity GIT_REPOSITORY https://github.com/infinity-MSFS/InfinityLauncherDependencies.git)
if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    FetchContent_Declare(MiniKDENotify GIT_REPOSITORY https://github.com/rikka-gh/MiniKDENotify.git)

    FetchContent_MakeAvailable(Boxer Infinity MiniKDENotify)
else ()
    FetchContent_MakeAvailable(GLFW Boxer Infinity)
endif ()

if (WIN32)
    message("${Blue}Fetched:${Green}GLFW ${Blue}set to:${Green}${glfw_SOURCE_DIR}${ColorReset}")
endif ()
message("${Blue}Fetched:${Green}Boxer ${Blue}set to:${Green}${boxer_SOURCE_DIR}${ColorReset}")
message("${Blue}Fetched:${Green}InfinityDeps ${Blue}set to:${Green}${infinity_SOURCE_DIR}${ColorReset}")
if (UNIX)
    message("${Blue}Fetched:${Green}MiniKdeNotify ${Blue}set to:${Green}${minikdenotify_SOURCE_DIR}${ColorReset}")
endif ()

if(NOT INFINITY_USE_X11 AND NOT INFINITY_USE_WAYLAND AND UNIX) 
  set(INFINITY_USE_WAYLAND ON)
endif()


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
        src/Backend/VulkanManager/VulkanManager.hpp
        src/Backend/Router/Router.cpp
        src/Backend/Router/Router.hpp
        src/Backend/Encryption/Encryption.cpp
        src/Backend/Encryption/Encryption.hpp
        src/Backend/Encryption/Encryption.cpp
        src/Backend/Encryption/Encryption.hpp

        # -- Util Source Files --
        src/Util/Easing/Easing.hpp
        src/Util/State/State.hpp
        src/Util/Error/Error.hpp
        src/Util/State/GroupStateManager.hpp
        src/Util/State/RenderGroupData.hpp

        # -- Frontend Source Files --
        src/Frontend/Theme/Theme.cpp
        src/Frontend/Theme/Theme.hpp
        src/Frontend/ColorInterpolation/ColorInterpolation.cpp
        src/Frontend/ColorInterpolation/ColorInterpolation.hpp
        src/Frontend/Background/Background.cpp
        src/Frontend/Background/Background.hpp
        src/Frontend/SVG/SVGDrawing.hpp
        src/Frontend/Pages/Downloads/Downloads.cpp
        src/Frontend/Pages/Downloads/Downloads.hpp
        src/Frontend/Pages/Home/Home.cpp
        src/Frontend/Pages/Home/Home.hpp
        src/Frontend/Pages/Settings/Settings.cpp
        src/Frontend/Pages/Settings/Settings.hpp
        src/Frontend/Pages/Project/Project.cpp
        src/Frontend/Pages/Project/Project.hpp
        src/Backend/Downloads/Downloads.cpp
        src/Backend/Downloads/Downloads.hpp
        src/Backend/ZipExtractor/ZipExtractor.cpp
        src/Backend/ZipExtractor/ZipExtractor.hpp
        src/Backend/Installer/Installer.cpp
        src/Backend/Installer/Installer.hpp
        src/Util/GroupUtil/GroupUtil.hpp
        src/Backend/Updater/Updater.cpp
        src/Backend/Updater/Updater.hpp
)


message("${Blue}Gathering Updater Source Files")
add_executable(Updater src-updater/main.cpp)

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
            ${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TRIPLET}/include
            ${MINIZIP_INCLUDE_DIR}
            ${minikdenotify_SOURCE_DIR}/include
            keys/include
            ${DBUS_INCLUDE_DIRS}
    )
else ()
    target_include_directories(InfinityLauncher PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${infinity_SOURCE_DIR}/include
            ${infinity_SOURCE_DIR}/src/imgui
            ${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TRIPLET}/include
            keys/include
            ${MINIZIP_INCLUDE_DIR}
    )
endif ()

target_compile_definitions(InfinityLauncher PRIVATE
        $<$<CONFIG:Release>:RELEASE_DIST>
)

if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

    configure_file(
            "resources.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
    )

    target_sources(Updater PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
    )

    if (MSVC)
        set_target_properties(Updater PROPERTIES
                LINK_FLAGS "/MANIFEST:NO"
                VS_GLOBAL_AppLocal "false"
        )
    endif ()
endif ()


if (WIN32)
    message("${Blue}Setting Up Windows Console Configuration")
    target_link_options(InfinityLauncher PRIVATE
            $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
            $<$<NOT:$<CONFIG:Release>>:/SUBSYSTEM:CONSOLE>
    )

    set(TOAST_LIB unofficial::wintoast::wintoast)

    message("${Blue}Setting Up OpenSSL Dynamic Link")
    add_custom_command(TARGET InfinityLauncher POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${infinity_SOURCE_DIR}/bin/libcrypto-3-x64.dll
            $<TARGET_FILE_DIR:InfinityLauncher>
    )
endif ()
message("${Blue}Configuring Linker")
if (WIN32)
    set(ZOE_LIB zoe::zoe)
else ()
    set(ZOE_LIB zoe::zoe-static)
endif ()
target_link_libraries(InfinityLauncher PRIVATE WebP::webp WebP::webpdecoder ${ZOE_LIB} WebP::webpdemux Vulkan::Vulkan glfw Boxer CURL::libcurl OpenSSL::SSL OpenSSL::Crypto unofficial::minizip::minizip ZLIB::ZLIB msgpack-cxx ${TOAST_LIB})

add_custom_target(InfinityLauncherDist)
add_dependencies(InfinityLauncherDist
        InfinityLauncher
        Updater
)
