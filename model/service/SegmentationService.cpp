#include "SegmentationService.hpp"

#include <cassert>
#include <optional>
#include <variant>
#include <vector>

#include "model/algorithm/RandomWalkerAlgorithm.hpp"
#include "model/algorithm/SeedExpansion.hpp"
#include "model/domain/SegmentationConstraintsValidation.hpp"
#include "model/domain/SegmentationLimits.hpp"

namespace random_walker::service {
    namespace {
        [[nodiscard]]
        domain::SegmentationError
        segmentation_error_for(
            domain::RandomWalkerParameterError error
        ) noexcept {
            using Error = domain::RandomWalkerParameterError;

            switch (error) {
            case Error::InvalidBeta:
                return domain::SegmentationError::InvalidBeta;
            case Error::InvalidDistancePower:
                return domain::SegmentationError::InvalidDistancePower;
            case Error::InvalidConnectivity:
                return domain::SegmentationError::InvalidConnectivity;
            }

            assert(false);
            return domain::SegmentationError::InvalidBeta;
        }
    }

    std::optional<domain::SegmentationError> SegmentationService::validate(
        const domain::SegmentationRequest& request) {
        if (request.image().empty()) {
            return domain::SegmentationError::EmptyImage;
        }

        if (!domain::is_supported_segmentation_image_geometry(
                request.image().width()
                , request.image().height()
            )
        ) {
            return domain::SegmentationError::ImageTooLarge;
        }

        if (const auto error = domain::validate(request.parameters());
            error.has_value()
        ) {
            return segmentation_error_for(*error);
        }

        if (const auto error = domain::validate(
                request.constraints()
                , request.image()
            ); error.has_value()
        ) {
            return *error;
        }

        return std::nullopt;
    }

    domain::SegmentationOutcome SegmentationService::segment(
        const domain::SegmentationRequest& request
        , domain::CancellationToken cancellation
        , const domain::ProgressReporter& progress
    ) const {
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report(domain::SegmentationStage::ValidatingInput, 0.0);
        if (const auto error = validate(request); error.has_value()) {
            return *error;
        }
        progress.report(domain::SegmentationStage::ValidatingInput, 1.0);

        algorithm::SeedExpansionOutcome expansion =
            algorithm::expand_seed_constraints(
                request.constraints()
                , request.image().width()
                , request.image().height()
                , cancellation
                , progress
            );

        if (std::holds_alternative<domain::Cancelled>(expansion)) {
            return domain::Cancelled {};
        }
        const std::vector<domain::Seed>& seeds =
            std::get<std::vector<domain::Seed>>(expansion);

        const domain::SegmentationInput input {
            request.image()
            , seeds
        };


        return algorithm::run_random_walker(
            input
            , request.parameters()
            , cancellation
            , progress
        );
    }
}
