#include "SegmentationService.hpp"

#include "model/algorithm/RandomWalkerAlgorithm.hpp"

namespace random_walker::service
{
    namespace
    {
        [[nodiscard]] bool is_inside(
            const domain::PixelCoordinate& position,
            const domain::GrayImage& image) noexcept
        {
            return position.x >= 0
                && position.x < image.width()
                && position.y >= 0
                && position.y < image.height();
        }
    }

    std::optional<domain::SegmentationError> SegmentationService::validate(
        const domain::SegmentationInput& input) noexcept
    {
        if (input.image.empty()) {
            return domain::SegmentationError::EmptyImage;
        }

        bool has_background_seed = false;
        bool has_object_seed = false;

        for (const domain::Seed& seed : input.seeds) {
            if (!is_inside(seed.position, input.image)) {
                return domain::SegmentationError::SeedOutOfBounds;
            }

            has_background_seed =
                has_background_seed || seed.label == domain::SeedLabel::Background;
            has_object_seed =
                has_object_seed || seed.label == domain::SeedLabel::Object;
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
        const domain::SegmentationInput& input) const
    {
        if (const auto error = validate(input); error.has_value()) {
            return *error;
        }

        return algorithm::detail::run_validated_random_walker(input);
    }
}
