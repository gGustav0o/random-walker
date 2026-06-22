cmake_minimum_required(VERSION 3.21)
project(random-walker LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

qt_add_resources(qml_resources qml.qrc)

set(HEADERS
    app/context/AppContext.hpp
    app/adapter/QImageAdapter.hpp
    app/adapter/MaskRenderer.hpp
    model/domain/Cancellation.hpp
    model/domain/GrayImage.hpp
    model/domain/ProgressReporter.hpp
    model/domain/Seed.hpp
    model/domain/Segmentation.hpp
    model/algorithm/RandomWalkerAlgorithm.hpp
    model/executor/SegmentationExecutor.hpp
    model/executor/JThreadSegmentationExecutor.hpp
    model/graph/PixelGraph.hpp
    model/service/SegmentationService.hpp
    app/qml/qml_names.hpp
    app/service/PresentationImageCache.hpp
    app/service/BaseImageProvider.hpp
    app/service/ResultImageProvider.hpp
    viewmodel/SegmentationViewModel.hpp
    viewmodel/SeedListModel.hpp
)
set(SOURCES
    main.cpp
    app/context/AppContext.cpp
    # model
    model/algorithm/RandomWalkerAlgorithm.cpp
    model/executor/JThreadSegmentationExecutor.cpp
    model/graph/PixelGraph.cpp
    model/service/SegmentationService.cpp
    app/adapter/MaskRenderer.cpp
    app/service/BaseImageProvider.cpp
    app/service/ResultImageProvider.cpp
    viewmodel/SegmentationViewModel.cpp
    viewmodel/SeedListModel.cpp
)

qt_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/app/adapter
        ${CMAKE_CURRENT_SOURCE_DIR}/app/context
        ${CMAKE_CURRENT_SOURCE_DIR}/app/qml
        ${CMAKE_CURRENT_SOURCE_DIR}/app/service
        ${CMAKE_CURRENT_SOURCE_DIR}/model
        ${CMAKE_CURRENT_SOURCE_DIR}/model/algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/model/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/model/executor
        ${CMAKE_CURRENT_SOURCE_DIR}/model/graph
        ${CMAKE_CURRENT_SOURCE_DIR}/model/service
        ${CMAKE_CURRENT_SOURCE_DIR}/viewmodel
)

target_sources(${PROJECT_NAME} PRIVATE ${qml_resources})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Eigen3::Eigen
)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND Qt6::windeployqt
            --no-translations
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/view"
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Deploying the Qt runtime"
        VERBATIM
    )
endif()
