#pragma once

#include <variant>

#include "model/domain/Segmentation.hpp"

namespace random_walker::application {
    enum class ApplicationError {
        UnexpectedInternalFailure
    };

    enum class ImageLoadError {
        Failed
    };

    enum class SettingsError {
        InvalidSettings
        , SaveFailed
    };

    using UserError = std::variant<
        ApplicationError
        , ImageLoadError
        , SettingsError
        , domain::SegmentationError
    >;
}
