## Building

### Windows

```shell
# Debug build (default)
.\build.ps1
# or explicitly
.\build.ps1 -BuildType Debug

# Release build
.\build.ps1 -BuildType Release
```

### Linux

```shell
# Debug build with Wayland (default)
./build.sh
# or explicitly
./build.sh Debug x11

# Release build with X11
./build.sh Release x11

# Debug build with Wayland
./build.sh Debug wayland

# Release build with Wayland
./build.sh Release wayland
```