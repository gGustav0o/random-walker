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
    set_property(GLOBAL APPEND PROPERTY RANDOM_WALKER_TEST_TARGETS ${target_name})
endfunction()

add_random_walker_test(model_domain_tests
    SOURCES
        model/DomainTests.cpp
    LIBRARIES
        random_walker_model
)

add_random_walker_test(model_algorithm_tests
    SOURCES
        model/AlgorithmTests.cpp
    LIBRARIES
        random_walker_model
)

add_random_walker_test(model_distance_transform_tests
    SOURCES
        model/DistanceTransformTests.cpp
    LIBRARIES
        random_walker_model
)

add_random_walker_test(model_gmm_intensity_tests
    SOURCES
        model/GmmIntensityTests.cpp
    LIBRARIES
        random_walker_model
)

add_random_walker_test(model_gmm_marker_proposal_tests
    SOURCES
        model/GmmMarkerProposalTests.cpp
    LIBRARIES
        random_walker_model
)

add_random_walker_test(model_service_tests
    SOURCES
        model/SegmentationServiceTests.cpp
    LIBRARIES
        random_walker_model
)

add_random_walker_test(application_auto_marker_service_tests
    SOURCES
        application/AutoMarkerServiceTests.cpp
    LIBRARIES
        random_walker_application
)

add_random_walker_test(application_diagnostics_tests
    SOURCES
        application/DiagnosticsLoggingTests.cpp
    LIBRARIES
        random_walker_application
)

add_random_walker_test(application_segmentation_use_case_tests
    SOURCES
        application/SegmentationUseCaseTests.cpp
    LIBRARIES
        random_walker_application
)

add_random_walker_test(infrastructure_logging_tests
    SOURCES
        infrastructure/LoggingTests.cpp
    LIBRARIES
        random_walker_infrastructure
)

add_random_walker_test(presentation_auto_marker_mask_renderer_tests
    SOURCES
        presentation/AutoMarkerMaskRendererTests.cpp
    LIBRARIES
        random_walker_presentation
)

add_random_walker_test(viewmodel_tests
    SOURCES
        viewmodel/SegmentationViewModelTests.cpp
    LIBRARIES
        random_walker_viewmodel
)

add_random_walker_test(viewmodel_helper_tests
    SOURCES
        viewmodel/ViewModelHelperTests.cpp
    LIBRARIES
        random_walker_viewmodel
)

add_random_walker_test(settings_tests
    SOURCES
        settings/SettingsTests.cpp
    LIBRARIES
        random_walker_infrastructure
)

get_property(RANDOM_WALKER_TEST_TARGETS GLOBAL PROPERTY RANDOM_WALKER_TEST_TARGETS)
if(RANDOM_WALKER_TEST_TARGETS)
    add_custom_target(random_walker_tests
        DEPENDS ${RANDOM_WALKER_TEST_TARGETS}
    )
    add_dependencies(${PROJECT_NAME} random_walker_tests)
endif()
