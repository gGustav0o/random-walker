#pragma once

#include "application/settings/ApplicationSettings.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::application {

    [[nodiscard]] domain::SegmentationRequest make_segmentation_request(
        domain::SegmentationRequestId request_id
        , const domain::GrayImage& image
        , domain::SegmentationConstraints constraints
        , const ApplicationSettings& settings
    );
}
