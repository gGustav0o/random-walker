#include "PartitionedLaplacian.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

namespace random_walker::algorithm {
    namespace {
        struct PartitionTriplets {
            std::vector<Eigen::Triplet<double>> unknown_unknown;
            std::vector<Eigen::Triplet<double>> unknown_boundary;
        };

        void assert_partition_laplacian_preconditions(
            const Eigen::SparseMatrix<double>& laplacian
            , const NodePartition& node_partition
        ) {
            assert(laplacian.rows() == laplacian.cols());
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
                == static_cast<std::size_t>(laplacian.rows())
            );
        }

        [[nodiscard]] PartitionedLaplacian make_empty_partitioned_laplacian(
            const NodePartition& node_partition
        ) {
            const auto unknown_count = static_cast<int>(
                node_partition.unknown_index_by_pixel.size()
            );
            const auto boundary_count = static_cast<int>(
                node_partition.boundary_index_by_pixel.size()
            );

            return PartitionedLaplacian {
                .unknown_unknown_block = Eigen::SparseMatrix<double>(
                    unknown_count
                    , unknown_count
                )
                , .unknown_boundary_block = Eigen::SparseMatrix<double>(
                    unknown_count
                    , boundary_count
                )
            };
        }

        [[nodiscard]] double partition_laplacian_progress(
            int outer
            , int outer_size
        ) noexcept {
            assert(outer >= 0);
            assert(outer_size > 0);
            return 0.4 + 0.45 * static_cast<double>(outer + 1)
                / static_cast<double>(outer_size);
        }

        void add_partitioned_entry(
            PartitionTriplets& triplets
            , const NodePartition& node_partition
            , const PartitionedLaplacian& target_shape
            , PixelIndex row_pixel
            , PixelIndex column_pixel
            , double value
        ) {
            const auto row =
                node_partition.unknown_index_by_pixel.find(row_pixel);
            if (row == node_partition.unknown_index_by_pixel.end()) {
                return;
            }

            assert(row->second.value >= 0);
            assert(
                static_cast<Eigen::Index>(row->second.value)
                < target_shape.unknown_unknown_block.rows()
            );

            if (
                const auto column =
                    node_partition.unknown_index_by_pixel.find(column_pixel);
                column != node_partition.unknown_index_by_pixel.end()
            ) {
                assert(column->second.value >= 0);
                assert(
                    static_cast<Eigen::Index>(column->second.value)
                    < target_shape.unknown_unknown_block.cols()
                );
                triplets.unknown_unknown.emplace_back(
                    row->second.value
                    , column->second.value
                    , value
                );
                return;
            }

            if (
                const auto column =
                    node_partition.boundary_index_by_pixel.find(column_pixel);
                column != node_partition.boundary_index_by_pixel.end()
            ) {
                assert(column->second.value >= 0);
                assert(
                    static_cast<Eigen::Index>(column->second.value)
                    < target_shape.unknown_boundary_block.cols()
                );
                triplets.unknown_boundary.emplace_back(
                    row->second.value
                    , column->second.value
                    , value
                );
            }
        }
    }

    PartitionedLaplacianOutcome partition_laplacian(
        const Eigen::SparseMatrix<double>& laplacian
        , const NodePartition& node_partition
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert_partition_laplacian_preconditions(laplacian, node_partition);

        PartitionedLaplacian result =
            make_empty_partitioned_laplacian(node_partition);
        PartitionTriplets triplets;

        for (int outer = 0; outer < laplacian.outerSize(); ++outer) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            for (
                Eigen::SparseMatrix<double>::InnerIterator entry(laplacian, outer);
                 entry;
                 ++entry
            ) {
                const PixelIndex row_pixel {
                    .value = static_cast<int>(entry.row())
                };
                const PixelIndex column_pixel {
                    .value = static_cast<int>(entry.col())
                };

                assert(row_pixel.value >= 0);
                assert(static_cast<Eigen::Index>(row_pixel.value) < laplacian.rows());
                assert(column_pixel.value >= 0);
                assert(static_cast<Eigen::Index>(column_pixel.value) < laplacian.cols());

                add_partitioned_entry(
                    triplets
                    , node_partition
                    , result
                    , row_pixel
                    , column_pixel
                    , entry.value()
                );
            }

            progress.report(
                domain::SegmentationStage::PartitioningSystem
                , partition_laplacian_progress(outer, laplacian.outerSize())
            );
        }

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        result.unknown_unknown_block.setFromTriplets(
            triplets.unknown_unknown.begin()
            , triplets.unknown_unknown.end()
        );
        result.unknown_boundary_block.setFromTriplets(
            triplets.unknown_boundary.begin()
            , triplets.unknown_boundary.end()
        );
        progress.report(
            domain::SegmentationStage::PartitioningSystem
            , 0.85
        );
        return result;
    }
}
