#pragma once

#include <unordered_map>
#include <variant>
#include <vector>

#include "BoundaryConditions.hpp"
#include "IndexTypes.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"

namespace random_walker::algorithm {
    struct NodePartition {
        std::vector<PixelIndex> unknown_pixels;
        std::vector<PixelIndex> boundary_pixels;
        std::unordered_map<PixelIndex, UnknownIndex, PixelIndexHash>
            unknown_index_by_pixel;
        std::unordered_map<PixelIndex, BoundaryIndex, PixelIndexHash>
            boundary_index_by_pixel;
    };

    using NodePartitionOutcome =
        std::variant<NodePartition, domain::Cancelled>;

    [[nodiscard]] NodePartitionOutcome partition_nodes(
        int pixel_count
        , const BoundaryConditions& boundary_conditions
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}
