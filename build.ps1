$BuildType = $null

Write-Host "Select build type:"
Write-Host "1. Debug (defauly)"
Write-Host "2. Release"
$selection = Read-Host "Enter build type (1 or 2)"

switch ($selection) {
    "2" { $BuildType = "Release" }
    default { $BuildType = "Debug" }
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) {
    Write-Host "Visual Studio installation not found!" -ForegroundColor Red
    exit 1
}

$devShellModule = Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
if (-not (Test-Path $devShellModule)) {
    Write-Host "Developer PowerShell module not found!" -ForegroundColor Red
    exit 1
}

Import-Module $devShellModule
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64"

$sourceDir = $PSScriptRoot
$buildDirDebug = Join-Path $sourceDir "cmake-build-debug"
$buildDirRelease = Join-Path $sourceDir "cmake-build-release"

$cmakePath = "C:\Users\Taco\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe"

if ($BuildType -eq "Debug") {
    if (!(Test-Path $buildDirDebug)) {
        New-Item -ItemType Directory -Path $buildDirDebug
    }
    $cmakeCmd = "$cmakePath -DCMAKE_BUILD_TYPE=Debug -G `"NMake Makefiles`" -S `"$sourceDir`" -B `"$buildDirDebug`""
}
elseif ($BuildType -eq "Release") {
    if (!(Test-Path $buildDirRelease)) {
        New-Item -ItemType Directory -Path $buildDirRelease
    }
    $cmakeCmd = "$cmakePath -DCMAKE_BUILD_TYPE=Release -G `"NMake Makefiles`" -S `"$sourceDir`" -B `"$buildDirRelease`""
}

Write-Host "Executing command: $cmakeCmd"

Write-Host "Configuring CMake project in $BuildType mode..."
Invoke-Expression $cmakeCmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "Building project..."
    $buildDir = if ($BuildType -eq "Debug") { $buildDirDebug } else { $buildDirRelease }
    cmake --build $buildDir --target InfinityLauncher

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build completed successfully!"

        $exePath = Join-Path $buildDir "InfinityLauncher.exe"
        if (Test-Path $exePath) {
            $runApp = Read-Host "Build successful. Would you like to run InfinityLauncher.exe now? (Y/n)"
            if ($runApp.ToLower() -ne "n") {
                Write-Host "Launching InfinityLauncher..."
                Start-Process -FilePath $exePath
            }
            else {
                Write-Host "Skipping execution of InfinityLauncher."
            }
        }
        else {
            Write-Host "Executable not found at $exePath. Ensure the target built successfully." -ForegroundColor Red
        }
    }
    else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}
else {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}
