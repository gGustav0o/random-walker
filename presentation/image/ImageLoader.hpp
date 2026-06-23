#pragma once

#include <variant>

#include <QImage>
#include <QString>

#include "application/error/UserError.hpp"

namespace random_walker::presentation {
    using ImageLoadOutcome = std::variant<
        QImage
        , application::ImageLoadError
    >;

    [[nodiscard]] ImageLoadOutcome load_image(const QString& path);
}
