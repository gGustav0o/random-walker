#include "ProbabilityField.hpp"

#include "IterationPolicy.hpp"
#include "ParallelPolicy.hpp"
#include "model/domain/ImageGeometry.hpp"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <optional>

namespace random_walker::algorithm {
    namespace {
        using ProbabilityAssemblyStepOutcome = std::optional<domain::Cancelled>;
        using ThresholdStepOutcome = std::optional<domain::Cancelled>;

        [[nodiscard]] std::size_t pixel_work_item_count(
            int width
            , int height
        ) noexcept {
            assert(width > 0);
            assert(height > 0);
            return domain::pixel_count(width, height);
        }

        [[nodiscard]] double probability_assembly_progress(
            int index
            , int pixel_count
        ) noexcept {
            assert(index >= 0);
            assert(pixel_count > 0);
            return static_cast<double>(index + 1)
                / static_cast<double>(pixel_count);
        }

        void write_probability_at_index(
            domain::ProbabilityMap& result
            , const BoundaryConditions& boundary_conditions
            , const NodePartition& node_partition
            , const Eigen::VectorXd& unknown_values
            , int index
            , int width
        ) {
            assert(index >= 0);
            assert(width > 0);
            assert(result.cols() == width);

            const int row = index / width;
            const int column = index % width;
            assert(row >= 0);
            assert(row < result.rows());
            assert(column >= 0);
            assert(column < result.cols());

            const PixelIndex pixel_index {
                .value = index
            };

            if (boundary_conditions.contains(pixel_index)) {
                result(row, column) = boundary_conditions.value_at(pixel_index);
                return;
            }

            const auto unknown_position =
                node_partition.unknown_index_by_pixel.find(pixel_index);
            assert(
                unknown_position
                != node_partition.unknown_index_by_pixel.end()
            );
            assert(unknown_position->second.value >= 0);
            assert(
                static_cast<Eigen::Index>(unknown_position->second.value)
                < unknown_values.size()
            );
            result(row, column) = unknown_values[unknown_position->second.value];
        }

        [[nodiscard]] ProbabilityAssemblyStepOutcome assemble_probabilities_sequential(
            domain::ProbabilityMap& result
            , const BoundaryConditions& boundary_conditions
            , const NodePartition& node_partition
            , const Eigen::VectorXd& unknown_values
            , int width
            , int pixel_count
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            for (int index = 0; index < pixel_count; ++index) {
                if (should_poll_cancellation(static_cast<std::size_t>(index))
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                write_probability_at_index(
                    result
                    , boundary_conditions
                    , node_partition
                    , unknown_values
                    , index
                    , width
                );

                if (should_report_progress(static_cast<std::size_t>(index))) {
                    progress.report(
                        domain::SegmentationStage::AssemblingProbabilities
                        , probability_assembly_progress(index, pixel_count)
                    );
                }
            }

            return std::nullopt;
        }

#if RANDOM_WALKER_ENABLE_OPENMP
        [[nodiscard]] ProbabilityAssemblyStepOutcome assemble_probabilities_parallel(
            domain::ProbabilityMap& result
            , const BoundaryConditions& boundary_conditions
            , const NodePartition& node_partition
            , const Eigen::VectorXd& unknown_values
            , int width
            , int pixel_count
            , const domain::CancellationToken& cancellation
        ) {
            std::atomic_bool cancelled { false };

#pragma omp parallel for schedule(static)
            for (int index = 0; index < pixel_count; ++index) {
                if (cancelled.load(std::memory_order_relaxed)) {
                    continue;
                }

                if (should_poll_cancellation(static_cast<std::size_t>(index))
                    && cancellation.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    continue;
                }

                write_probability_at_index(
                    result
                    , boundary_conditions
                    , node_partition
                    , unknown_values
                    , index
                    , width
                );
            }

            if (cancelled.load(std::memory_order_relaxed)) {
                return domain::Cancelled {};
            }

            return std::nullopt;
        }
#endif

        void write_threshold_row(
            domain::BinaryMask& mask
            , const domain::ProbabilityMap& probabilities
            , int row
            , int width
        ) {
            assert(row >= 0);
            assert(row < probabilities.rows());
            assert(width > 0);
            assert(width == probabilities.cols());
            assert(mask.rows() == probabilities.rows());
            assert(mask.cols() == probabilities.cols());

            for (int column = 0; column < width; ++column) {
                mask(row, column) =
                    probabilities(row, column) >= 0.5 ? 1 : 0;
            }
        }

        [[nodiscard]] double threshold_progress(
            int row
            , int height
        ) noexcept {
            assert(row >= 0);
            assert(height > 0);
            return static_cast<double>(row + 1) / static_cast<double>(height);
        }

        [[nodiscard]] ThresholdStepOutcome threshold_probabilities_sequential(
            domain::BinaryMask& mask
            , const domain::ProbabilityMap& probabilities
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            const int height = static_cast<int>(probabilities.rows());
            const int width = static_cast<int>(probabilities.cols());

            for (int row = 0; row < height; ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                write_threshold_row(mask, probabilities, row, width);
                progress.report(
                    domain::SegmentationStage::Thresholding
                    , threshold_progress(row, height)
                );
            }

            return std::nullopt;
        }

#if RANDOM_WALKER_ENABLE_OPENMP
        [[nodiscard]] ThresholdStepOutcome threshold_probabilities_parallel(
            domain::BinaryMask& mask
            , const domain::ProbabilityMap& probabilities
            , const domain::CancellationToken& cancellation
        ) {
            const int height = static_cast<int>(probabilities.rows());
            const int width = static_cast<int>(probabilities.cols());
            std::atomic_bool cancelled { false };

#pragma omp parallel for schedule(static)
            for (int row = 0; row < height; ++row) {
                if (cancelled.load(std::memory_order_relaxed)) {
                    continue;
                }

                if (cancellation.stop_requested()) {
                    cancelled.store(true, std::memory_order_relaxed);
                    continue;
                }

                write_threshold_row(mask, probabilities, row, width);
            }

            if (cancelled.load(std::memory_order_relaxed)) {
                return domain::Cancelled {};
            }

            return std::nullopt;
        }
#endif
    }

