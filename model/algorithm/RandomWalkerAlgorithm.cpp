#include "RandomWalkerAlgorithm.hpp"

#include "model/graph/PixelGraph.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

namespace random_walker::algorithm::detail
{
    namespace
    {
        using Eigen::MatrixXi;
        using Eigen::SparseMatrix;
        using Eigen::VectorXd;

        struct LabelData
        {
            MatrixXi labels;
            std::unordered_set<int> labeled_indices;
        };

        struct UnlabeledData
        {
            std::vector<int> indices;
            std::unordered_map<int, int> index_by_pixel;
        };

        struct LaplacianBlocks
        {
            SparseMatrix<double> unlabeled_to_unlabeled;
            SparseMatrix<double> unlabeled_to_labeled;
        };

        template <typename Value>
        using CancellableOutcome = std::variant<Value, domain::Cancelled>;

        using SolveOutcome = std::variant<
            VectorXd,
            domain::SegmentationError,
            domain::Cancelled>;

        [[nodiscard]] constexpr int flatten(int row, int column, int width) noexcept
        {
            return row * width + column;
        }

        [[nodiscard]] CancellableOutcome<LabelData> build_labels(
            const domain::SegmentationInput& input,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            const int height = input.image.height();
            const int width = input.image.width();

            LabelData result {
                .labels = MatrixXi::Zero(height, width),
                .labeled_indices = {}
            };

            std::size_t completed = 0;
            const std::size_t total = input.seeds.size() * 2;
            const auto apply_seeds = [&](domain::SeedLabel label, int value) {
                for (const domain::Seed& seed : input.seeds) {
                    if (cancellation.stop_requested()) {
                        return false;
                    }

                    if (seed.label == label) {
                        result.labels(seed.position.y, seed.position.x) = value;
                        result.labeled_indices.insert(
                            flatten(seed.position.y, seed.position.x, width));
                    }

                    ++completed;
                    if ((completed & 0x0fff) == 0) {
                        progress.report(
                            domain::SegmentationStage::BuildingLabels,
                            total == 0
                                ? 1.0
                                : static_cast<double>(completed)
                                    / static_cast<double>(total));
                    }
                }
                return true;
            };

            if (!apply_seeds(domain::SeedLabel::Background, 0)
                || !apply_seeds(domain::SeedLabel::Object, 1)) {
                return domain::Cancelled {};
            }
            progress.report(
                domain::SegmentationStage::BuildingLabels,
                1.0);
            return result;
        }

        [[nodiscard]] CancellableOutcome<UnlabeledData>
        extract_unlabeled_indices(
            int pixel_count,
            const std::unordered_set<int>& labeled_indices,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            UnlabeledData result;
            result.indices.reserve(
                static_cast<std::size_t>(pixel_count) - labeled_indices.size());

            for (int index = 0; index < pixel_count; ++index) {
                if ((index & 0x0fff) == 0
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                if (labeled_indices.contains(index)) {
                    continue;
                }

                const int unlabeled_index = static_cast<int>(result.indices.size());
                result.indices.push_back(index);
                result.index_by_pixel.emplace(index, unlabeled_index);

                if ((index & 0x0fff) == 0) {
                    progress.report(
                        domain::SegmentationStage::PartitioningSystem,
                        0.25 * static_cast<double>(index + 1)
                            / static_cast<double>(pixel_count));
                }
            }

            progress.report(
                domain::SegmentationStage::PartitioningSystem,
                0.25);
            return result;
        }

        [[nodiscard]] CancellableOutcome<std::unordered_map<int, int>>
        index_by_pixel(
            const std::vector<int>& pixel_indices,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            std::unordered_map<int, int> result;
            result.reserve(pixel_indices.size());

            for (std::size_t index = 0; index < pixel_indices.size(); ++index) {
                if ((index & 0x0fff) == 0
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                result.emplace(pixel_indices[index], static_cast<int>(index));

                if ((index & 0x0fff) == 0) {
                    progress.report(
                        domain::SegmentationStage::PartitioningSystem,
                        0.25
                            + 0.15 * static_cast<double>(index + 1)
                                / static_cast<double>(
                                    pixel_indices.size()));
                }
            }

            progress.report(
                domain::SegmentationStage::PartitioningSystem,
                0.4);
            return result;
        }

        [[nodiscard]] CancellableOutcome<LaplacianBlocks> split_laplacian(
            const SparseMatrix<double>& laplacian,
            const std::unordered_map<int, int>& labeled_index,
            const std::unordered_map<int, int>& unlabeled_index,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            LaplacianBlocks result {
                .unlabeled_to_unlabeled = SparseMatrix<double>(
                    static_cast<int>(unlabeled_index.size()),
                    static_cast<int>(unlabeled_index.size())),
                .unlabeled_to_labeled = SparseMatrix<double>(
                    static_cast<int>(unlabeled_index.size()),
                    static_cast<int>(labeled_index.size()))
            };

            std::vector<Eigen::Triplet<double>> unlabeled_triplets;
            std::vector<Eigen::Triplet<double>> labeled_triplets;

            for (int outer = 0; outer < laplacian.outerSize(); ++outer) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (SparseMatrix<double>::InnerIterator entry(laplacian, outer);
                     entry;
                     ++entry) {
                    const auto row = unlabeled_index.find(entry.row());
                    if (row == unlabeled_index.end()) {
                        continue;
                    }

                    if (const auto column = unlabeled_index.find(entry.col());
                        column != unlabeled_index.end()) {
                        unlabeled_triplets.emplace_back(
                            row->second,
                            column->second,
                            entry.value());
                    } else if (const auto column = labeled_index.find(entry.col());
                               column != labeled_index.end()) {
                        labeled_triplets.emplace_back(
                            row->second,
                            column->second,
                            entry.value());
                    }
                }

                progress.report(
                    domain::SegmentationStage::PartitioningSystem,
                    0.4
                        + 0.45 * static_cast<double>(outer + 1)
                            / static_cast<double>(
                                laplacian.outerSize()));
            }

            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }

            result.unlabeled_to_unlabeled.setFromTriplets(
                unlabeled_triplets.begin(),
                unlabeled_triplets.end());
            result.unlabeled_to_labeled.setFromTriplets(
                labeled_triplets.begin(),
                labeled_triplets.end());
            progress.report(
                domain::SegmentationStage::PartitioningSystem,
                0.85);
            return result;
        }

