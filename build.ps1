param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug"
)

$sourceDir = $PSScriptRoot
$buildDir = Join-Path $sourceDir "cmake-build-$($BuildType.ToLower())"

if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir
}

Write-Host "Configuring CMake project in $BuildType mode..."
cmake -DCMAKE_BUILD_TYPE=$BuildType -G "NMake Makefiles" -S $sourceDir -B $buildDir

if ($LASTEXITCODE -eq 0) {
    Write-Host "Building project..."
    cmake --build $buildDir --target InfinityLauncher

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build completed successfully!"
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}