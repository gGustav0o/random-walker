#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <stop_token>
#include <utility>
#include <variant>
#include <vector>

#include <Eigen/Sparse>
#include <QtTest>

#include "model/algorithm/BoundaryConditions.hpp"
#include "model/algorithm/NodePartition.hpp"
#include "model/algorithm/PartitionedLaplacian.hpp"
#include "model/algorithm/ParallelPolicy.hpp"
#include "model/algorithm/ProbabilityField.hpp"
#include "model/algorithm/SeedExpansion.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/Segmentation.hpp"
#include "model/graph/EdgeWeight.hpp"
#include "model/graph/GridLaplacian.hpp"

namespace algorithm = random_walker::algorithm;
namespace domain = random_walker::domain;
namespace graph = random_walker::graph;

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

    [[nodiscard]] domain::CancellationToken cancelled_token() {
        std::stop_source source;
        source.request_stop();
        return domain::CancellationToken(source.get_token());
    }

    [[nodiscard]] bool is_cancelled(const auto& outcome) {
        return std::holds_alternative<domain::Cancelled>(outcome);
    }

    [[nodiscard]] double coefficient(
        const Eigen::SparseMatrix<double>& matrix
        , int row
        , int column
    ) {
        return matrix.coeff(row, column);
    }

    [[nodiscard]] std::size_t large_parallel_pixel_count() noexcept {
        return algorithm::kParallelWorkItemThreshold;
    }

    [[nodiscard]] int large_parallel_width() noexcept {
        return 256;
    }

    [[nodiscard]] int large_parallel_height() noexcept {
        constexpr std::size_t width = 256;
        return static_cast<int>(
            (large_parallel_pixel_count() + width - 1) / width
        );
    }

    [[nodiscard]] domain::RandomWalkerParameters random_walker_parameters(
        double beta
        , domain::PixelConnectivity connectivity
        , double distance_power = domain::kDefaultRandomWalkerDistancePower
    ) noexcept {
        return {
            .beta = beta
            , .distance_power = distance_power
            , .connectivity = connectivity
        };
    }

    [[nodiscard]] graph::EdgeWeightInput edge_weight_input(
        std::uint8_t first_intensity
        , std::uint8_t second_intensity
        , double distance
    ) noexcept {
        return {
            .first_intensity = first_intensity
            , .second_intensity = second_intensity
            , .first_position = {.row = 0, .column = 0}
            , .second_position = {.row = 0, .column = 1}
            , .distance = distance
        };
    }

    [[nodiscard]] double probability_value_for_index(int index) noexcept {
        return static_cast<double>((index * 37) % 101) / 100.0;
    }

    [[nodiscard]] double normalized_beta_from_raw_beta(
        double raw_beta
    ) noexcept {
        return domain::normalized_beta_from_legacy_raw_beta(raw_beta);
    }

    [[nodiscard]] algorithm::BoundaryConditions empty_boundary_conditions() {
        return algorithm::BoundaryConditions {};
    }

    [[nodiscard]] algorithm::NodePartition all_unknown_partition(
        int pixel_count
    ) {
        algorithm::NodePartition partition;
        partition.unknown_pixels.reserve(static_cast<std::size_t>(pixel_count));
        partition.unknown_index_by_pixel.reserve(
            static_cast<std::size_t>(pixel_count)
        );

        for (int index = 0; index < pixel_count; ++index) {
            const algorithm::PixelIndex pixel_index {.value = index};
            partition.unknown_pixels.push_back(pixel_index);
            partition.unknown_index_by_pixel.emplace(
                pixel_index
                , algorithm::UnknownIndex {.value = index}
            );
        }

        return partition;
    }

    [[nodiscard]] Eigen::VectorXd large_unknown_values(int pixel_count) {
        Eigen::VectorXd values(pixel_count);
        for (int index = 0; index < pixel_count; ++index) {
            values[index] = probability_value_for_index(index);
        }
        return values;
    }

    [[nodiscard]] domain::ProbabilityMap large_probability_map() {
        const int width = large_parallel_width();
        const int height = large_parallel_height();
        domain::ProbabilityMap probabilities(height, width);

        for (int index = 0; index < width * height; ++index) {
            probabilities(index / width, index % width) =
                probability_value_for_index(index);
        }

        return probabilities;
    }

    struct ProgressLog {
        std::vector<domain::SegmentationProgress> entries;

        [[nodiscard]] domain::ProgressReporter reporter() {
            return domain::ProgressReporter {
                7
                , [this](domain::SegmentationProgress progress) {
                    entries.push_back(std::move(progress));
                }
            };
        }
    };

    void verify_pixel_wise_progress_contract(
        const ProgressLog& progress_log
        , domain::SegmentationStage stage
    ) {
        QVERIFY(progress_log.entries.size() >= std::size_t {2});
        QCOMPARE(progress_log.entries.front().stage, stage);
        QVERIFY(progress_log.entries.front().fraction.has_value());
        QCOMPARE(*progress_log.entries.front().fraction, 0.0);
        QCOMPARE(progress_log.entries.back().stage, stage);
        QVERIFY(progress_log.entries.back().fraction.has_value());
        QCOMPARE(*progress_log.entries.back().fraction, 1.0);

        if constexpr (algorithm::kOpenMpEnabled) {
            QCOMPARE(progress_log.entries.size(), std::size_t {2});
        }
    }

    [[nodiscard]] algorithm::BoundaryConditions boundary_conditions_for_2x2() {
        algorithm::BoundaryConditions conditions;
        conditions.pixels = {
            algorithm::PixelIndex {.value = 0},
            algorithm::PixelIndex {.value = 3}
        };
        conditions.value_by_pixel.emplace(
            algorithm::PixelIndex {.value = 0}
            , 0.0
        );
        conditions.value_by_pixel.emplace(
            algorithm::PixelIndex {.value = 3}
            , 1.0
        );
        return conditions;
    }
}