        [[nodiscard]] CancellableOutcome<VectorXd> build_boundary_values(
            const MatrixXi& labels,
            const std::vector<int>& labeled_pixels,
            int width,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            VectorXd values(static_cast<int>(labeled_pixels.size()));

            for (std::size_t index = 0; index < labeled_pixels.size(); ++index) {
                if ((index & 0x0fff) == 0
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                const int pixel = labeled_pixels[index];
                values[static_cast<int>(index)] = labels(pixel / width, pixel % width);

                if ((index & 0x0fff) == 0) {
                    progress.report(
                        domain::SegmentationStage::PartitioningSystem,
                        0.85
                            + 0.15 * static_cast<double>(index + 1)
                                / static_cast<double>(
                                    labeled_pixels.size()));
                }
            }

            progress.report(
                domain::SegmentationStage::PartitioningSystem,
                1.0);
            return values;
        }

        [[nodiscard]] SolveOutcome solve_sparse_system(
            const SparseMatrix<double>& unlabeled_to_unlabeled,
            const SparseMatrix<double>& unlabeled_to_labeled,
            const VectorXd& boundary_values,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }

            progress.report_indeterminate(
                domain::SegmentationStage::Factorizing);
            Eigen::SimplicialLLT<SparseMatrix<double>> solver;
            solver.compute(unlabeled_to_unlabeled);

            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            if (solver.info() != Eigen::Success) {
                return domain::SegmentationError::LaplacianDecompositionFailed;
            }

            progress.report_indeterminate(
                domain::SegmentationStage::Solving);
            VectorXd solution = solver.solve(
                -unlabeled_to_labeled * boundary_values);

            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            if (solver.info() != Eigen::Success) {
                return domain::SegmentationError::LinearSystemSolveFailed;
            }
            if (!solution.allFinite()) {
                return domain::SegmentationError::NonFiniteSolution;
            }

            return solution;
        }

        [[nodiscard]] CancellableOutcome<domain::ProbabilityMap>
        assemble_probabilities(
            const MatrixXi& labels,
            const std::unordered_set<int>& labeled_indices,
            const std::unordered_map<int, int>& unlabeled_index,
            const VectorXd& unlabeled_values,
            int width,
            int height,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            domain::ProbabilityMap result(height, width);
            const int pixel_count = width * height;

            progress.report(
                domain::SegmentationStage::AssemblingProbabilities,
                0.0);
            for (int index = 0; index < pixel_count; ++index) {
                if ((index & 0x0fff) == 0
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                const int row = index / width;
                const int column = index % width;

                if (labeled_indices.contains(index)) {
                    result(row, column) = labels(row, column);
                } else {
                    result(row, column) = unlabeled_values[unlabeled_index.at(index)];
                }

                if ((index & 0x0fff) == 0) {
                    progress.report(
                        domain::SegmentationStage::AssemblingProbabilities,
                        static_cast<double>(index + 1)
                            / static_cast<double>(pixel_count));
                }
            }

            progress.report(
                domain::SegmentationStage::AssemblingProbabilities,
                1.0);
            return result;
        }

        [[nodiscard]] CancellableOutcome<domain::BinaryMask>
        threshold_probabilities(
            const domain::ProbabilityMap& probabilities,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            const int height = static_cast<int>(probabilities.rows());
            const int width = static_cast<int>(probabilities.cols());
            domain::BinaryMask mask(height, width);

            progress.report(domain::SegmentationStage::Thresholding, 0.0);
            for (int row = 0; row < height; ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (int column = 0; column < width; ++column) {
                    mask(row, column) =
                        probabilities(row, column) >= 0.5 ? 1 : 0;
                }

                progress.report(
                    domain::SegmentationStage::Thresholding,
                    static_cast<double>(row + 1)
                        / static_cast<double>(height));
            }

            return mask;
        }
    }

