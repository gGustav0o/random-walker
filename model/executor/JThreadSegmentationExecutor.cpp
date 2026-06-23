#include "JThreadSegmentationExecutor.hpp"

#include <chrono>
#include <optional>
#include <utility>

namespace random_walker::executor {
    namespace {
        class ProgressThrottle final {
        public:
            [[nodiscard]] bool should_emit(
                const domain::SegmentationProgress& progress) {
                const auto now = Clock::now();
                const bool stage_changed =
                    !last_stage_.has_value()
                    || *last_stage_ != progress.stage;
                const bool mode_changed =
                    !last_fraction_state_.has_value()
                    || *last_fraction_state_
                        != progress.fraction.has_value();
                const bool stage_completed =
                    progress.fraction.has_value()
                    && *progress.fraction >= 1.0
                    && (!last_fraction_.has_value()
                        || *last_fraction_ < 1.0);
                const bool interval_elapsed =
                    !last_emission_.has_value()
                    || now - *last_emission_ >= kMinimumInterval;

                if (!stage_changed
                    && !mode_changed
                    && !stage_completed
                    && !interval_elapsed) {
                    return false;
                }

                last_stage_ = progress.stage;
                last_fraction_state_ = progress.fraction.has_value();
                last_fraction_ = progress.fraction;
                last_emission_ = now;
                return true;
            }

        private:
            using Clock = std::chrono::steady_clock;
            static constexpr auto kMinimumInterval =
                std::chrono::milliseconds(33);

            std::optional<domain::SegmentationStage> last_stage_;
            std::optional<bool> last_fraction_state_;
            std::optional<double> last_fraction_;
            std::optional<Clock::time_point> last_emission_;
        };
    }

    JThreadSegmentationExecutor::JThreadSegmentationExecutor()
        : worker_([this](std::stop_token thread_stop) {
            run(thread_stop);
        }) {
    }

    JThreadSegmentationExecutor::~JThreadSegmentationExecutor() {
        cancel();
        worker_.request_stop();
        task_available_.notify_all();
    }

    void JThreadSegmentationExecutor::submit(
        domain::SegmentationRequest request
        , SegmentationProgressHandler progress_handler
        , SegmentationCompletionHandler completion_handler
    ) {
        const domain::SegmentationRequestId request_id =
            request.request_id();
        auto task = std::make_unique<Task>(Task {
            .request = std::move(request)
            , .progress_handler = std::move(progress_handler)
            , .completion_handler = std::move(completion_handler)
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

    void JThreadSegmentationExecutor::cancel() noexcept {
        std::lock_guard lock(mutex_);

        latest_request_id_.reset();
        pending_task_.reset();

        if (active_stop_source_.has_value()) {
            active_stop_source_->request_stop();
        }
    }

    void JThreadSegmentationExecutor::run(std::stop_token thread_stop) {
        while (!thread_stop.stop_requested()) {
            std::unique_ptr<Task> task;
            std::stop_source task_stop_source;

            {
                std::unique_lock lock(mutex_);
                task_available_.wait(
                    lock
                    , thread_stop
                    , [this] {
                        return pending_task_ != nullptr;
                    });

                if (thread_stop.stop_requested()) {
                    return;
                }

                task = std::move(pending_task_);
                task_stop_source = std::stop_source {};
                active_stop_source_ = task_stop_source;
            }

            const domain::SegmentationRequestId request_id =
                task->request.request_id();
            ProgressThrottle progress_throttle;
            const domain::ProgressReporter progress_reporter(
                request_id
                , [this
                 , request_id
                 , &progress_throttle
                 , &progress_handler = task->progress_handler](
                    domain::SegmentationProgress progress) {
                    if (!progress_throttle.should_emit(progress)) {
                        return;
                    }

                    SegmentationProgressHandler current_handler;
                    {
                        std::lock_guard lock(mutex_);
                        const bool is_latest =
                            latest_request_id_.has_value()
                            && *latest_request_id_ == request_id;
                        if (is_latest) {
                            current_handler = progress_handler;
                        }
                    }

                    if (current_handler) {
                        current_handler(std::move(progress));
                    }
                });

            domain::SegmentationOutcome outcome =
                segmentation_service_.segment(
                    task->request
                    , domain::CancellationToken(
                        task_stop_source.get_token())
                    , progress_reporter
                );

            SegmentationCompletionHandler completion_handler;
            SegmentationCompletion completion {
                .request_id = task->request.request_id()
                , .outcome = std::move(outcome)
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