class AlgorithmTests final : public QObject {
    Q_OBJECT

private slots:
    void parallel_policy_respects_compile_time_flag_and_threshold();
    void expands_seed_regions_in_row_major_order();
    void seed_expansion_honors_cancellation();
    void builds_boundary_conditions_with_expected_values();
    void rejects_conflicting_boundary_seed_labels();
    void deduplicates_matching_boundary_seed_labels();
    void boundary_value_vector_follows_boundary_pixel_order();
    void partitions_boundary_and_unknown_pixels();
    void partitions_laplacian_into_unknown_blocks();
    void assembles_probability_map_from_boundary_and_unknown_values();
    void assembles_large_probability_map_deterministically();
    void large_probability_assembly_honors_cancellation();
    void thresholds_probabilities_at_half();
    void thresholds_large_probability_map_deterministically();
    void large_thresholding_honors_cancellation();
    void global_beta_edge_weight_parameters_validate_domain_bounds();
    void edge_weight_zero_distance_power_matches_baseline();
    void edge_weight_keeps_orthogonal_edges_independent_of_distance_power();
    void edge_weight_uses_distance_power();
    void edge_weight_function_object_matches_pure_formula();
    void grid_laplacian_has_expected_2x2_structure();
    void grid_laplacian_eight_connectivity_adds_diagonal_edges();
    void grid_laplacian_distance_power_normalizes_diagonal_edges();
    void grid_laplacian_beta_changes_edge_weight();
};

void AlgorithmTests::parallel_policy_respects_compile_time_flag_and_threshold() {
#if RANDOM_WALKER_ENABLE_OPENMP
    constexpr bool openmp_enabled = true;
#else
    constexpr bool openmp_enabled = false;
#endif

    QCOMPARE(algorithm::kOpenMpEnabled, openmp_enabled);
    QVERIFY(!algorithm::should_run_parallel(0));
    QVERIFY(!algorithm::should_run_parallel(
        algorithm::kParallelWorkItemThreshold - 1
    ));
    QCOMPARE(
        algorithm::should_run_parallel(algorithm::kParallelWorkItemThreshold)
        , openmp_enabled
    );
}

void AlgorithmTests::expands_seed_regions_in_row_major_order() {
    const std::vector<domain::SeedRegion> regions {
        domain::SeedRegion {
            .area = {.x = 1, .y = 2, .width = 2, .height = 2},
            .label = domain::SeedLabel::Object
            , .source = domain::SeedSource::Automatic
        },
        domain::SeedRegion {
            .area = {.x = 0, .y = 0, .width = 1, .height = 1},
            .label = domain::SeedLabel::Background
        }
    };

    const auto outcome = algorithm::expand_seed_regions(
        regions
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<std::vector<domain::Seed>>(outcome));
    const auto& seeds = std::get<std::vector<domain::Seed>>(outcome);
    QCOMPARE(seeds.size(), std::size_t {5});
    QCOMPARE(seeds[0].position.x, 1);
    QCOMPARE(seeds[0].position.y, 2);
    QCOMPARE(seeds[1].position.x, 2);
    QCOMPARE(seeds[1].position.y, 2);
    QCOMPARE(seeds[2].position.x, 1);
    QCOMPARE(seeds[2].position.y, 3);
    QCOMPARE(seeds[3].position.x, 2);
    QCOMPARE(seeds[3].position.y, 3);
    QCOMPARE(static_cast<int>(seeds[0].label), static_cast<int>(domain::SeedLabel::Object));
    QCOMPARE(static_cast<int>(seeds[0].source), static_cast<int>(domain::SeedSource::Automatic));
    QCOMPARE(seeds[0].confidence, 1.0);
    QCOMPARE(static_cast<int>(seeds[4].label), static_cast<int>(domain::SeedLabel::Background));
    QCOMPARE(static_cast<int>(seeds[4].source), static_cast<int>(domain::SeedSource::User));
}

