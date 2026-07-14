# Build Instructions

`random-walker` is a Windows desktop application built with C++20, Qt 6,
Eigen, spdlog, and CMake.

The checked-in CMake presets define `debug` and `release` configurations. Local
machine-specific paths are expected to live in `CMakeUserPresets.json`, which is
ignored by git.

## Requirements

| Component | Requirement |
| --- | --- |
| CMake | 3.21 or newer |
| Compiler | MSVC with C++20 support |
| Generator | Ninja |
| Qt | Qt 6.8.1, local installation |
| Eigen | 3.4, local installation |
| vcpkg | Installed locally and available through `VCPKG_ROOT` |
| spdlog | Installed by vcpkg manifest mode |
| OpenMP | Optional |

The current presets are Windows-only. Command-line builds should be run from a
Visual Studio developer shell so that MSVC include and library paths are
initialized.

## Dependency Setup

Qt and Eigen are discovered through local CMake paths. Put those paths in your
local `CMakeUserPresets.json`.

`spdlog` is provided through vcpkg manifest mode. The project manifest is
`vcpkg.json`, and `CMakePresets.json` points CMake to the vcpkg toolchain via
`VCPKG_ROOT`:

```json
"CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
```

On the first configure, vcpkg installs the manifest dependencies into the local
build tree.

## Install vcpkg

Install vcpkg once in a local directory:

```powershell
git clone https://github.com/microsoft/vcpkg D:\Development\vcpkg
D:\Development\vcpkg\bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT` for your user account and restart Visual Studio or your
terminal:

```powershell
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'D:\Development\vcpkg', 'User')
```

For the current PowerShell session only:

```powershell
$env:VCPKG_ROOT = 'D:\Development\vcpkg'
```

## Local CMakeUserPresets.json

Create `CMakeUserPresets.json` in the repository root. It should define the
machine-specific paths for Qt, Eigen, and optionally Ninja. Do not commit this
file.

Minimal example:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local-dependencies",
      "hidden": true,
      "cacheVariables": {
        "Qt6_DIR": "D:/Development/qt/Qt/6.8.1/msvc2022_64/lib/cmake/Qt6",
        "Eigen3_DIR": "D:/Development/libs/eigen-3.4.0/build"
      }
    },
    {
      "name": "debug",
      "displayName": "Debug",
      "inherits": [
        "local-dependencies",
        "debug-base"
      ],
      "architecture": {
        "value": "x64",
        "strategy": "external"
      }
    },
    {
      "name": "release",
      "displayName": "Release",
      "inherits": [
        "local-dependencies",
        "release-base"
      ],
      "architecture": {
        "value": "x64",
        "strategy": "external"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "debug",
      "configurePreset": "debug",
      "targets": ["random-walker"]
    },
    {
      "name": "release",
      "configurePreset": "release",
      "targets": ["random-walker"]
    }
  ],
  "testPresets": [
    {
      "name": "debug",
      "configurePreset": "debug",
      "output": {
        "outputOnFailure": true
      }
    },
    {
      "name": "release",
      "configurePreset": "release",
      "output": {
        "outputOnFailure": true
      }
    }
  ],
  "workflowPresets": [
    {
      "name": "debug",
      "steps": [
        { "type": "configure", "name": "debug" },
        { "type": "build", "name": "debug" },
        { "type": "test", "name": "debug" }
      ]
    },
    {
      "name": "release",
      "steps": [
        { "type": "configure", "name": "release" },
        { "type": "build", "name": "release" },
        { "type": "test", "name": "release" }
      ]
    }
  ]
}
```

If Ninja is not on `PATH`, add `CMAKE_MAKE_PROGRAM` to `local-dependencies`.
If you do not want to use `VCPKG_ROOT`, you may also set
`CMAKE_TOOLCHAIN_FILE` there explicitly, but the recommended setup is to keep
the shared preset and provide `VCPKG_ROOT`.

## Optional OpenMP

OpenMP is controlled by the CMake option `RANDOM_WALKER_ENABLE_OPENMP`. It is
off by default in the shared presets.

Enable it in your local preset:

```json
"RANDOM_WALKER_ENABLE_OPENMP": true
```

or pass it during configure:

```powershell
cmake --preset debug -DRANDOM_WALKER_ENABLE_OPENMP=ON
```

When enabled, CMake links `OpenMP::OpenMP_CXX` and defines
`RANDOM_WALKER_ENABLE_OPENMP=1` for the algorithm layer.

## Visual Studio

1. Open the repository root with **Open a local folder**.
2. Ensure `VCPKG_ROOT` is set and `CMakeUserPresets.json` contains valid
   Qt/Eigen paths.
3. Select the `Debug` or `Release` CMake preset.
4. If the project was configured before dependency paths changed, run
   **Project -> Delete Cache and Reconfigure**.
5. Select `random-walker.exe` as the startup item.
6. Build and run the application.

On Windows, `windeployqt` runs as a post-build step for `random-walker.exe`.
It copies the required Qt runtime libraries and QML plugins next to the
executable.

## Command Line

Run these commands from the repository root in **Developer PowerShell for Visual
Studio** or **Developer Command Prompt for Visual Studio**.

Debug:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
.\out\build\debug\bin\random-walker.exe
```

Release:

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release
.\out\build\release\bin\random-walker.exe
```

Full configure/build/test workflow:

```powershell
cmake --workflow --preset debug
cmake --workflow --preset release
```

## Notes

- `cmake --build --preset <name>` builds the `random-walker` target.
- With `BUILD_TESTING` enabled, the application target depends on the test
  executables, so they are compiled during the normal build. Run `ctest`
  separately to execute them.
- `windeployqt` is attached to the application as a post-build command. 
