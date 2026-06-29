function(add_random_walker_test target_name)
    cmake_parse_arguments(TEST_TARGET "" "" "SOURCES;LIBRARIES" ${ARGN})

    qt_add_executable(${target_name}
        ${TEST_TARGET_SOURCES}
    )

    target_include_directories(${target_name}
        PRIVATE
            ${PROJECT_SOURCE_DIR}
    )

    target_link_libraries(${target_name}
        PRIVATE
            Qt6::Core
            Qt6::Test
            ${TEST_TARGET_LIBRARIES}
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

add_random_walker_test(model_domain_tests
    SOURCES
        model/DomainTests.cpp
    LIBRARIES
        Eigen3::Eigen
)

add_random_walker_test(model_algorithm_tests
    SOURCES
        model/AlgorithmTests.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/BoundaryConditions.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/NodePartition.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/PartitionedLaplacian.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/ProbabilityField.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/SeedExpansion.cpp
        ${PROJECT_SOURCE_DIR}/model/graph/GridLaplacian.cpp
    LIBRARIES
        Eigen3::Eigen
)

add_random_walker_test(model_service_tests
    SOURCES
        model/SegmentationServiceTests.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/BoundaryConditions.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/HarmonicSolver.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/NodePartition.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/PartitionedLaplacian.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/ProbabilityField.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/RandomWalkerAlgorithm.cpp
        ${PROJECT_SOURCE_DIR}/model/algorithm/SeedExpansion.cpp
        ${PROJECT_SOURCE_DIR}/model/graph/GridLaplacian.cpp
        ${PROJECT_SOURCE_DIR}/model/service/SegmentationService.cpp
    LIBRARIES
        Eigen3::Eigen
)

add_random_walker_test(application_diagnostics_tests
    SOURCES
        application/DiagnosticsLoggingTests.cpp
        ${PROJECT_SOURCE_DIR}/application/diagnostics/Logging.cpp
)

add_random_walker_test(infrastructure_logging_tests
    SOURCES
        infrastructure/LoggingTests.cpp
        ${PROJECT_SOURCE_DIR}/application/diagnostics/Logging.cpp
        ${PROJECT_SOURCE_DIR}/infrastructure/logging/Logging.cpp
    LIBRARIES
        spdlog::spdlog
)

add_random_walker_test(viewmodel_tests
    SOURCES
        viewmodel/SegmentationViewModelTests.cpp
        ${PROJECT_SOURCE_DIR}/application/diagnostics/Logging.cpp
        ${PROJECT_SOURCE_DIR}/application/settings/SettingsService.cpp
        ${PROJECT_SOURCE_DIR}/presentation/adapter/MaskRenderer.cpp
        ${PROJECT_SOURCE_DIR}/presentation/error/ErrorPresenter.cpp
        ${PROJECT_SOURCE_DIR}/presentation/image/ImageLoader.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/ErrorState.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/ImageState.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/ProgressState.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/ResultState.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/SeedListModel.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/SeedRegionState.cpp
        ${PROJECT_SOURCE_DIR}/viewmodel/SegmentationViewModel.cpp
    LIBRARIES
        Qt6::Gui
        Eigen3::Eigen
)

add_random_walker_test(settings_tests
    SOURCES
        settings/SettingsTests.cpp
        ${PROJECT_SOURCE_DIR}/application/settings/SettingsService.cpp
        ${PROJECT_SOURCE_DIR}/infrastructure/settings/QtSettingsRepository.cpp
    LIBRARIES
        Eigen3::Eigen
)
