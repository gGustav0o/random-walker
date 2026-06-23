#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

#include "model/executor/SegmentationExecutor.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/service/SegmentationService.hpp"

namespace random_walker::executor {
    class JThreadSegmentationExecutor final : public SegmentationExecutor {
    public:
        JThreadSegmentationExecutor();
        ~JThreadSegmentationExecutor() override;

        JThreadSegmentationExecutor(const JThreadSegmentationExecutor&) = delete;
        JThreadSegmentationExecutor& operator=(
            const JThreadSegmentationExecutor&) = delete;

        void submit(
            domain::SegmentationRequest request
            , SegmentationProgressHandler progress_handler
            , SegmentationCompletionHandler completion_handler
        ) override;
        void cancel() noexcept override;

    private:
        struct Task {
            domain::SegmentationRequest request;
            SegmentationProgressHandler progress_handler;
            SegmentationCompletionHandler completion_handler;
        };

        void run(std::stop_token thread_stop);

        service::SegmentationService segmentation_service_;
        std::mutex mutex_;
        std::condition_variable_any task_available_;
        std::unique_ptr<Task> pending_task_;
        std::optional<std::stop_source> active_stop_source_;
        std::optional<domain::SegmentationRequestId> latest_request_id_;
        std::jthread worker_;
    };
}
