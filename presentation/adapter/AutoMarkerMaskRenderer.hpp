#pragma once

#include <QImage>

#include "model/domain/AutoMarkers.hpp"

namespace random_walker::qt_adapter {
    [[nodiscard]] QImage render_marker_label_mask(
        const domain::MarkerLabelMask& mask
    );
}