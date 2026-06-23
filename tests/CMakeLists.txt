qt_add_executable(settings_tests
    settings/SettingsTests.cpp
    ${PROJECT_SOURCE_DIR}/application/settings/SettingsService.cpp
    ${PROJECT_SOURCE_DIR}/infrastructure/settings/QSettingsRepository.cpp
)

target_include_directories(settings_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(settings_tests
    PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME settings_tests COMMAND settings_tests)
