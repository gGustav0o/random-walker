#include "JThreadSegmentationExecutor.hpp"

#include <utility>

namespace random_walker::executor
{
    JThreadSegmentationExecutor::JThreadSegmentationExecutor()
        : worker_([this](std::stop_token thread_stop) {
            run(thread_stop);
        })
    {
    }

    JThreadSegmentationExecutor::~JThreadSegmentationExecutor()
    {
        cancel();
        worker_.request_stop();
        task_available_.notify_all();
    }

    void JThreadSegmentationExecutor::submit(
        domain::SegmentationRequest request,
        SegmentationCompletionHandler completion_handler)
    {
        const domain::SegmentationRequestId request_id =
            request.request_id();
        auto task = std::make_unique<Task>(Task {
            .request = std::move(request),
            .completion_handler = std::move(completion_handler)
        });

        {
            std::lock_guard lock(mutex_);

            if (active_stop_source_.has_value()) {
                active_stop_source_->request_stop();
            }

            latest_request_id_ = request_id;
            pending_task_ = std::move(task);
        }

        task_available_.notify_one();
    }

    void JThreadSegmentationExecutor::cancel() noexcept
    {
        std::lock_guard lock(mutex_);

        latest_request_id_.reset();
        pending_task_.reset();

        if (active_stop_source_.has_value()) {
            active_stop_source_->request_stop();
        }
    }

    void JThreadSegmentationExecutor::run(std::stop_token thread_stop)
    {
        while (!thread_stop.stop_requested()) {
            std::unique_ptr<Task> task;
            std::stop_source task_stop_source;

            {
                std::unique_lock lock(mutex_);
                task_available_.wait(
                    lock,
                    thread_stop,
                    [this] {
                        return pending_task_ != nullptr;
                    });

                if (thread_stop.stop_requested()) {
                    return;
                }

                task = std::move(pending_task_);
                task_stop_source = std::stop_source {};
                active_stop_source_ = task_stop_source;
            }

            domain::SegmentationOutcome outcome =
                segmentation_service_.segment(
                    task->request,
                    domain::CancellationToken(
                        task_stop_source.get_token()));

            SegmentationCompletionHandler completion_handler;
            SegmentationCompletion completion {
                .request_id = task->request.request_id(),
                .outcome = std::move(outcome)
            };

            {
                std::lock_guard lock(mutex_);
                active_stop_source_.reset();

                const bool is_latest =
                    latest_request_id_.has_value()
                    && *latest_request_id_ == completion.request_id;
                if (is_latest && !thread_stop.stop_requested()) {
                    completion_handler =
                        std::move(task->completion_handler);
                    latest_request_id_.reset();
                }
            }

            if (completion_handler) {
                completion_handler(std::move(completion));
            }
        }
    }
}
