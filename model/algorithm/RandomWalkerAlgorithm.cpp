#include "RandomWalkerAlgorithm.hpp"

#include <utility>
#include <variant>

#include <Eigen/Sparse>

#include "BoundaryConditions.hpp"
#include "HarmonicSolver.hpp"
#include "NodePartition.hpp"
#include "PartitionedLaplacian.hpp"
#include "ProbabilityField.hpp"
#include "model/graph/GridLaplacian.hpp"

namespace random_walker::algorithm::detail {
    namespace {
        [[nodiscard]] bool is_cancelled(
            const auto& outcome) {
            return std::holds_alternative<domain::Cancelled>(outcome);
        }

        [[nodiscard]] domain::SegmentationOutcome handle_fully_constrained_image(
            const BoundaryConditions& boundary_conditions
            , const NodePartition& node_partition
            , int width
            , int height
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }

            progress.report(
                domain::SegmentationStage::PartitioningSystem
                , 1.0
            );

            const Eigen::VectorXd empty_unknown_values;
            ProbabilityMapOutcome probabilities_outcome =
                assemble_probability_map(
                    boundary_conditions
                    , node_partition
                    , empty_unknown_values
                    , width
                    , height
                    , cancellation
                    , progress
                );
            if (is_cancelled(probabilities_outcome)) {
                return domain::Cancelled {};
            }
            domain::ProbabilityMap probabilities =
                std::get<domain::ProbabilityMap>(std::move(probabilities_outcome));

            BinaryMaskOutcome mask_outcome =
                threshold_probabilities(
                    probabilities
                    , cancellation
                    , progress
                );
            if (is_cancelled(mask_outcome)) {
                return domain::Cancelled {};
            }

            return domain::SegmentationResult {
                .probabilities = std::move(probabilities)
                , .mask = std::get<domain::BinaryMask>(std::move(mask_outcome))
            };
        }

        [[nodiscard]] domain::SegmentationOutcome solve_unconstrained_pixels(
            const Eigen::SparseMatrix<double>& laplacian
            , const BoundaryConditions& boundary_conditions
            , const NodePartition& node_partition
            , int width
            , int height
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            PartitionedLaplacianOutcome laplacian_partition_outcome =
                partition_laplacian(
                    laplacian
                    , node_partition
                    , cancellation
                    , progress
                );
            if (is_cancelled(laplacian_partition_outcome)) {
                return domain::Cancelled {};
            }
            PartitionedLaplacian laplacian_partition =
                std::get<PartitionedLaplacian>(
                    std::move(laplacian_partition_outcome)
                );

            BoundaryValuesOutcome boundary_values_outcome =
                build_boundary_value_vector(
                    boundary_conditions
                    , node_partition.boundary_pixels
                    , cancellation
                    , progress
                );
            if (is_cancelled(boundary_values_outcome)) {
                return domain::Cancelled {};
            }
            Eigen::VectorXd boundary_values =
                std::get<Eigen::VectorXd>(std::move(boundary_values_outcome));

            HarmonicSolveOutcome solve_outcome =
                solve_harmonic_dirichlet_problem(
                    laplacian_partition
                    , boundary_values
                    , cancellation
                    , progress
                );
            if (const auto* error =
                    std::get_if<domain::SegmentationError>(&solve_outcome)) {
                return *error;
            }
            if (is_cancelled(solve_outcome)) {
                return domain::Cancelled {};
            }
            Eigen::VectorXd unknown_values =
                std::get<Eigen::VectorXd>(std::move(solve_outcome));

            ProbabilityMapOutcome probabilities_outcome =
                assemble_probability_map(
                    boundary_conditions
                    , node_partition
                    , unknown_values
                    , width
                    , height
                    , cancellation
                    , progress
                );
            if (is_cancelled(probabilities_outcome)) {
                return domain::Cancelled {};
            }
            domain::ProbabilityMap probabilities =
                std::get<domain::ProbabilityMap>(std::move(probabilities_outcome));

            BinaryMaskOutcome mask_outcome =
                threshold_probabilities(
                    probabilities
                    , cancellation
                    , progress
                );
            if (is_cancelled(mask_outcome)) {
                return domain::Cancelled {};
            }

            return domain::SegmentationResult {
                .probabilities = std::move(probabilities)
                , .mask = std::get<domain::BinaryMask>(std::move(mask_outcome))
            };
        }
    }

    domain::SegmentationOutcome run_validated_random_walker(
        const domain::SegmentationInput& input
        , double beta
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        const int width = input.image.width();
        const int height = input.image.height();
        const int pixel_count = width * height;

        graph::GridLaplacianOutcome laplacian_outcome =
            graph::build_grid_laplacian(
                input.image
                , beta
                , cancellation
                , progress
            );
        if (is_cancelled(laplacian_outcome)) {
            return domain::Cancelled {};
        }
        Eigen::SparseMatrix<double> laplacian =
            std::get<Eigen::SparseMatrix<double>>(std::move(laplacian_outcome));

        progress.report(domain::SegmentationStage::BuildingLabels, 0.0);
        BoundaryConditionsOutcome boundary_outcome =
            build_boundary_conditions(
                input
                , cancellation
                , progress
            );
        if (is_cancelled(boundary_outcome)) {
            return domain::Cancelled {};
        }
        BoundaryConditions boundary_conditions =
            std::get<BoundaryConditions>(std::move(boundary_outcome));

        progress.report(domain::SegmentationStage::PartitioningSystem, 0.0);
        NodePartitionOutcome node_partition_outcome =
            partition_nodes(
                pixel_count
                , boundary_conditions
                , cancellation
                , progress
            );
        if (is_cancelled(node_partition_outcome)) {
            return domain::Cancelled {};
        }
        NodePartition node_partition =
            std::get<NodePartition>(std::move(node_partition_outcome));

        if (node_partition.unknown_pixels.empty()) {
            return handle_fully_constrained_image(
                boundary_conditions
                , node_partition
                , width
                , height
                , cancellation
                , progress
            );
        }

        return solve_unconstrained_pixels(
            laplacian
            , boundary_conditions
            , node_partition
            , width
            , height
            , cancellation
            , progress
        );
    }
}
