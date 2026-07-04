#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "application/markers/AutoMarkerService.hpp"
#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/Seed.hpp"

namespace random_walker::application {
    using AutoMarkerRequestId = std::uint64_t;

    class AutoMarkerRequest final {
    public:
        AutoMarkerRequest(
            AutoMarkerRequestId request_id
            , domain::GrayImage image
            , domain::AutoMarkerParameters parameters
            , std::vector<domain::SeedRegion> manual_seed_regions
        )
            : request_id_(request_id)
            , image_(std::move(image))
            , parameters_(parameters)
            , manual_seed_regions_(std::move(manual_seed_regions)) {}

        AutoMarkerRequest(const AutoMarkerRequest&) = default;
        AutoMarkerRequest(AutoMarkerRequest&&) noexcept = default;
        AutoMarkerRequest& operator=(const AutoMarkerRequest&) = delete;
        AutoMarkerRequest& operator=(AutoMarkerRequest&&) = delete;

        [[nodiscard]] AutoMarkerRequestId request_id() const noexcept {
            return request_id_;
        }

        [[nodiscard]] const domain::GrayImage& image() const noexcept {
            return image_;
        }

        [[nodiscard]] const domain::AutoMarkerParameters& parameters()
            const noexcept {
            return parameters_;
        }

        [[nodiscard]] std::span<const domain::SeedRegion>
        manual_seed_regions() const noexcept {
            return manual_seed_regions_;
        }

    private:
        AutoMarkerRequestId request_id_;
        domain::GrayImage image_;
        domain::AutoMarkerParameters parameters_;
        std::vector<domain::SeedRegion> manual_seed_regions_;
    };

    struct AutoMarkerCompletion {
        AutoMarkerRequestId request_id;
        AutoMarkerProposalOutcome outcome;
    };

    using AutoMarkerCompletionHandler =
        std::function<void(AutoMarkerCompletion)>;

    class AutoMarkerExecutor {
    public:
        virtual ~AutoMarkerExecutor() = default;

        // The handler is invoked on the executor's worker thread.
        virtual void submit(
            AutoMarkerRequest request
            , AutoMarkerCompletionHandler completion_handler
        ) = 0;
        virtual void cancel() noexcept = 0;
    };
}