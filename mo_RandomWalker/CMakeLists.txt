cmake_minimum_required(VERSION 3.21)
project(RandomWalker LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 QUIET)
if (NOT Eigen3_FOUND)
    message(STATUS "Eigen3 не найден. Подключаем через Conan...")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_conan_profile msvc-release)
    else()
        set(_conan_profile msvc-debug)
    endif()

    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
        message(STATUS "Выполняем: conan install с профилем ${_conan_profile}")
        execute_process(
            COMMAND conan install ${CMAKE_SOURCE_DIR}
                    --output-folder=${CMAKE_BINARY_DIR}
                    --build=missing
                    --profile:build=${_conan_profile}
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Ошибка при вызове conan install!")
        endif()
    endif()

    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE _toolchain)
    if(NOT _toolchain)
        message(FATAL_ERROR "conan_toolchain.cmake не найден. Проверьте Conan.")
    endif()

    include(${CMAKE_BINARY_DIR}/conan_deps.cmake REQUIRED)
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

qt_add_resources(qml_resources qml.qrc)

set(HEADERS
    MetaAnnotations.hpp
    global.hpp    
    app/context/AppContext.hpp
    model/algorithm/RandomWalkerAlgorithm.hpp
    model/graph/PixelGraph.hpp
    model/service/SegmentationService.hpp
    model/utils/ImageUtils.hpp
    app/enums/QmlEnums.hpp
    app/qml/qml_names.hpp
    app/qml/qml_registration.hpp
    app/service/BaseImageProvider.hpp
    app/service/EventBus.hpp
    viewmodel/ControlPanelViewModel.hpp
    viewmodel/SettingsPanelViewModel.hpp
    viewmodel/BaseImageViewModel.hpp
    viewmodel/ResultOverlayViewModel.hpp
    viewmodel/ModeSelectionPanelViewModel.hpp
    viewmodel/WithReferenceToolBarViewModel.hpp
    viewmodel/WithoutReferenceToolBarViewModel.hpp
)
set(SOURCES
    main.cpp 
    app/context/AppContext.cpp
    # model
    model/algorithm/RandomWalkerAlgorithm.cpp
    model/graph/PixelGraph.cpp
    model/service/SegmentationService.cpp
    app/service/BaseImageProvider.cpp
    app/service/EventBus.cpp
    viewmodel/ControlPanelViewModel.cpp
    viewmodel/SettingsPanelViewModel.cpp
    viewmodel/BaseImageViewModel.cpp
    viewmodel/ResultOverlayViewModel.cpp
    viewmodel/ModeSelectionPanelViewModel.cpp
    viewmodel/WithReferenceToolBarViewModel.cpp
    viewmodel/WithoutReferenceToolBarViewModel.cpp
)

qt_add_executable(RandomWalker
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(RandomWalker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/app/context
        ${CMAKE_CURRENT_SOURCE_DIR}/app/data
        ${CMAKE_CURRENT_SOURCE_DIR}/app/enums
        ${CMAKE_CURRENT_SOURCE_DIR}/app/qml
        ${CMAKE_CURRENT_SOURCE_DIR}/app/service
        ${CMAKE_CURRENT_SOURCE_DIR}/model
        ${CMAKE_CURRENT_SOURCE_DIR}/model/algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/model/graph
        ${CMAKE_CURRENT_SOURCE_DIR}/model/service
        ${CMAKE_CURRENT_SOURCE_DIR}/model/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/viewmodel
)

target_sources(RandomWalker PRIVATE ${qml_resources})

target_link_libraries(RandomWalker
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Eigen3::Eigen
)
