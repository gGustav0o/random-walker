#pragma once

#include <QImage>

#include "model/domain/Segmentation.hpp"

namespace random_walker::qt_adapter {
    [[nodiscard]] QImage render_binary_mask(
        const domain::BinaryMask& mask);
}
