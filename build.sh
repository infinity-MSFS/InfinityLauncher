echo "Select build type:"
echo "1. Debug (default)"
echo "2. Release"
read -p "Enter build type (1 or 2): " selection

case "$selection" in
    2) BuildType="Release" ;;
    *) BuildType="Debug" ;;
esac
echo "Selected build type: $BuildType"

echo "Select display server:"
echo "1. X11 (default)"
echo "2. Wayland"
read -p "Enter display server (1 or 2): " displaySelection

case "$displaySelection" in
    2) 
        DISPLAY_OPTIONS="-DINFINITY_USE_X11=OFF -DINFINITY_USE_WAYLAND=ON"
        echo "Using Wayland display server"
        ;;
    *)
        DISPLAY_OPTIONS="-DINFINITY_USE_X11=ON -DINFINITY_USE_WAYLAND=OFF"
        echo "Using X11 display server"
        ;;
esac

SOURCE_DIR="$(pwd)"
BUILD_DIR_DEBUG="$SOURCE_DIR/cmake-build-debug"
BUILD_DIR_RELEASE="$SOURCE_DIR/cmake-build-release"

if [ "$BuildType" = "Debug" ]; then
    BUILD_DIR="$BUILD_DIR_DEBUG"
elif [ "$BuildType" = "Release" ]; then
    BUILD_DIR="$BUILD_DIR_RELEASE"
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

CMAKE_CMD="cmake $DISPLAY_OPTIONS -DCMAKE_BUILD_TYPE=$BuildType -G 'Unix Makefiles' -S \"$SOURCE_DIR\" -B \"$BUILD_DIR\""
echo "Executing command: $CMAKE_CMD"
eval $CMAKE_CMD

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!" >&2
    exit 1
fi

echo "Building project in $BuildType mode..."
cmake --build "$BUILD_DIR" --target InfinityLauncher
if [ $? -ne 0 ]; then
    echo "Build failed!" >&2
    exit 1
fi

echo "Build completed successfully!"

EXE_PATH="$BUILD_DIR/InfinityLauncher"
if [ -f "$EXE_PATH" ]; then
    read -p "Build successful. Would you like to run InfinityLauncher now? (Y/n): " runApp
    if [ "$runApp" != "n" ] && [ "$runApp" != "N" ]; then
        echo "Launching InfinityLauncher..."
        "$EXE_PATH"
    else
        echo "Skipping execution of InfinityLauncher."
    fi
else
    echo "Executable not found at $EXE_PATH. Ensure the target built successfully." >&2
fi