void AlgorithmTests::seed_expansion_honors_cancellation() {
    const std::vector<domain::SeedRegion> regions {
        domain::SeedRegion {
            .area = {.x = 0, .y = 0, .width = 1, .height = 1},
            .label = domain::SeedLabel::Object
        }
    };

    const auto outcome = algorithm::expand_seed_regions(
        regions
        , cancelled_token()
        , domain::ProgressReporter {}
    );

    QVERIFY(is_cancelled(outcome));
}

void AlgorithmTests::builds_boundary_conditions_with_expected_values() {
    const domain::GrayImage image = make_image(2, 2, {10, 20, 30, 40});
    const std::vector<domain::Seed> seeds {
        domain::Seed {
            .position = {.x = 1, .y = 0},
            .label = domain::SeedLabel::Object
        },
        domain::Seed {
            .position = {.x = 0, .y = 1},
            .label = domain::SeedLabel::Background
        }
    };
    const domain::SegmentationInput input {.image = image, .seeds = seeds};

    const auto outcome = algorithm::build_boundary_conditions(
        input
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<algorithm::BoundaryConditions>(outcome));
    const auto& conditions = std::get<algorithm::BoundaryConditions>(outcome);
    QVERIFY(conditions.contains(algorithm::PixelIndex {.value = 1}));
    QVERIFY(conditions.contains(algorithm::PixelIndex {.value = 2}));
    QCOMPARE(conditions.value_at(algorithm::PixelIndex {.value = 1}), 1.0);
    QCOMPARE(conditions.value_at(algorithm::PixelIndex {.value = 2}), 0.0);
}

void AlgorithmTests::rejects_conflicting_boundary_seed_labels() {
    const domain::GrayImage image = make_image(1, 1, {10});
    const std::vector<domain::Seed> seeds {
        domain::Seed {
            .position = {.x = 0, .y = 0},
            .label = domain::SeedLabel::Background
        },
        domain::Seed {
            .position = {.x = 0, .y = 0},
            .label = domain::SeedLabel::Object
        }
    };
    const domain::SegmentationInput input {.image = image, .seeds = seeds};

    const auto outcome = algorithm::build_boundary_conditions(
        input
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<domain::SegmentationError>(outcome));
    QCOMPARE(
        static_cast<int>(std::get<domain::SegmentationError>(outcome))
        , static_cast<int>(domain::SegmentationError::ConflictingSeedLabels)
    );
}

void AlgorithmTests::deduplicates_matching_boundary_seed_labels() {
    const domain::GrayImage image = make_image(1, 1, {10});
    const std::vector<domain::Seed> seeds {
        domain::Seed {
            .position = {.x = 0, .y = 0},
            .label = domain::SeedLabel::Background
        },
        domain::Seed {
            .position = {.x = 0, .y = 0},
            .label = domain::SeedLabel::Background
        }
    };
    const domain::SegmentationInput input {.image = image, .seeds = seeds};

    const auto outcome = algorithm::build_boundary_conditions(
        input
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<algorithm::BoundaryConditions>(outcome));
    const auto& conditions = std::get<algorithm::BoundaryConditions>(outcome);
    QCOMPARE(conditions.pixels.size(), std::size_t {1});
    QVERIFY(conditions.contains(algorithm::PixelIndex {.value = 0}));
    QCOMPARE(conditions.value_at(algorithm::PixelIndex {.value = 0}), 0.0);
}

void AlgorithmTests::boundary_value_vector_follows_boundary_pixel_order() {
    const algorithm::BoundaryConditions conditions = boundary_conditions_for_2x2();
    const std::vector<algorithm::PixelIndex> boundary_pixels {
        algorithm::PixelIndex {.value = 3},
        algorithm::PixelIndex {.value = 0}
    };

    const auto outcome = algorithm::build_boundary_value_vector(
        conditions
        , boundary_pixels
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<Eigen::VectorXd>(outcome));
    const Eigen::VectorXd& values = std::get<Eigen::VectorXd>(outcome);
    QCOMPARE(static_cast<int>(values.size()), 2);
    QCOMPARE(values[0], 1.0);
    QCOMPARE(values[1], 0.0);
}

