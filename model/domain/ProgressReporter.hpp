#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

namespace random_walker::domain {
    using SegmentationRequestId = std::uint64_t;

    enum class SegmentationStage {
        ValidatingInput
        , ExpandingSeeds
        , BuildingGraph
        , BuildingBoundaryConditions
        , PartitioningSystem
        , Factorizing
        , Solving
        , AssemblingProbabilities
        , Thresholding
    };

    struct SegmentationProgress {
        SegmentationRequestId request_id;
        SegmentationStage stage;
        // Local progress within the stage. nullopt means indeterminate.
        std::optional<double> fraction;
    };

    class ProgressReporter final {
    public:
        using Handler = std::function<void(SegmentationProgress)>;

        ProgressReporter() = default;

        ProgressReporter(
            SegmentationRequestId request_id
            , Handler handler
        )
            : request_id_(request_id)
            , handler_(std::move(handler)) {
        }

        void report(
            SegmentationStage stage
            , double fraction) const {
            if (!handler_) {
                return;
            }

            handler_({
                .request_id = request_id_
                , .stage = stage
                , .fraction = std::clamp(fraction, 0.0, 1.0)
            });
        }

        void report_indeterminate(SegmentationStage stage) const {
            if (!handler_) {
                return;
            }

            handler_({
                .request_id = request_id_
                , .stage = stage
                , .fraction = std::nullopt
            });
        }

    private:
        SegmentationRequestId request_id_ = 0;
        Handler handler_;
    };
}