    domain::SegmentationOutcome run_validated_random_walker(
        const domain::SegmentationInput& input,
        double beta,
        const domain::CancellationToken& cancellation,
        const domain::ProgressReporter& progress)
    {
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        const int width = input.image.width();
        const int height = input.image.height();
        const int pixel_count = width * height;

        graph::LaplacianOutcome laplacian_outcome =
            graph::build_laplacian(
                input.image,
                beta,
                cancellation,
                progress);
        if (std::holds_alternative<domain::Cancelled>(laplacian_outcome)) {
            return domain::Cancelled {};
        }
        SparseMatrix<double> laplacian =
            std::get<SparseMatrix<double>>(std::move(laplacian_outcome));

        progress.report(domain::SegmentationStage::BuildingLabels, 0.0);
        CancellableOutcome<LabelData> label_outcome =
            build_labels(input, cancellation, progress);
        if (std::holds_alternative<domain::Cancelled>(label_outcome)) {
            return domain::Cancelled {};
        }
        LabelData label_data =
            std::get<LabelData>(std::move(label_outcome));

        progress.report(domain::SegmentationStage::PartitioningSystem, 0.0);
        CancellableOutcome<UnlabeledData> unlabeled_outcome =
            extract_unlabeled_indices(
            pixel_count,
            label_data.labeled_indices,
            cancellation,
            progress);
        if (std::holds_alternative<domain::Cancelled>(unlabeled_outcome)) {
            return domain::Cancelled {};
        }
        UnlabeledData unlabeled_data =
            std::get<UnlabeledData>(std::move(unlabeled_outcome));

        domain::ProbabilityMap probabilities;

        if (unlabeled_data.indices.empty()) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            progress.report(
                domain::SegmentationStage::PartitioningSystem,
                1.0);
            progress.report(
                domain::SegmentationStage::AssemblingProbabilities,
                0.0);
            probabilities = label_data.labels.cast<double>();
            progress.report(
                domain::SegmentationStage::AssemblingProbabilities,
                1.0);
        } else {
            const std::vector<int> labeled_pixels(
                label_data.labeled_indices.begin(),
                label_data.labeled_indices.end());

            auto labeled_index_outcome =
                index_by_pixel(
                    labeled_pixels,
                    cancellation,
                    progress);
            if (std::holds_alternative<domain::Cancelled>(
                    labeled_index_outcome)) {
                return domain::Cancelled {};
            }
            std::unordered_map<int, int> labeled_index =
                std::get<std::unordered_map<int, int>>(
                    std::move(labeled_index_outcome));
            CancellableOutcome<LaplacianBlocks> blocks_outcome =
                split_laplacian(
                laplacian,
                labeled_index,
                unlabeled_data.index_by_pixel,
                cancellation,
                progress);
            if (std::holds_alternative<domain::Cancelled>(blocks_outcome)) {
                return domain::Cancelled {};
            }
            LaplacianBlocks blocks =
                std::get<LaplacianBlocks>(std::move(blocks_outcome));
            CancellableOutcome<VectorXd> boundary_outcome =
                build_boundary_values(
                label_data.labels,
                labeled_pixels,
                width,
                cancellation,
                progress);
            if (std::holds_alternative<domain::Cancelled>(boundary_outcome)) {
                return domain::Cancelled {};
            }
            VectorXd boundary_values =
                std::get<VectorXd>(std::move(boundary_outcome));
            SolveOutcome solve_outcome = solve_sparse_system(
                blocks.unlabeled_to_unlabeled,
                blocks.unlabeled_to_labeled,
                boundary_values,
                cancellation,
                progress);

            if (const auto* error =
                    std::get_if<domain::SegmentationError>(&solve_outcome)) {
                return *error;
            }
            if (std::holds_alternative<domain::Cancelled>(solve_outcome)) {
                return domain::Cancelled {};
            }

            VectorXd unlabeled_values =
                std::get<VectorXd>(std::move(solve_outcome));
            CancellableOutcome<domain::ProbabilityMap>
                probabilities_outcome = assemble_probabilities(
                label_data.labels,
                label_data.labeled_indices,
                unlabeled_data.index_by_pixel,
                unlabeled_values,
                width,
                height,
                cancellation,
                progress);
            if (std::holds_alternative<domain::Cancelled>(
                    probabilities_outcome)) {
                return domain::Cancelled {};
            }
            probabilities = std::get<domain::ProbabilityMap>(
                std::move(probabilities_outcome));
        }

        CancellableOutcome<domain::BinaryMask> mask_outcome =
            threshold_probabilities(
                probabilities,
                cancellation,
                progress);
        if (std::holds_alternative<domain::Cancelled>(mask_outcome)) {
            return domain::Cancelled {};
        }
        domain::BinaryMask mask =
            std::get<domain::BinaryMask>(std::move(mask_outcome));

        return domain::SegmentationResult {
            .probabilities = std::move(probabilities),
            .mask = std::move(mask)
        };
    }
}
