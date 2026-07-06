#include "SegmentationUseCase.hpp"

#include <cassert>
#include <utility>

namespace random_walker::application {

    domain::SegmentationRequest make_segmentation_request(
        domain::SegmentationRequestId request_id
        , const domain::GrayImage& image
        , std::vector<domain::SeedRegion> seed_regions
        , const ApplicationSettings& settings
    ) {
        assert(domain::is_valid(settings.random_walker));

        return domain::SegmentationRequest(
            request_id
            , image
            , std::move(seed_regions)
            , settings.random_walker
        );
    }
}
