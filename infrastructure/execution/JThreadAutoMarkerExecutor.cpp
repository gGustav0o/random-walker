#include "JThreadAutoMarkerExecutor.hpp"

#include <cstdio>
#include <utility>

namespace random_walker::infrastructure {
    JThreadAutoMarkerExecutor::JThreadAutoMarkerExecutor()
        : worker_([this](std::stop_token thread_stop) {
            run(thread_stop);
        }) {
    }

    JThreadAutoMarkerExecutor::~JThreadAutoMarkerExecutor() {
        cancel();
        worker_.request_stop();
        task_available_.notify_all();
    }

    void JThreadAutoMarkerExecutor::submit(
        application::AutoMarkerRequest request
        , application::AutoMarkerCompletionHandler completion_handler
    ) {
        const application::AutoMarkerRequestId request_id =
            request.request_id();
        auto task = std::make_unique<Task>(Task {
            .request = std::move(request)
            , .completion_handler = std::move(completion_handler)
        });

        {
            std::lock_guard lock(mutex_);
            latest_request_id_ = request_id;
            pending_task_ = std::move(task);
        }

        task_available_.notify_one();
    }

    void JThreadAutoMarkerExecutor::cancel() noexcept {
        std::lock_guard lock(mutex_);
        latest_request_id_.reset();
        pending_task_.reset();
    }

    void JThreadAutoMarkerExecutor::run(std::stop_token thread_stop) {
        while (!thread_stop.stop_requested()) {
            std::unique_ptr<Task> task;

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
            }

            application::AutoMarkerProposalOutcome outcome;
            try {
                outcome = service_.propose(
                    task->request.image()
                    , task->request.parameters()
                    , task->request.manual_seed_regions()
                );
            } catch (...) {
                std::fputs(
                    "Unexpected exception during auto-marker execution.\n"
                    , stderr
                );
                outcome = application::AutoMarkerError::ProposalFailed;
            }

            application::AutoMarkerCompletion completion {
                .request_id = task->request.request_id()
                , .outcome = std::move(outcome)
            };
            application::AutoMarkerCompletionHandler completion_handler;

            {
                std::lock_guard lock(mutex_);
                const bool is_latest =
                    latest_request_id_.has_value()
                    && *latest_request_id_ == completion.request_id;

                if (is_latest && !thread_stop.stop_requested()) {
                    completion_handler = std::move(task->completion_handler);
                    latest_request_id_.reset();
                }
            }

            if (completion_handler) {
                try {
                    completion_handler(std::move(completion));
                } catch (...) {
                    std::fputs(
                        "Auto-marker completion handler threw; completion was dropped.\n"
                        , stderr
                    );
                }
            }
        }
    }
}
