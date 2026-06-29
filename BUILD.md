# random-walker

**random-walker** — десктопное приложение на C++ с использованием **Qt 6.8.1**, **Eigen**, **spdlog** и CMake.

---

## Требования

| Компонент                        | Версия / Уточнение                 |
| -------------------------------- | ---------------------------------- |
| [CMake](https://cmake.org/)      | 3.21 или выше                      |
| [Qt](https://www.qt.io/download) | 6.8.1 (локальная установка)        |
| [Eigen](https://eigen.tuxfamily.org/) | 3.4                       |
| [vcpkg](https://github.com/microsoft/vcpkg) | установлен и доступен через `VCPKG_ROOT` |
| Компилятор                       | MSVC с поддержкой C++20            |
| Генератор                        | Ninja                              |
| OpenMP                           | опционально, через компилятор      |

---

## Управление зависимостями

Qt 6 и Eigen сейчас задаются локальными CMake-путями в `CMakeUserPresets.json`. Этот файл не входит в git, поскольку пути зависят от рабочей станции.

`spdlog` поставляется через `vcpkg` в manifest mode. Manifest находится в `vcpkg.json`, где также зафиксирован `builtin-baseline` для воспроизводимого выбора версий пакетов. Общий `CMakePresets.json` подключает vcpkg toolchain через переменную окружения `VCPKG_ROOT`. При первом configure vcpkg автоматически установит зависимости из manifest-а в локальный build-каталог.

### Установка vcpkg

Один раз установите vcpkg в удобный локальный каталог:

```powershell
git clone https://github.com/microsoft/vcpkg D:\Development\vcpkg
D:\Development\vcpkg\bootstrap-vcpkg.bat
```

Задайте `VCPKG_ROOT` для пользователя и перезапустите Visual Studio, чтобы IDE увидела новую переменную окружения:

```powershell
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'D:\Development\vcpkg', 'User')
```

Для текущей PowerShell-сессии можно дополнительно задать:

```powershell
$env:VCPKG_ROOT = 'D:\Development\vcpkg'
```

### Локальный CMakeUserPresets.json

В локальном `CMakeUserPresets.json` укажите фактические пути к Qt и Eigen. vcpkg toolchain указывать не нужно: `debug-base` и `release-base` уже наследуют общий preset `vcpkg-manifest`.

Пример cache variables для configure preset:

```json
{
  "cacheVariables": {
    "Qt6_DIR": "D:/Development/qt/Qt/6.8.1/msvc2022_64/lib/cmake/Qt6",
    "Eigen3_DIR": "D:/Development/libs/eigen-3.4.0/build"
  }
}
```

Если `spdlog` уже установлен вручную и vcpkg использовать не нужно, создайте отдельный локальный preset, который не наследует `debug-base`/`release-base`, и укажите `spdlog_DIR` так, чтобы `find_package(spdlog CONFIG REQUIRED)` находил пакет. Для проекта рекомендуемый путь — vcpkg manifest.

## Опциональный OpenMP

Параллелизм в алгоритмическом слое управляется CMake-опцией `RANDOM_WALKER_ENABLE_OPENMP`. По умолчанию она выключена, поэтому OpenMP не является обязательным требованием для конфигурации проекта.

Чтобы включить OpenMP, задайте опцию в локальном preset-е или при configure:

```powershell
cmake --preset debug -DRANDOM_WALKER_ENABLE_OPENMP=ON
```

Если опция включена, CMake ищет `OpenMP::OpenMP_CXX` и передает настройку в algorithm-инфраструктуру через `model/algorithm/ParallelPolicy.hpp`. Сейчас OpenMP используется только для детерминированных pixel-wise проходов; sparse-сборка остается последовательной до отдельного архитектурного дизайна.

## Visual Studio

1. Откройте корневой каталог репозитория через **Open a local folder**.
2. Убедитесь, что задана переменная окружения `VCPKG_ROOT`, а локальный `CMakeUserPresets.json` содержит пути к Qt/Eigen.
3. Выберите CMake preset `Debug` или `Release`.
4. Выполните **Project -> Delete Cache and Reconfigure**, если проект уже конфигурировался до подключения vcpkg.
5. Выберите startup item `random-walker.exe`.
6. Выполните **Build All**, затем запустите приложение.

Сборочный preset выполняет цель `deploy`, поэтому необходимые библиотеки и плагины Qt копируются рядом с исполняемым файлом.

## Командная строка

Команды необходимо выполнять из корня репозитория в **Developer PowerShell for Visual Studio**.

Debug:

```powershell
cmake --preset debug
cmake --build --preset debug
.\out\build\debug\bin\random-walker.exe
```

Release:

```powershell
cmake --preset release
cmake --build --preset release
.\out\build\release\bin\random-walker.exe
```

Для configure/build/test workflow целиком:

```powershell
cmake --workflow --preset debug
cmake --workflow --preset release
```