    ProbabilityMapOutcome assemble_probability_map(
        const BoundaryConditions& boundary_conditions
        , const NodePartition& node_partition
        , const Eigen::VectorXd& unknown_values
        , int width
        , int height
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(width > 0);
        assert(height > 0);
        assert(
            unknown_values.size()
            == static_cast<Eigen::Index>(node_partition.unknown_pixels.size())
        );
        assert(
            node_partition.unknown_pixels.size()
            == node_partition.unknown_index_by_pixel.size()
        );
        assert(
            node_partition.boundary_pixels.size()
            == node_partition.boundary_index_by_pixel.size()
        );
        assert(
            node_partition.unknown_pixels.size()
                + node_partition.boundary_pixels.size()
            == domain::pixel_count(width, height)
        );
        assert(
            boundary_conditions.pixels.size()
            == boundary_conditions.value_by_pixel.size()
        );

        domain::ProbabilityMap result(height, width);
        const int pixel_count = domain::pixel_count_as_int(width, height);

        progress.report(
            domain::SegmentationStage::AssemblingProbabilities
            , 0.0
        );

#if RANDOM_WALKER_ENABLE_OPENMP
        if (should_run_parallel(pixel_work_item_count(width, height))) {
            if (const auto cancelled = assemble_probabilities_parallel(
                    result
                    , boundary_conditions
                    , node_partition
                    , unknown_values
                    , width
                    , pixel_count
                    , cancellation
                ); cancelled.has_value()
            ) {
                return *cancelled;
            }

            progress.report(
                domain::SegmentationStage::AssemblingProbabilities
                , 1.0
            );
            return result;
        }
#endif

        if (const auto cancelled = assemble_probabilities_sequential(
                result
                , boundary_conditions
                , node_partition
                , unknown_values
                , width
                , pixel_count
                , cancellation
                , progress
            ); cancelled.has_value()
        ) {
            return *cancelled;
        }

        progress.report(
            domain::SegmentationStage::AssemblingProbabilities
            , 1.0
        );
        return result;
    }

    BinaryMaskOutcome threshold_probabilities(
        const domain::ProbabilityMap& probabilities
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        const int height = static_cast<int>(probabilities.rows());
        const int width = static_cast<int>(probabilities.cols());
        assert(height > 0);
        assert(width > 0);
        domain::BinaryMask mask(height, width);

        progress.report(domain::SegmentationStage::Thresholding, 0.0);

#if RANDOM_WALKER_ENABLE_OPENMP
        if (should_run_parallel(pixel_work_item_count(width, height))) {
            if (const auto cancelled = threshold_probabilities_parallel(
                    mask
                    , probabilities
                    , cancellation
                ); cancelled.has_value()
            ) {
                return *cancelled;
            }

            progress.report(domain::SegmentationStage::Thresholding, 1.0);
            return mask;
        }
#endif

        if (const auto cancelled = threshold_probabilities_sequential(
                mask
                , probabilities
                , cancellation
                , progress
            ); cancelled.has_value()
        ) {
            return *cancelled;
        }

        return mask;
    }
}
