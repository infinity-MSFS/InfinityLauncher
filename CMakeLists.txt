cmake_minimum_required(VERSION 3.2..4.0)

cmake_policy(VERSION 3.5)

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


project(InfinityLauncher)

cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Bin2Header src-bin2header/main.cpp)
load_keys()


message("${Blue}Using toolchain file: ${Green}${CMAKE_TOOLCHAIN_FILE}")
find_package(ZLIB REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(NanoSVG CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(msgpack-cxx CONFIG REQUIRED)
find_package(unofficial-minizip CONFIG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(zoe CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
if (WIN32)
    find_package(unofficial-wintoast REQUIRED)
    find_package(Vulkan REQUIRED)
endif ()

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
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
FetchContent_Declare(Traypp GIT_REPOSITORY https://github.com/Soundux/traypp.git)
FetchContent_Declare(Boxer GIT_REPOSITORY https://github.com/aaronmjacobs/Boxer.git)
FetchContent_Declare(Infinity GIT_REPOSITORY https://github.com/infinity-MSFS/InfinityLauncherDependencies.git)
if (UNIX)
    FetchContent_MakeAvailable(Boxer Infinity Traypp)
else ()
    FetchContent_MakeAvailable(GLFW Boxer Infinity Traypp)
endif ()

#add_subdirectory(${traypp_SOURCE_DIR} EXCLUDE_FROM_ALL)

if (WIN32)
    message("${Blue}Fetched:${Green}GLFW ${Blue}set to:${Green}${glfw_SOURCE_DIR}${ColorReset}")
endif ()
message("${Blue}Fetched:${Green}Boxer ${Blue}set to:${Green}${boxer_SOURCE_DIR}${ColorReset}")
message("${Blue}Fetched:${Green}InfinityDeps ${Blue}set to:${Green}${infinity_SOURCE_DIR}${ColorReset}")


if (NOT INFINITY_USE_X11 AND NOT INFINITY_USE_WAYLAND AND UNIX)
    set(INFINITY_USE_WAYLAND ON)
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
        src/Backend/Router/Router.cpp
        src/Backend/Router/Router.hpp
        src/Backend/Encryption/Encryption.cpp
        src/Backend/Encryption/Encryption.hpp
        src/Backend/Encryption/Encryption.cpp
        src/Backend/Encryption/Encryption.hpp
        src/Backend/SystemTray/SystemTray.cpp
        src/Backend/SystemTray/SystemTray.hpp
        src/Backend/Downloads/Downloads.cpp
        src/Backend/Downloads/Downloads.hpp
        src/Backend/ZipExtractor/ZipExtractor.cpp
        src/Backend/ZipExtractor/ZipExtractor.hpp
        src/Backend/Installer/Installer.cpp
        src/Backend/Installer/Installer.hpp
        src/Backend/Updater/Updater.cpp
        src/Backend/Updater/Updater.hpp
        src/Backend/TextureQueue/TextureQueue.hpp
        src/Backend/TextureQueue/TextureQueue.cpp
        src/Assets/Fonts/IcontsFontAwesome5.h
        src/Backend/HWID/Hwid.cpp
        src/Backend/HWID/Hwid.hpp
        src/Backend/Image/SvgImage.cpp
        src/Backend/Image/SvgImage.hpp

        # -- Util Source Files --
        src/Util/Easing/Easing.hpp
        src/Util/State/State.hpp
        src/Util/Error/Error.hpp
        src/Util/State/GroupStateManager.hpp
        src/Util/State/RenderGroupData.hpp
        src/Util/GroupUtil/GroupUtil.hpp

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
        src/Frontend/Markdown/Markdown.cpp
        src/Frontend/Markdown/Markdown.hpp
        src/Frontend/Background/Meteors.cpp
        src/Frontend/Background/Meteors.hpp
        src/Frontend/Pages/Betas/Betas.cpp
        src/Frontend/Pages/Betas/Betas.hpp
        src/Backend/Notifications/Notifications.cpp
        src/Backend/Notifications/Notifications.hpp


)

add_dependencies(InfinityLauncher GenerateKeyHeaders)


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
        ${infinity_SOURCE_DIR}/src/imgui/backends/imgui_impl_opengl3.cpp
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
            keys/include
            ${LIBNOTIFY_INCLUDE_DIRS}
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
    set(WIN_LIBS wbemuuid ole32 oleaut32)
else ()
    set(ZOE_LIB zoe::zoe-static ${LIBNOTIFY_LIBRARIES})
endif ()
target_link_libraries(InfinityLauncher PRIVATE tray WebP::webp WebP::webpdecoder PNG::PNG JPEG::JPEG $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static> ${ZOE_LIB} NanoSVG::nanosvg NanoSVG::nanosvgrast WebP::webpdemux GLEW::GLEW OpenGL::GL glfw Boxer CURL::libcurl OpenSSL::SSL OpenSSL::Crypto unofficial::minizip::minizip ZLIB::ZLIB msgpack-cxx ${TOAST_LIB} ${WIN_LIBS})

add_custom_target(InfinityLauncherDist)
add_dependencies(InfinityLauncherDist
        InfinityLauncher
        Updater
)