void AlgorithmTests::partitions_boundary_and_unknown_pixels() {
    const algorithm::BoundaryConditions conditions = boundary_conditions_for_2x2();

    const auto outcome = algorithm::partition_nodes(
        4
        , conditions
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<algorithm::NodePartition>(outcome));
    const auto& partition = std::get<algorithm::NodePartition>(outcome);
    QCOMPARE(partition.boundary_pixels.size(), std::size_t {2});
    QCOMPARE(partition.unknown_pixels.size(), std::size_t {2});
    QCOMPARE(partition.unknown_pixels[0].value, 1);
    QCOMPARE(partition.unknown_pixels[1].value, 2);
    QCOMPARE(partition.unknown_index_by_pixel.at(algorithm::PixelIndex {.value = 1}).value, 0);
    QCOMPARE(partition.unknown_index_by_pixel.at(algorithm::PixelIndex {.value = 2}).value, 1);
    QCOMPARE(partition.boundary_index_by_pixel.at(algorithm::PixelIndex {.value = 0}).value, 0);
    QCOMPARE(partition.boundary_index_by_pixel.at(algorithm::PixelIndex {.value = 3}).value, 1);
}

void AlgorithmTests::partitions_laplacian_into_unknown_blocks() {
    algorithm::NodePartition partition;
    partition.unknown_pixels = {
        algorithm::PixelIndex {.value = 1},
        algorithm::PixelIndex {.value = 2}
    };
    partition.boundary_pixels = {
        algorithm::PixelIndex {.value = 0},
        algorithm::PixelIndex {.value = 3}
    };
    partition.unknown_index_by_pixel.emplace(algorithm::PixelIndex {.value = 1}, algorithm::UnknownIndex {.value = 0});
    partition.unknown_index_by_pixel.emplace(algorithm::PixelIndex {.value = 2}, algorithm::UnknownIndex {.value = 1});
    partition.boundary_index_by_pixel.emplace(algorithm::PixelIndex {.value = 0}, algorithm::BoundaryIndex {.value = 0});
    partition.boundary_index_by_pixel.emplace(algorithm::PixelIndex {.value = 3}, algorithm::BoundaryIndex {.value = 1});

    Eigen::SparseMatrix<double> laplacian(4, 4);
    const std::vector<Eigen::Triplet<double>> triplets {
        {0, 0, 2.0}, {0, 1, -1.0}, {0, 2, -1.0},
        {1, 0, -1.0}, {1, 1, 2.0}, {1, 3, -1.0},
        {2, 0, -1.0}, {2, 2, 2.0}, {2, 3, -1.0},
        {3, 1, -1.0}, {3, 2, -1.0}, {3, 3, 2.0}
    };
    laplacian.setFromTriplets(triplets.begin(), triplets.end());

    const auto outcome = algorithm::partition_laplacian(
        laplacian
        , partition
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<algorithm::PartitionedLaplacian>(outcome));
    const auto& blocks = std::get<algorithm::PartitionedLaplacian>(outcome);
    QCOMPARE(static_cast<int>(blocks.unknown_unknown_block.rows()), 2);
    QCOMPARE(static_cast<int>(blocks.unknown_unknown_block.cols()), 2);
    QCOMPARE(static_cast<int>(blocks.unknown_boundary_block.rows()), 2);
    QCOMPARE(static_cast<int>(blocks.unknown_boundary_block.cols()), 2);
    QCOMPARE(coefficient(blocks.unknown_unknown_block, 0, 0), 2.0);
    QCOMPARE(coefficient(blocks.unknown_unknown_block, 0, 1), 0.0);
    QCOMPARE(coefficient(blocks.unknown_unknown_block, 1, 0), 0.0);
    QCOMPARE(coefficient(blocks.unknown_unknown_block, 1, 1), 2.0);
    QCOMPARE(coefficient(blocks.unknown_boundary_block, 0, 0), -1.0);
    QCOMPARE(coefficient(blocks.unknown_boundary_block, 0, 1), -1.0);
    QCOMPARE(coefficient(blocks.unknown_boundary_block, 1, 0), -1.0);
    QCOMPARE(coefficient(blocks.unknown_boundary_block, 1, 1), -1.0);
}

