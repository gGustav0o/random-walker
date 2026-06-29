#include "PartitionedLaplacian.hpp"

#include <cassert>
#include <vector>

namespace random_walker::algorithm {
    PartitionedLaplacianOutcome partition_laplacian(
        const Eigen::SparseMatrix<double>& laplacian
        , const NodePartition& node_partition
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
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

        PartitionedLaplacian result {
            .unknown_unknown_block = Eigen::SparseMatrix<double>(
                static_cast<int>(node_partition.unknown_index_by_pixel.size())
                , static_cast<int>(node_partition.unknown_index_by_pixel.size())
            )
            , .unknown_boundary_block = Eigen::SparseMatrix<double>(
                static_cast<int>(node_partition.unknown_index_by_pixel.size())
                , static_cast<int>(node_partition.boundary_index_by_pixel.size())
            )
        };

        std::vector<Eigen::Triplet<double>> unknown_unknown_block_triplets;
        std::vector<Eigen::Triplet<double>> unknown_boundary_block_triplets;

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

                const auto row =
                    node_partition.unknown_index_by_pixel.find(row_pixel);

                if (row == node_partition.unknown_index_by_pixel.end()) {
                    continue;
                }

                assert(row->second.value >= 0);
                assert(
                    static_cast<Eigen::Index>(row->second.value)
                    < result.unknown_unknown_block.rows()
                );
                if (
                    const auto column =
                        node_partition.unknown_index_by_pixel.find(column_pixel);
                    column != node_partition.unknown_index_by_pixel.end()
                ) {
                    assert(column->second.value >= 0);
                    assert(
                        static_cast<Eigen::Index>(column->second.value)
                        < result.unknown_unknown_block.cols()
                    );
                    unknown_unknown_block_triplets.emplace_back(
                        row->second.value
                        , column->second.value
                        , entry.value()
                    );
                } else if (
                    const auto column =
                        node_partition.boundary_index_by_pixel.find(column_pixel);
                    column != node_partition.boundary_index_by_pixel.end()
                ) {
                    assert(column->second.value >= 0);
                    assert(
                        static_cast<Eigen::Index>(column->second.value)
                        < result.unknown_boundary_block.cols()
                    );
                    unknown_boundary_block_triplets.emplace_back(
                        row->second.value
                        , column->second.value
                        , entry.value()
                    );
                }
            }

            progress.report(
                domain::SegmentationStage::PartitioningSystem
                , 0.4 + 0.45 * static_cast<double>(outer + 1)
                    / static_cast<double>(laplacian.outerSize())
            );
        }

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        result.unknown_unknown_block.setFromTriplets(
            unknown_unknown_block_triplets.begin()
            , unknown_unknown_block_triplets.end()
        );
        result.unknown_boundary_block.setFromTriplets(
            unknown_boundary_block_triplets.begin()
            , unknown_boundary_block_triplets.end()
        );
        progress.report(
            domain::SegmentationStage::PartitioningSystem
            , 0.85
        );
        return result;
    }
}
