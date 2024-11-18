BUILD_TYPE="${1:-Debug}"
DISPLAY_SERVER="${2:-wayland}"

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ]; then
    echo "Invalid build type. Use Debug or Release"
    exit 1
fi

if [ "$DISPLAY_SERVER" = "x11" ]; then
    DISPLAY_OPTIONS="-DINFINITY_USE_X11=ON -DINFINITY_USE_WAYLAND=OFF"
    echo "Using X11 display server"
elif [ "$DISPLAY_SERVER" = "wayland" ]; then
    DISPLAY_OPTIONS="-DINFINITY_USE_X11=OFF -DINFINITY_USE_WAYLAND=ON"
    echo "Using Wayland display server"
else
    echo "Invalid display server option. Use x11 or wayland"
    exit 1
fi

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SOURCE_DIR}/cmake-build-${BUILD_TYPE,,}"

mkdir -p "${BUILD_DIR}"

echo "Configuring CMake project in $BUILD_TYPE mode..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $DISPLAY_OPTIONS -G "Unix Makefiles" -S "${SOURCE_DIR}" -B "${BUILD_DIR}"

if [ $? -eq 0 ]; then
    echo "Building project..."
    cmake --build "${BUILD_DIR}" --target InfinityLauncher

    if [ $? -eq 0 ]; then
        echo "Build completed successfully!"
    else
        echo "Build failed!"
        exit 1
    fi
else
    echo "CMake configuration failed!"
    exit 1
fi