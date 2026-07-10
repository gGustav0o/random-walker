#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <QtTest>

#include "application/diagnostics/Logging.hpp"
#include "application/markers/AutoMarkerService.hpp"
#include "model/domain/GrayImage.hpp"

namespace application = random_walker::application;
namespace domain = random_walker::domain;

namespace {
    [[nodiscard]] domain::GrayImage make_image(
        int height
        , int width
        , std::initializer_list<std::uint8_t> values
    ) {
        domain::GrayImageMatrix pixels(height, width);
        auto value = values.begin();
        for (int row = 0; row < height; ++row) {
            for (int column = 0; column < width; ++column) {
                pixels(row, column) = *value;
                ++value;
            }
        }
        return domain::GrayImage(std::move(pixels));
    }

    [[nodiscard]] domain::AutoMarkerParameters permissive_parameters() noexcept {
        return {
            .gmm = {
                .max_iterations = 100
                , .convergence_tolerance = 1e-10
                , .minimum_variance = 1.0
            }
            , .confidence_threshold = 0.95
            , .minimum_component_area = 1
            , .minimum_boundary_distance = 0
            , .foreground_polarity = domain::ForegroundPolarity::BrightObject
        };
    }

    struct CapturedLogMessage {
        application::LogLevel level = application::LogLevel::Info;
        std::string category;
        std::string message;
    };
}

class AutoMarkerServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void maps_empty_image_to_user_error();
    void maps_invalid_parameters_to_user_error();
    void filters_automatic_markers_that_conflict_with_manual_regions();
    void logs_boundary_distance_diagnostics();
};

void AutoMarkerServiceTests::cleanup() {
    application::clear_log_sink();
}

void AutoMarkerServiceTests::maps_empty_image_to_user_error() {
    const application::AutoMarkerService service;

    const application::AutoMarkerProposalOutcome outcome = service.propose(
        domain::GrayImage {}
        , permissive_parameters()
        , {}
    );

    QVERIFY(std::holds_alternative<application::AutoMarkerError>(outcome));
    QCOMPARE(
        static_cast<int>(std::get<application::AutoMarkerError>(outcome))
        , static_cast<int>(application::AutoMarkerError::EmptyImage)
    );
}

void AutoMarkerServiceTests::maps_invalid_parameters_to_user_error() {
    const application::AutoMarkerService service;
    domain::AutoMarkerParameters parameters = permissive_parameters();
    parameters.confidence_threshold = 2.0;

    const application::AutoMarkerProposalOutcome outcome = service.propose(
        make_image(1, 4, {0, 0, 200, 200})
        , parameters
        , {}
    );

    QVERIFY(std::holds_alternative<application::AutoMarkerError>(outcome));
    QCOMPARE(
        static_cast<int>(std::get<application::AutoMarkerError>(outcome))
        , static_cast<int>(application::AutoMarkerError::InvalidParameters)
    );
}

void AutoMarkerServiceTests::filters_automatic_markers_that_conflict_with_manual_regions() {
    const application::AutoMarkerService service;
    const std::vector<domain::SeedRegion> manual_regions {
        domain::SeedRegion {
            .area = {.x = 2, .y = 0, .width = 1, .height = 1}
            , .label = domain::SeedLabel::Background
            , .source = domain::SeedSource::User
        }
    };

    const application::AutoMarkerProposalOutcome outcome = service.propose(
        make_image(1, 4, {0, 0, 200, 200})
        , permissive_parameters()
        , manual_regions
    );

    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    const domain::MarkerProposal& proposal = std::get<domain::MarkerProposal>(outcome);
    QCOMPARE(proposal.mask.seed_count(), std::size_t {3});
    QCOMPARE(proposal.diagnostics.proposed_seed_count, proposal.mask.seed_count());
    QCOMPARE(proposal.diagnostics.rejected_manual_conflict_count, std::size_t {1});
}

void AutoMarkerServiceTests::logs_boundary_distance_diagnostics() {
    std::vector<CapturedLogMessage> messages;
    application::set_log_sink(
        [&messages](
            application::LogLevel level
            , std::string_view category
            , std::string_view message
        ) {
            messages.push_back({
                .level = level
                , .category = std::string(category)
                , .message = std::string(message)
            });
        }
    );

    const application::AutoMarkerService service;
    domain::AutoMarkerParameters parameters = permissive_parameters();
    parameters.minimum_boundary_distance = 2;

    const application::AutoMarkerProposalOutcome outcome = service.propose(
        make_image(
            5
            , 5
            , {
                0, 0, 0, 0, 0,
                0, 200, 200, 200, 0,
                0, 200, 200, 200, 0,
                0, 200, 200, 200, 0,
                0, 0, 0, 0, 0
            }
        )
        , parameters
        , {}
    );

    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    QCOMPARE(messages.size(), std::size_t {2});
    QCOMPARE(
        static_cast<int>(messages[0].level)
        , static_cast<int>(application::LogLevel::Info)
    );
    QVERIFY(messages[0].category == std::string(application::log_category::auto_markers));
    QVERIFY(messages[0].message.find("minimum_boundary_distance=2")
        != std::string::npos);
    QCOMPARE(
        static_cast<int>(messages[1].level)
        , static_cast<int>(application::LogLevel::Info)
    );
    QVERIFY(messages[1].category == std::string(application::log_category::auto_markers));
    QVERIFY(messages[1].message.find("rejected_boundary_distance=")
        != std::string::npos);
}

QTEST_GUILESS_MAIN(AutoMarkerServiceTests)
#include "AutoMarkerServiceTests.moc"