void AlgorithmTests::assembles_probability_map_from_boundary_and_unknown_values() {
    const algorithm::BoundaryConditions conditions = boundary_conditions_for_2x2();
    algorithm::NodePartition partition;
    partition.unknown_pixels = {
        algorithm::PixelIndex {.value = 1},
        algorithm::PixelIndex {.value = 2}
    };
    partition.boundary_pixels = {
        algorithm::PixelIndex {.value = 0},
        algorithm::PixelIndex {.value = 3}
    };
    partition.unknown_index_by_pixel.emplace(algorithm::PixelIndex {.value = 1}, algorithm::UnknownIndex {.value = 0});
    partition.unknown_index_by_pixel.emplace(algorithm::PixelIndex {.value = 2}, algorithm::UnknownIndex {.value = 1});
    partition.boundary_index_by_pixel.emplace(algorithm::PixelIndex {.value = 0}, algorithm::BoundaryIndex {.value = 0});
    partition.boundary_index_by_pixel.emplace(algorithm::PixelIndex {.value = 3}, algorithm::BoundaryIndex {.value = 1});

    Eigen::VectorXd unknown_values(2);
    unknown_values << 0.25, 0.75;

    const auto outcome = algorithm::assemble_probability_map(
        conditions
        , partition
        , unknown_values
        , 2
        , 2
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<domain::ProbabilityMap>(outcome));
    const domain::ProbabilityMap& probabilities =
        std::get<domain::ProbabilityMap>(outcome);
    QCOMPARE(static_cast<int>(probabilities.rows()), 2);
    QCOMPARE(static_cast<int>(probabilities.cols()), 2);
    QCOMPARE(probabilities(0, 0), 0.0);
    QCOMPARE(probabilities(0, 1), 0.25);
    QCOMPARE(probabilities(1, 0), 0.75);
    QCOMPARE(probabilities(1, 1), 1.0);
}


void AlgorithmTests::assembles_large_probability_map_deterministically() {
    const int width = large_parallel_width();
    const int height = large_parallel_height();
    const int pixel_count = width * height;
    const algorithm::BoundaryConditions conditions = empty_boundary_conditions();
    const algorithm::NodePartition partition = all_unknown_partition(pixel_count);
    const Eigen::VectorXd unknown_values = large_unknown_values(pixel_count);
    ProgressLog progress_log;

    const auto outcome = algorithm::assemble_probability_map(
        conditions
        , partition
        , unknown_values
        , width
        , height
        , domain::CancellationToken {}
        , progress_log.reporter()
    );

    QVERIFY(std::holds_alternative<domain::ProbabilityMap>(outcome));
    const domain::ProbabilityMap& probabilities =
        std::get<domain::ProbabilityMap>(outcome);
    QCOMPARE(static_cast<int>(probabilities.rows()), height);
    QCOMPARE(static_cast<int>(probabilities.cols()), width);

    for (int index = 0; index < pixel_count; index += 997) {
        QCOMPARE(
            probabilities(index / width, index % width)
            , probability_value_for_index(index)
        );
    }
    QCOMPARE(
        probabilities((pixel_count - 1) / width, (pixel_count - 1) % width)
        , probability_value_for_index(pixel_count - 1)
    );
    verify_pixel_wise_progress_contract(
        progress_log
        , domain::SegmentationStage::AssemblingProbabilities
    );
}

void AlgorithmTests::large_probability_assembly_honors_cancellation() {
    const int width = large_parallel_width();
    const int height = large_parallel_height();
    const int pixel_count = width * height;
    const algorithm::BoundaryConditions conditions = empty_boundary_conditions();
    const algorithm::NodePartition partition = all_unknown_partition(pixel_count);
    const Eigen::VectorXd unknown_values = large_unknown_values(pixel_count);

    const auto outcome = algorithm::assemble_probability_map(
        conditions
        , partition
        , unknown_values
        , width
        , height
        , cancelled_token()
        , domain::ProgressReporter {}
    );

    QVERIFY(is_cancelled(outcome));
}

