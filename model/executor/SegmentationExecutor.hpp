#pragma once

#include <functional>
#include <variant>

#include "model/domain/Segmentation.hpp"

namespace random_walker::executor {
    enum class ExecutionError {
        UnexpectedInternalFailure
    };

    using SegmentationCompletionOutcome = std::variant<
        domain::SegmentationOutcome
        , ExecutionError
    >;

    struct SegmentationCompletion {
        domain::SegmentationRequestId request_id;
        SegmentationCompletionOutcome outcome;
    };

    using SegmentationCompletionHandler =
        std::function<void(SegmentationCompletion)>;
    using SegmentationProgressHandler =
        std::function<void(domain::SegmentationProgress)>;

    class SegmentationExecutor {
    public:
        virtual ~SegmentationExecutor() = default;

        // The handler is invoked on the executor's worker thread.
        virtual void submit(
            domain::SegmentationRequest request
            , SegmentationProgressHandler progress_handler
            , SegmentationCompletionHandler completion_handler
        ) = 0;
        virtual void cancel() noexcept = 0;
    };
}
