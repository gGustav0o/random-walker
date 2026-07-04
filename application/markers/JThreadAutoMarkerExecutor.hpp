#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "application/markers/AutoMarkerExecutor.hpp"
#include "application/markers/AutoMarkerService.hpp"

namespace random_walker::application {
    class JThreadAutoMarkerExecutor final : public AutoMarkerExecutor {
    public:
        JThreadAutoMarkerExecutor();
        ~JThreadAutoMarkerExecutor() override;

        JThreadAutoMarkerExecutor(const JThreadAutoMarkerExecutor&) = delete;
        JThreadAutoMarkerExecutor& operator=(
            const JThreadAutoMarkerExecutor&
        ) = delete;

        void submit(
            AutoMarkerRequest request
            , AutoMarkerCompletionHandler completion_handler
        ) override;
        void cancel() noexcept override;

    private:
        struct Task {
            AutoMarkerRequest request;
            AutoMarkerCompletionHandler completion_handler;
        };

        void run(std::stop_token thread_stop);

        AutoMarkerService service_;
        std::mutex mutex_;
        std::condition_variable_any task_available_;
        std::unique_ptr<Task> pending_task_;
        std::optional<AutoMarkerRequestId> latest_request_id_;
        std::jthread worker_;
    };
}
