#include "SegmentationUseCase.hpp"

#include <cassert>
#include <utility>

namespace random_walker::application {

    domain::SegmentationRequest make_segmentation_request(
        domain::SegmentationRequestId request_id
        , const domain::GrayImage& image
        , domain::SegmentationConstraints constraints
        , const ApplicationSettings& settings
    ) {
        assert(domain::is_valid(settings.random_walker));

        return domain::SegmentationRequest(
            request_id
            , image
            , std::move(constraints)
            , settings.random_walker
        );
    }
}