void AlgorithmTests::thresholds_probabilities_at_half() {
    domain::ProbabilityMap probabilities(2, 3);
    probabilities << 0.0, 0.49, 0.5,
        0.51, 1.0, 0.2;

    const auto outcome = algorithm::threshold_probabilities(
        probabilities
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<domain::BinaryMask>(outcome));
    const domain::BinaryMask& mask = std::get<domain::BinaryMask>(outcome);
    QCOMPARE(static_cast<int>(mask.rows()), 2);
    QCOMPARE(static_cast<int>(mask.cols()), 3);
    QCOMPARE(static_cast<int>(mask(0, 0)), 0);
    QCOMPARE(static_cast<int>(mask(0, 1)), 0);
    QCOMPARE(static_cast<int>(mask(0, 2)), 1);
    QCOMPARE(static_cast<int>(mask(1, 0)), 1);
    QCOMPARE(static_cast<int>(mask(1, 1)), 1);
    QCOMPARE(static_cast<int>(mask(1, 2)), 0);
}

void AlgorithmTests::thresholds_large_probability_map_deterministically() {
    const domain::ProbabilityMap probabilities = large_probability_map();
    const int width = static_cast<int>(probabilities.cols());
    const int height = static_cast<int>(probabilities.rows());
    const int pixel_count = width * height;
    ProgressLog progress_log;

    const auto outcome = algorithm::threshold_probabilities(
        probabilities
        , domain::CancellationToken {}
        , progress_log.reporter()
    );

    QVERIFY(std::holds_alternative<domain::BinaryMask>(outcome));
    const domain::BinaryMask& mask = std::get<domain::BinaryMask>(outcome);
    QCOMPARE(static_cast<int>(mask.rows()), height);
    QCOMPARE(static_cast<int>(mask.cols()), width);

    for (int index = 0; index < pixel_count; index += 997) {
        const int expected = probability_value_for_index(index) >= 0.5 ? 1 : 0;
        QCOMPARE(static_cast<int>(mask(index / width, index % width)), expected);
    }
    const int last_index = pixel_count - 1;
    QCOMPARE(
        static_cast<int>(mask(last_index / width, last_index % width))
        , probability_value_for_index(last_index) >= 0.5 ? 1 : 0
    );
    verify_pixel_wise_progress_contract(
        progress_log
        , domain::SegmentationStage::Thresholding
    );
}

void AlgorithmTests::large_thresholding_honors_cancellation() {
    const domain::ProbabilityMap probabilities = large_probability_map();

    const auto outcome = algorithm::threshold_probabilities(
        probabilities
        , cancelled_token()
        , domain::ProgressReporter {}
    );

    QVERIFY(is_cancelled(outcome));
}


void AlgorithmTests::global_beta_edge_weight_parameters_validate_domain_bounds() {
    QVERIFY(graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .beta = domain::kMinimumRandomWalkerBeta
        , .distance_power = domain::kMinimumRandomWalkerDistancePower
    }));
    QVERIFY(graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .beta = domain::kMaximumRandomWalkerBeta
        , .distance_power = domain::kMaximumRandomWalkerDistancePower
    }));
    QVERIFY(!graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .beta = std::nextafter(domain::kMinimumRandomWalkerBeta, 0.0)
    }));
    QVERIFY(!graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .beta = std::numeric_limits<double>::quiet_NaN()
    }));
    QVERIFY(!graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .distance_power = std::nextafter(
            domain::kMinimumRandomWalkerDistancePower
            , -1.0
        )
    }));
    QVERIFY(!graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .distance_power = std::nextafter(
            domain::kMaximumRandomWalkerDistancePower
            , 3.0
        )
    }));
    QVERIFY(!graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .distance_power = std::numeric_limits<double>::quiet_NaN()
    }));
    QVERIFY(!graph::is_valid(graph::GlobalBetaEdgeWeightParameters {
        .distance_power = std::numeric_limits<double>::infinity()
    }));
}


void AlgorithmTests::edge_weight_zero_distance_power_matches_baseline() {
    constexpr double legacy_raw_beta = 0.01;
    const graph::GlobalBetaEdgeWeightParameters parameters {
        .beta = normalized_beta_from_raw_beta(legacy_raw_beta)
        , .distance_power = 0.0
    };

    const double diagonal_weight = graph::compute_global_beta_edge_weight(
        edge_weight_input(0, 10, std::sqrt(2.0))
        , parameters
    );

    QVERIFY(std::abs(diagonal_weight - std::exp(-legacy_raw_beta * 100.0))
        < 1e-12);
}

