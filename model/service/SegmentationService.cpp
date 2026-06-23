#include "SegmentationService.hpp"

#include <cmath>
#include <variant>

#include "model/algorithm/RandomWalkerAlgorithm.hpp"

namespace random_walker::service
{
    std::optional<domain::SegmentationError> SegmentationService::validate(
        const domain::SegmentationRequest& request) noexcept
    {
        if (request.image().empty()) {
            return domain::SegmentationError::EmptyImage;
        }

        const double beta = request.parameters().beta;
        if (!std::isfinite(beta) || beta <= 0.0) {
            return domain::SegmentationError::InvalidBeta;
        }

        bool has_background_seed = false;
        bool has_object_seed = false;

        for (const domain::SeedRegion& region : request.seed_regions()) {
            const domain::PixelRectangle& area = region.area;
            if (area.width <= 0
                || area.height <= 0
                || area.x < 0
                || area.y < 0
                || area.x > request.image().width() - area.width
                || area.y > request.image().height() - area.height) {
                return domain::SegmentationError::SeedOutOfBounds;
            }

            has_background_seed =
                has_background_seed
                || region.label == domain::SeedLabel::Background;
            has_object_seed =
                has_object_seed
                || region.label == domain::SeedLabel::Object;
        }

        if (!has_background_seed) {
            return domain::SegmentationError::MissingBackgroundSeeds;
        }
        if (!has_object_seed) {
            return domain::SegmentationError::MissingObjectSeeds;
        }

        return std::nullopt;
    }

    domain::SegmentationOutcome SegmentationService::segment(
        const domain::SegmentationRequest& request,
        domain::CancellationToken cancellation,
        const domain::ProgressReporter& progress) const
    {
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report(domain::SegmentationStage::ValidatingInput, 0.0);
        if (const auto error = validate(request); error.has_value()) {
            return *error;
        }
        progress.report(domain::SegmentationStage::ValidatingInput, 1.0);

        domain::SeedExpansionOutcome expansion =
            domain::expand_seed_regions(
                request.seed_regions(),
                cancellation,
                progress);
        if (std::holds_alternative<domain::Cancelled>(expansion)) {
            return domain::Cancelled {};
        }
        const std::vector<domain::Seed>& seeds =
            std::get<std::vector<domain::Seed>>(expansion);
        const domain::SegmentationInput input {
            request.image(),
            seeds
        };

        return algorithm::detail::run_validated_random_walker(
            input,
            request.parameters().beta,
            cancellation,
            progress);
    }
}
