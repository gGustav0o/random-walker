#include "DirichletProblemValidation.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <utility>
#include <variant>
#include <vector>

#include "IterationPolicy.hpp"

namespace random_walker::algorithm {
    namespace {
        using BoundaryAdjacencyOutcome =
            std::variant<std::vector<std::uint8_t>, domain::Cancelled>;
        using UnknownAdjacencyOutcome =
            std::variant<std::vector<std::vector<int>>, domain::Cancelled>;

        [[nodiscard]] bool is_positive_conductance(
            double laplacian_entry_value
        ) noexcept {
            const double conductance = -laplacian_entry_value;
            return std::isfinite(conductance) && conductance > 0.0;
        }

        void assert_dirichlet_validation_preconditions(
            const PartitionedLaplacian& laplacian
        ) {
            assert(
                laplacian.unknown_unknown_block.rows()
                == laplacian.unknown_unknown_block.cols()
            );
            assert(
                laplacian.unknown_boundary_block.rows()
                == laplacian.unknown_unknown_block.rows()
            );
        }

        [[nodiscard]] BoundaryAdjacencyOutcome unknowns_adjacent_to_boundary(
            const Eigen::SparseMatrix<double>& unknown_boundary_block
            , const domain::CancellationToken& cancellation
        ) {
            std::vector<std::uint8_t> result(
                static_cast<std::size_t>(unknown_boundary_block.rows())
                , std::uint8_t {0}
            );

            for (int outer = 0; outer < unknown_boundary_block.outerSize(); ++outer) {
                if (should_poll_cancellation(static_cast<std::size_t>(outer))
                    && cancellation.stop_requested()
                ) {
                    return domain::Cancelled {};
                }

                for (
                    Eigen::SparseMatrix<double>::InnerIterator entry(
                        unknown_boundary_block
                        , outer
                    );
                    entry;
                    ++entry
                ) {
                    if (!is_positive_conductance(entry.value())) {
                        continue;
                    }

                    assert(entry.row() >= 0);
                    assert(entry.row() < unknown_boundary_block.rows());
                    result[static_cast<std::size_t>(entry.row())] =
                        std::uint8_t {1};
                }
            }

            return result;
        }

        [[nodiscard]] UnknownAdjacencyOutcome unknown_adjacency(
            const Eigen::SparseMatrix<double>& unknown_unknown_block
            , const domain::CancellationToken& cancellation
        ) {
            const int unknown_count =
                static_cast<int>(unknown_unknown_block.rows());
            std::vector<std::vector<int>> result(
                static_cast<std::size_t>(unknown_count)
            );

            for (int outer = 0; outer < unknown_unknown_block.outerSize(); ++outer) {
                if (should_poll_cancellation(static_cast<std::size_t>(outer))
                    && cancellation.stop_requested()
                ) {
                    return domain::Cancelled {};
                }

                for (
                    Eigen::SparseMatrix<double>::InnerIterator entry(
                        unknown_unknown_block
                        , outer
                    );
                    entry;
                    ++entry
                ) {
                    if (entry.row() == entry.col()) {
                        continue;
                    }
                    if (!is_positive_conductance(entry.value())) {
                        continue;
                    }

                    assert(entry.row() >= 0);
                    assert(entry.row() < unknown_count);
                    assert(entry.col() >= 0);
                    assert(entry.col() < unknown_count);
                    result[static_cast<std::size_t>(entry.row())].push_back(
                        static_cast<int>(entry.col())
                    );
                }
            }

            return result;
        }

        [[nodiscard]] DirichletAnchoringOutcome mark_boundary_reachable_unknowns(
            std::vector<std::uint8_t>& visited
            , const std::vector<std::uint8_t>& boundary_adjacent
            , const std::vector<std::vector<int>>& adjacency
            , const domain::CancellationToken& cancellation
        ) {
            assert(visited.size() == boundary_adjacent.size());
            assert(visited.size() == adjacency.size());

            std::deque<int> queue;
            for (std::size_t index = 0; index < boundary_adjacent.size(); ++index) {
                if (should_poll_cancellation(index)
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                if (boundary_adjacent[index] == std::uint8_t {0}) {
                    continue;
                }

                visited[index] = std::uint8_t {1};
                queue.push_back(static_cast<int>(index));
            }

            while (!queue.empty()) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                const int current = queue.front();
                queue.pop_front();
                assert(current >= 0);
                assert(static_cast<std::size_t>(current) < adjacency.size());

                for (const int next : adjacency[static_cast<std::size_t>(current)]) {
                    assert(next >= 0);
                    assert(static_cast<std::size_t>(next) < visited.size());
                    if (visited[static_cast<std::size_t>(next)]
                        != std::uint8_t {0}
                    ) {
                        continue;
                    }

                    visited[static_cast<std::size_t>(next)] = std::uint8_t {1};
                    queue.push_back(next);
                }
            }

            return DirichletAnchoringValidated {};
        }

        [[nodiscard]] bool all_unknowns_are_boundary_reachable(
            const std::vector<std::uint8_t>& visited
        ) noexcept {
            for (const std::uint8_t value : visited) {
                if (value == std::uint8_t {0}) {
                    return false;
                }
            }

            return true;
        }
    }

    DirichletAnchoringOutcome validate_dirichlet_anchoring(
        const PartitionedLaplacian& laplacian
        , const domain::CancellationToken& cancellation
    ) {
        assert_dirichlet_validation_preconditions(laplacian);

        const int unknown_count =
            static_cast<int>(laplacian.unknown_unknown_block.rows());
        if (unknown_count == 0) {
            return DirichletAnchoringValidated {};
        }
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        BoundaryAdjacencyOutcome boundary_adjacent_outcome =
            unknowns_adjacent_to_boundary(
                laplacian.unknown_boundary_block
                , cancellation
            );
        if (std::holds_alternative<domain::Cancelled>(
                boundary_adjacent_outcome
            )
        ) {
            return domain::Cancelled {};
        }
        std::vector<std::uint8_t> boundary_adjacent =
            std::get<std::vector<std::uint8_t>>(
                std::move(boundary_adjacent_outcome)
            );

        UnknownAdjacencyOutcome adjacency_outcome =
            unknown_adjacency(laplacian.unknown_unknown_block, cancellation);
        if (std::holds_alternative<domain::Cancelled>(adjacency_outcome)) {
            return domain::Cancelled {};
        }
        std::vector<std::vector<int>> adjacency =
            std::get<std::vector<std::vector<int>>>(
                std::move(adjacency_outcome)
            );

        std::vector<std::uint8_t> visited(
            static_cast<std::size_t>(unknown_count)
            , std::uint8_t {0}
        );
        const DirichletAnchoringOutcome marking_outcome =
            mark_boundary_reachable_unknowns(
                visited
                , boundary_adjacent
                , adjacency
                , cancellation
            );
        if (std::holds_alternative<domain::Cancelled>(marking_outcome)) {
            return domain::Cancelled {};
        }

        if (!all_unknowns_are_boundary_reachable(visited)) {
            return domain::SegmentationError::UnanchoredUnknownRegion;
        }

        return DirichletAnchoringValidated {};
    }
}