void AlgorithmTests::edge_weight_keeps_orthogonal_edges_independent_of_distance_power() {
    constexpr double legacy_raw_beta = 0.01;
    const graph::GlobalBetaEdgeWeightParameters weak_distance_penalty {
        .beta = normalized_beta_from_raw_beta(legacy_raw_beta)
        , .distance_power = 0.0
    };
    const graph::GlobalBetaEdgeWeightParameters strong_distance_penalty {
        .beta = normalized_beta_from_raw_beta(legacy_raw_beta)
        , .distance_power = 2.0
    };

    const double baseline = graph::compute_global_beta_edge_weight(
        edge_weight_input(0, 10, 1.0)
        , weak_distance_penalty
    );
    const double normalized = graph::compute_global_beta_edge_weight(
        edge_weight_input(0, 10, 1.0)
        , strong_distance_penalty
    );

    QVERIFY(std::abs(baseline - normalized) < 1e-12);
    QVERIFY(std::abs(normalized - std::exp(-legacy_raw_beta * 100.0))
        < 1e-12);
}

void AlgorithmTests::edge_weight_uses_distance_power() {
    constexpr double legacy_raw_beta = 0.01;
    const graph::GlobalBetaEdgeWeightParameters linear_distance_penalty =
        graph::global_beta_edge_weight_parameters_from(
            random_walker_parameters(
                normalized_beta_from_raw_beta(legacy_raw_beta)
                , domain::PixelConnectivity::Eight
                , 1.0
            )
        );
    const graph::GlobalBetaEdgeWeightParameters quadratic_distance_penalty =
        graph::global_beta_edge_weight_parameters_from(
            random_walker_parameters(
                normalized_beta_from_raw_beta(legacy_raw_beta)
                , domain::PixelConnectivity::Eight
                , 2.0
            )
        );

    const double linear_diagonal_weight = graph::compute_global_beta_edge_weight(
        edge_weight_input(0, 10, std::sqrt(2.0))
        , linear_distance_penalty
    );
    const double quadratic_diagonal_weight = graph::compute_global_beta_edge_weight(
        edge_weight_input(0, 10, std::sqrt(2.0))
        , quadratic_distance_penalty
    );

    const double contrast_weight = std::exp(-legacy_raw_beta * 100.0);
    QVERIFY(
        std::abs(linear_diagonal_weight - contrast_weight / std::sqrt(2.0))
        < 1e-12
    );
    QVERIFY(std::abs(quadratic_diagonal_weight - contrast_weight / 2.0)
        < 1e-12);
}

void AlgorithmTests::edge_weight_function_object_matches_pure_formula() {
    const graph::GlobalBetaEdgeWeightParameters parameters {
        .beta = domain::kDefaultRandomWalkerBeta
        , .distance_power = 1.0
    };
    const graph::GlobalBetaEdgeWeight edge_weight {
        .parameters = parameters
    };

    const double direct_weight = graph::compute_global_beta_edge_weight(
        edge_weight_input(0, 10, std::sqrt(2.0))
        , parameters
    );
    const double function_object_weight = edge_weight(
        edge_weight_input(0, 10, std::sqrt(2.0))
    );

    QVERIFY(std::abs(function_object_weight - direct_weight) < 1e-12);
}

void AlgorithmTests::grid_laplacian_has_expected_2x2_structure() {
    const domain::GrayImage image = make_image(2, 2, {10, 10, 10, 10});

    const auto outcome = graph::build_grid_laplacian(
        image
        , random_walker_parameters(0.01, domain::PixelConnectivity::Four)
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<Eigen::SparseMatrix<double>>(outcome));
    const Eigen::SparseMatrix<double>& laplacian =
        std::get<Eigen::SparseMatrix<double>>(outcome);
    QCOMPARE(static_cast<int>(laplacian.rows()), 4);
    QCOMPARE(static_cast<int>(laplacian.cols()), 4);

    for (int index = 0; index < 4; ++index) {
        QCOMPARE(coefficient(laplacian, index, index), 2.0);
    }

    QCOMPARE(coefficient(laplacian, 0, 1), -1.0);
    QCOMPARE(coefficient(laplacian, 1, 0), -1.0);
    QCOMPARE(coefficient(laplacian, 0, 2), -1.0);
    QCOMPARE(coefficient(laplacian, 2, 0), -1.0);
    QCOMPARE(coefficient(laplacian, 1, 3), -1.0);
    QCOMPARE(coefficient(laplacian, 3, 1), -1.0);
    QCOMPARE(coefficient(laplacian, 2, 3), -1.0);
    QCOMPARE(coefficient(laplacian, 3, 2), -1.0);
    QCOMPARE(coefficient(laplacian, 0, 3), 0.0);
    QCOMPARE(coefficient(laplacian, 1, 2), 0.0);
}

