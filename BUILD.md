# random-walker

**random-walker** — десктопное приложение на C++ с использованием **Qt 6.8.1**, **Eigen** и CMake.

---

## Требования

| Компонент                        | Версия / Уточнение                 |
| -------------------------------- | ---------------------------------- |
| [CMake](https://cmake.org/)      | 3.21 или выше                      |
| [Qt](https://www.qt.io/download) | 6.8.1 (локальная установка)        |
| [Eigen](https://eigen.tuxfamily.org/) | 3.4                       |
| Компилятор                       | MSVC с поддержкой C++20            |
| Генератор                        | Ninja                              |

---

## Локальные пути к зависимостям

Локальные пути к Qt 6 и Eigen задаются в `CMakeUserPresets.json`. Этот файл не входит в git, поскольку пути зависят от рабочей станции.

На текущей машине файл уже настроен на Qt 6.8.1 и Eigen 3.4. При переносе проекта укажите фактические каталоги с `Qt6Config.cmake` и `Eigen3Config.cmake`.

## Visual Studio

1. Откройте корневой каталог репозитория через **Open a local folder**.
2. Выберите CMake preset `Debug` или `Release`.
3. Выберите startup item `random-walker.exe`.
4. Выполните **Build All**, затем запустите приложение.

Сборочный preset выполняет цель `deploy`, поэтому необходимые библиотеки и плагины Qt копируются рядом с исполняемым файлом.

## Командная строка

Команды необходимо выполнять из корня репозитория в **Developer PowerShell for Visual Studio**:

```powershell
cmake --workflow --preset debug
.\out\build\debug\bin\random-walker.exe
```

Release:

```powershell
cmake --workflow --preset release
.\out\build\release\bin\random-walker.exe
```
