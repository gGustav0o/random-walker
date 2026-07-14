cmake_minimum_required(VERSION 3.21)
project(random-walker LANGUAGES CXX)

include(CTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(RANDOM_WALKER_ENABLE_OPENMP "Enable OpenMP in algorithm implementation" OFF)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
find_package(spdlog CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

add_library(random_walker_algorithm_options INTERFACE)
if(RANDOM_WALKER_ENABLE_OPENMP)
    find_package(OpenMP REQUIRED COMPONENTS CXX)
    target_compile_definitions(random_walker_algorithm_options
        INTERFACE
            RANDOM_WALKER_ENABLE_OPENMP=1
    )
    target_link_libraries(random_walker_algorithm_options
        INTERFACE
            OpenMP::OpenMP_CXX
    )
else()
    target_compile_definitions(random_walker_algorithm_options
        INTERFACE
            RANDOM_WALKER_ENABLE_OPENMP=0
    )
endif()

qt_add_resources(qml_resources qml.qrc)

set(HEADERS
    application/error/UserError.hpp
    application/diagnostics/Logging.hpp
    application/markers/AutoMarkerExecutor.hpp
    application/markers/AutoMarkerService.hpp
    application/markers/JThreadAutoMarkerExecutor.hpp
    application/settings/ApplicationSettings.hpp
    application/settings/SettingsRepository.hpp
    application/settings/SettingsService.hpp
    application/segmentation/SegmentationUseCase.hpp
    bootstrap/AppContext.hpp
    infrastructure/settings/QtSettingsRepository.hpp
    infrastructure/logging/Logging.hpp
    model/domain/AutoMarkers.hpp
    model/domain/Cancellation.hpp
    model/domain/GrayImage.hpp
    model/domain/ImageGeometry.hpp
    model/domain/ProgressReporter.hpp
    model/domain/RandomWalkerParameters.hpp
    model/domain/Seed.hpp
    model/domain/Segmentation.hpp
    model/domain/SegmentationConstraints.hpp
    model/domain/SegmentationConstraintsValidation.hpp
    model/domain/SegmentationLimits.hpp
    model/algorithm/BoundaryConditions.hpp
    model/algorithm/DirichletProblemValidation.hpp
    model/algorithm/DistanceTransform.hpp
    model/algorithm/HarmonicSolver.hpp
    model/algorithm/IndexTypes.hpp
    model/algorithm/IterationPolicy.hpp
    model/algorithm/NodePartition.hpp
    model/algorithm/ParallelPolicy.hpp
    model/algorithm/PartitionedLaplacian.hpp
    model/algorithm/ProbabilityField.hpp
    model/algorithm/RandomWalkerAlgorithm.hpp
    model/algorithm/SeedExpansion.hpp
    model/executor/SegmentationExecutor.hpp
    model/executor/JThreadSegmentationExecutor.hpp
    model/graph/EdgeWeight.hpp
    model/graph/GridGeometry.hpp
    model/graph/GridLaplacian.hpp
    model/markers/GmmIntensity.hpp
    model/markers/GmmMarkerProposal.hpp
    model/service/SegmentationService.hpp
    presentation/adapter/AutoMarkerMaskRenderer.hpp
    presentation/adapter/MaskRenderer.hpp
    presentation/adapter/QImageAdapter.hpp
    presentation/image/ImageLoader.hpp
    presentation/error/ErrorPresenter.hpp
    presentation/image/AutoMarkerImageProvider.hpp
    presentation/image/BaseImageProvider.hpp
    presentation/image/PresentationImageCache.hpp
    presentation/image/ResultImageProvider.hpp
    presentation/qml/qml_names.hpp
    viewmodel/AutomaticMarkerState.hpp
    viewmodel/ErrorState.hpp
    viewmodel/ImageState.hpp
    viewmodel/ProgressState.hpp
    viewmodel/ResultState.hpp
    viewmodel/SeedListModel.hpp
    viewmodel/SeedRegionGeometry.hpp
    viewmodel/SeedRegionState.hpp
    viewmodel/SegmentationViewModel.hpp
    viewmodel/ViewModelCallbackGate.hpp
    viewmodel/ViewModelConversions.hpp
)
set(SOURCES
    main.cpp
    bootstrap/AppContext.cpp
    application/diagnostics/Logging.cpp
    application/markers/AutoMarkerService.cpp
    application/markers/JThreadAutoMarkerExecutor.cpp
    application/settings/SettingsService.cpp
    application/segmentation/SegmentationUseCase.cpp
    infrastructure/settings/QtSettingsRepository.cpp
    infrastructure/logging/Logging.cpp
    # model
    model/algorithm/BoundaryConditions.cpp
    model/algorithm/DirichletProblemValidation.cpp
    model/algorithm/HarmonicSolver.cpp
    model/algorithm/NodePartition.cpp
    model/algorithm/PartitionedLaplacian.cpp
    model/algorithm/ProbabilityField.cpp
    model/algorithm/RandomWalkerAlgorithm.cpp
    model/algorithm/SeedExpansion.cpp
    model/domain/SegmentationConstraintsValidation.cpp
    model/executor/JThreadSegmentationExecutor.cpp
    model/graph/GridLaplacian.cpp
    model/service/SegmentationService.cpp
    presentation/adapter/AutoMarkerMaskRenderer.cpp
    presentation/adapter/MaskRenderer.cpp
    presentation/image/ImageLoader.cpp
    presentation/error/ErrorPresenter.cpp
    presentation/image/AutoMarkerImageProvider.cpp
    presentation/image/BaseImageProvider.cpp
    presentation/image/ResultImageProvider.cpp
    viewmodel/AutomaticMarkerState.cpp
    viewmodel/ErrorState.cpp
    viewmodel/ImageState.cpp
    viewmodel/ProgressState.cpp
    viewmodel/ResultState.cpp
    viewmodel/SeedListModel.cpp
    viewmodel/SeedRegionGeometry.cpp
    viewmodel/SeedRegionState.cpp
    viewmodel/SegmentationViewModel.cpp
    viewmodel/ViewModelCallbackGate.cpp
    viewmodel/ViewModelConversions.cpp
)

qt_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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
        spdlog::spdlog
        random_walker_algorithm_options
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

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
