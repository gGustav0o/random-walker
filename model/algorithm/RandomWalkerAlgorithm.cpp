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

        using SolveOutcome = std::variant<VectorXd, domain::SegmentationError>;

        [[nodiscard]] constexpr int flatten(int row, int column, int width) noexcept
        {
            return row * width + column;
        }

        [[nodiscard]] LabelData build_labels(const domain::SegmentationInput& input)
        {
            const int height = input.image.height();
            const int width = input.image.width();

            LabelData result {
                .labels = MatrixXi::Zero(height, width),
                .labeled_indices = {}
            };

            const auto apply_seeds = [&](domain::SeedLabel label, int value) {
                for (const domain::Seed& seed : input.seeds) {
                    if (seed.label != label) {
                        continue;
                    }

                    result.labels(seed.position.y, seed.position.x) = value;
                    result.labeled_indices.insert(
                        flatten(seed.position.y, seed.position.x, width));
                }
            };

            apply_seeds(domain::SeedLabel::Background, 0);
            apply_seeds(domain::SeedLabel::Object, 1);
            return result;
        }

        [[nodiscard]] UnlabeledData extract_unlabeled_indices(
            int pixel_count,
            const std::unordered_set<int>& labeled_indices)
        {
            UnlabeledData result;
            result.indices.reserve(
                static_cast<std::size_t>(pixel_count) - labeled_indices.size());

            for (int index = 0; index < pixel_count; ++index) {
                if (labeled_indices.contains(index)) {
                    continue;
                }

                const int unlabeled_index = static_cast<int>(result.indices.size());
                result.indices.push_back(index);
                result.index_by_pixel.emplace(index, unlabeled_index);
            }

            return result;
        }

        [[nodiscard]] std::unordered_map<int, int> index_by_pixel(
            const std::vector<int>& pixel_indices)
        {
            std::unordered_map<int, int> result;
            result.reserve(pixel_indices.size());

            for (std::size_t index = 0; index < pixel_indices.size(); ++index) {
                result.emplace(pixel_indices[index], static_cast<int>(index));
            }

            return result;
        }

        [[nodiscard]] LaplacianBlocks split_laplacian(
            const SparseMatrix<double>& laplacian,
            const std::unordered_map<int, int>& labeled_index,
            const std::unordered_map<int, int>& unlabeled_index)
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
            }

            result.unlabeled_to_unlabeled.setFromTriplets(
                unlabeled_triplets.begin(),
                unlabeled_triplets.end());
            result.unlabeled_to_labeled.setFromTriplets(
                labeled_triplets.begin(),
                labeled_triplets.end());
            return result;
        }

        [[nodiscard]] VectorXd build_boundary_values(
            const MatrixXi& labels,
            const std::vector<int>& labeled_pixels,
            int width)
        {
            VectorXd values(static_cast<int>(labeled_pixels.size()));

            for (std::size_t index = 0; index < labeled_pixels.size(); ++index) {
                const int pixel = labeled_pixels[index];
                values[static_cast<int>(index)] = labels(pixel / width, pixel % width);
            }

            return values;
        }

        [[nodiscard]] SolveOutcome solve_sparse_system(
            const SparseMatrix<double>& unlabeled_to_unlabeled,
            const SparseMatrix<double>& unlabeled_to_labeled,
            const VectorXd& boundary_values)
        {
            Eigen::SimplicialLLT<SparseMatrix<double>> solver;
            solver.compute(unlabeled_to_unlabeled);

            if (solver.info() != Eigen::Success) {
                return domain::SegmentationError::LaplacianDecompositionFailed;
            }

            VectorXd solution = solver.solve(
                -unlabeled_to_labeled * boundary_values);

            if (solver.info() != Eigen::Success) {
                return domain::SegmentationError::LinearSystemSolveFailed;
            }
            if (!solution.allFinite()) {
                return domain::SegmentationError::NonFiniteSolution;
            }

            return solution;
        }

        [[nodiscard]] domain::ProbabilityMap assemble_probabilities(
            const MatrixXi& labels,
            const std::unordered_set<int>& labeled_indices,
            const std::unordered_map<int, int>& unlabeled_index,
            const VectorXd& unlabeled_values,
            int width,
            int height)
        {
            domain::ProbabilityMap result(height, width);
            const int pixel_count = width * height;

            for (int index = 0; index < pixel_count; ++index) {
                const int row = index / width;
                const int column = index % width;

                if (labeled_indices.contains(index)) {
                    result(row, column) = labels(row, column);
                } else {
                    result(row, column) = unlabeled_values[unlabeled_index.at(index)];
                }
            }

            return result;
        }
    }

    domain::SegmentationOutcome run_validated_random_walker(
        const domain::SegmentationInput& input)
    {
        const int width = input.image.width();
        const int height = input.image.height();
        const int pixel_count = width * height;

        const SparseMatrix<double> laplacian =
            graph::build_laplacian(input.image);
        const LabelData label_data = build_labels(input);
        const UnlabeledData unlabeled_data = extract_unlabeled_indices(
            pixel_count,
            label_data.labeled_indices);

        domain::ProbabilityMap probabilities;

        if (unlabeled_data.indices.empty()) {
            probabilities = label_data.labels.cast<double>();
        } else {
            const std::vector<int> labeled_pixels(
                label_data.labeled_indices.begin(),
                label_data.labeled_indices.end());
            const std::unordered_map<int, int> labeled_index =
                index_by_pixel(labeled_pixels);
            const LaplacianBlocks blocks = split_laplacian(
                laplacian,
                labeled_index,
                unlabeled_data.index_by_pixel);
            const VectorXd boundary_values = build_boundary_values(
                label_data.labels,
                labeled_pixels,
                width);
            SolveOutcome solve_outcome = solve_sparse_system(
                blocks.unlabeled_to_unlabeled,
                blocks.unlabeled_to_labeled,
                boundary_values);

            if (const auto* error =
                    std::get_if<domain::SegmentationError>(&solve_outcome)) {
                return *error;
            }

            VectorXd unlabeled_values =
                std::get<VectorXd>(std::move(solve_outcome));
            probabilities = assemble_probabilities(
                label_data.labels,
                label_data.labeled_indices,
                unlabeled_data.index_by_pixel,
                unlabeled_values,
                width,
                height);
        }

        domain::BinaryMask mask =
            (probabilities.array() >= 0.5).cast<std::uint8_t>();

        return domain::SegmentationResult {
            .probabilities = std::move(probabilities),
            .mask = std::move(mask)
        };
    }
}
