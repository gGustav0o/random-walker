#pragma once

#include "application/settings/ApplicationSettings.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/Segmentation.hpp"

#include <vector>

namespace random_walker::application {

    [[nodiscard]] domain::SegmentationRequest make_segmentation_request(
        domain::SegmentationRequestId request_id
        , const domain::GrayImage& image
        , std::vector<domain::SeedRegion> seed_regions
        , const ApplicationSettings& settings
    );
}
