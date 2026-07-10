#include <cstdint>
#include <initializer_list>
#include <utility>
#include <variant>
#include <vector>

#include <QtTest>

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/markers/GmmMarkerProposal.hpp"

namespace domain = random_walker::domain;
namespace markers = random_walker::markers;

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

    [[nodiscard]] int marker_count(
        const domain::MarkerProposal& proposal
        , domain::MarkerLabel label
    ) noexcept {
        return static_cast<int>(proposal.mask.count(label));
    }
}

class GmmMarkerProposalTests final : public QObject {
    Q_OBJECT

private slots:
    void rejects_empty_image();
    void proposes_high_confidence_object_and_background_seeds();
    void dark_object_polarity_assigns_low_intensity_to_object();
    void rejects_small_components();
    void distance_transform_removes_boundary_candidates();
    void manual_regions_take_priority_over_automatic_seeds();
};

void GmmMarkerProposalTests::rejects_empty_image() {
    const markers::MarkerProposalOutcome outcome = markers::propose_gmm_markers(
        domain::GrayImage {}
        , permissive_parameters()
    );

    QVERIFY(std::holds_alternative<markers::MarkerProposalError>(outcome));
    QCOMPARE(
        static_cast<int>(std::get<markers::MarkerProposalError>(outcome))
        , static_cast<int>(markers::MarkerProposalError::EmptyImage)
    );
}

void GmmMarkerProposalTests::proposes_high_confidence_object_and_background_seeds() {
    const domain::GrayImage image = make_image(1, 4, {0, 0, 200, 200});

    const markers::MarkerProposalOutcome outcome = markers::propose_gmm_markers(
        image
        , permissive_parameters()
    );

    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    const domain::MarkerProposal& proposal = std::get<domain::MarkerProposal>(outcome);
    QCOMPARE(proposal.mask.seed_count(), std::size_t {4});
    QCOMPARE(marker_count(proposal, domain::MarkerLabel::Background), 2);
    QCOMPARE(marker_count(proposal, domain::MarkerLabel::Object), 2);
    QCOMPARE(proposal.diagnostics.proposed_seed_count, std::size_t {4});
    QCOMPARE(proposal.diagnostics.rejected_low_confidence_count, std::size_t {0});
    QVERIFY(proposal.diagnostics.gmm_iterations > std::size_t {0});
    QCOMPARE(proposal.mask.width(), 4);
    QCOMPARE(proposal.mask.height(), 1);
}

void GmmMarkerProposalTests::dark_object_polarity_assigns_low_intensity_to_object() {
    const domain::GrayImage image = make_image(1, 4, {0, 0, 200, 200});
    domain::AutoMarkerParameters parameters = permissive_parameters();
    parameters.foreground_polarity = domain::ForegroundPolarity::DarkObject;

    const auto outcome = markers::propose_gmm_markers(image, parameters);

    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    const domain::MarkerProposal& proposal = std::get<domain::MarkerProposal>(outcome);
    QCOMPARE(static_cast<int>(proposal.mask.at(0, 0)), static_cast<int>(domain::MarkerLabel::Object));
    QCOMPARE(static_cast<int>(proposal.mask.at(0, 1)), static_cast<int>(domain::MarkerLabel::Object));
    QCOMPARE(static_cast<int>(proposal.mask.at(0, 2)), static_cast<int>(domain::MarkerLabel::Background));
    QCOMPARE(static_cast<int>(proposal.mask.at(0, 3)), static_cast<int>(domain::MarkerLabel::Background));
}

void GmmMarkerProposalTests::rejects_small_components() {
    const domain::GrayImage image = make_image(1, 5, {0, 0, 200, 0, 0});
    domain::AutoMarkerParameters parameters = permissive_parameters();
    parameters.minimum_component_area = 2;

    const auto outcome = markers::propose_gmm_markers(image, parameters);

    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    const domain::MarkerProposal& proposal = std::get<domain::MarkerProposal>(outcome);
    QCOMPARE(marker_count(proposal, domain::MarkerLabel::Object), 0);
    QVERIFY(proposal.diagnostics.rejected_small_component_count >= std::size_t {1});
}

void GmmMarkerProposalTests::distance_transform_removes_boundary_candidates() {
    const domain::GrayImage image = make_image(
        5
        , 5
        , {
            0, 0, 0, 0, 0,
            0, 200, 200, 200, 0,
            0, 200, 200, 200, 0,
            0, 200, 200, 200, 0,
            0, 0, 0, 0, 0
        }
    );
    domain::AutoMarkerParameters parameters = permissive_parameters();
    parameters.minimum_boundary_distance = 2;

    const auto outcome = markers::propose_gmm_markers(image, parameters);

    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    const domain::MarkerProposal& proposal = std::get<domain::MarkerProposal>(outcome);
    QCOMPARE(marker_count(proposal, domain::MarkerLabel::Object), 1);
    QCOMPARE(
        static_cast<int>(proposal.mask.at(2, 2))
        , static_cast<int>(domain::MarkerLabel::Object)
    );
    QVERIFY(proposal.diagnostics.rejected_boundary_distance_count > std::size_t {0});
}

void GmmMarkerProposalTests::manual_regions_take_priority_over_automatic_seeds() {
    const domain::GrayImage image = make_image(1, 4, {0, 0, 200, 200});
    const auto outcome = markers::propose_gmm_markers(image, permissive_parameters());
    QVERIFY(std::holds_alternative<domain::MarkerProposal>(outcome));
    const domain::MarkerProposal proposal = std::get<domain::MarkerProposal>(outcome);
    const std::vector<domain::SeedRegion> manual_regions {
        domain::SeedRegion {
            .area = {.x = 2, .y = 0, .width = 1, .height = 1}
            , .label = domain::SeedLabel::Background
            , .source = domain::SeedSource::User
        }
    };

    const domain::MarkerProposal filtered = markers::remove_manual_conflicts(
        proposal
        , manual_regions
    );

    QCOMPARE(filtered.mask.seed_count(), proposal.mask.seed_count() - 1);
    QCOMPARE(filtered.diagnostics.proposed_seed_count, filtered.mask.seed_count());
    QCOMPARE(filtered.diagnostics.rejected_manual_conflict_count, std::size_t {1});
    QCOMPARE(static_cast<int>(filtered.mask.at(0, 2)), static_cast<int>(domain::MarkerLabel::None));
}

QTEST_GUILESS_MAIN(GmmMarkerProposalTests)
#include "GmmMarkerProposalTests.moc"