void AlgorithmTests::grid_laplacian_eight_connectivity_adds_diagonal_edges() {
    const domain::GrayImage image = make_image(2, 2, {10, 10, 10, 10});

    const auto outcome = graph::build_grid_laplacian(
        image
        , random_walker_parameters(0.01, domain::PixelConnectivity::Eight)
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<Eigen::SparseMatrix<double>>(outcome));
    const Eigen::SparseMatrix<double>& laplacian =
        std::get<Eigen::SparseMatrix<double>>(outcome);
    QCOMPARE(static_cast<int>(laplacian.rows()), 4);
    QCOMPARE(static_cast<int>(laplacian.cols()), 4);

    for (int index = 0; index < 4; ++index) {
        QCOMPARE(coefficient(laplacian, index, index), 3.0);
    }

    QCOMPARE(coefficient(laplacian, 0, 1), -1.0);
    QCOMPARE(coefficient(laplacian, 1, 0), -1.0);
    QCOMPARE(coefficient(laplacian, 0, 2), -1.0);
    QCOMPARE(coefficient(laplacian, 2, 0), -1.0);
    QCOMPARE(coefficient(laplacian, 1, 3), -1.0);
    QCOMPARE(coefficient(laplacian, 3, 1), -1.0);
    QCOMPARE(coefficient(laplacian, 2, 3), -1.0);
    QCOMPARE(coefficient(laplacian, 3, 2), -1.0);
    QCOMPARE(coefficient(laplacian, 0, 3), -1.0);
    QCOMPARE(coefficient(laplacian, 3, 0), -1.0);
    QCOMPARE(coefficient(laplacian, 1, 2), -1.0);
    QCOMPARE(coefficient(laplacian, 2, 1), -1.0);
}

void AlgorithmTests::grid_laplacian_distance_power_normalizes_diagonal_edges() {
    const domain::GrayImage image = make_image(2, 2, {10, 10, 10, 10});

    const auto outcome = graph::build_grid_laplacian(
        image
        , random_walker_parameters(0.01, domain::PixelConnectivity::Eight, 1.0)
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<Eigen::SparseMatrix<double>>(outcome));
    const Eigen::SparseMatrix<double>& laplacian =
        std::get<Eigen::SparseMatrix<double>>(outcome);
    const double diagonal_weight = 1.0 / std::sqrt(2.0);
    const double expected_degree = 2.0 + diagonal_weight;

    for (int index = 0; index < 4; ++index) {
        QVERIFY(std::abs(coefficient(laplacian, index, index) - expected_degree)
            < 1e-12);
    }

    QCOMPARE(coefficient(laplacian, 0, 1), -1.0);
    QCOMPARE(coefficient(laplacian, 0, 2), -1.0);
    QVERIFY(std::abs(coefficient(laplacian, 0, 3) + diagonal_weight) < 1e-12);
    QVERIFY(std::abs(coefficient(laplacian, 1, 2) + diagonal_weight) < 1e-12);
}

void AlgorithmTests::grid_laplacian_beta_changes_edge_weight() {
    const domain::GrayImage image = make_image(1, 2, {0, 10});
    constexpr double weak_legacy_raw_beta = 0.001;
    constexpr double strong_legacy_raw_beta = 0.01;

    const auto weak_penalty_outcome = graph::build_grid_laplacian(
        image
        , random_walker_parameters(
            normalized_beta_from_raw_beta(weak_legacy_raw_beta)
            , domain::PixelConnectivity::Four
        )
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );
    const auto strong_penalty_outcome = graph::build_grid_laplacian(
        image
        , random_walker_parameters(
            normalized_beta_from_raw_beta(strong_legacy_raw_beta)
            , domain::PixelConnectivity::Four
        )
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<Eigen::SparseMatrix<double>>(weak_penalty_outcome));
    QVERIFY(std::holds_alternative<Eigen::SparseMatrix<double>>(strong_penalty_outcome));
    const auto& weak_penalty =
        std::get<Eigen::SparseMatrix<double>>(weak_penalty_outcome);
    const auto& strong_penalty =
        std::get<Eigen::SparseMatrix<double>>(strong_penalty_outcome);

    const double weak_weight = -coefficient(weak_penalty, 0, 1);
    const double strong_weight = -coefficient(strong_penalty, 0, 1);
    QVERIFY(weak_weight > strong_weight);
    QVERIFY(std::abs(weak_weight - std::exp(-weak_legacy_raw_beta * 100.0))
        < 1e-12);
    QVERIFY(std::abs(strong_weight - std::exp(-strong_legacy_raw_beta * 100.0))
        < 1e-12);
}


QTEST_GUILESS_MAIN(AlgorithmTests)
#include "AlgorithmTests.moc"
