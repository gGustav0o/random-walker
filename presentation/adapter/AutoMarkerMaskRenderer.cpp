#include "AutoMarkerMaskRenderer.hpp"

#include <QColor>
#include <QtGlobal>

namespace random_walker::qt_adapter {
    QImage render_marker_label_mask(const domain::MarkerLabelMask& mask) {
        Q_ASSERT(mask.width() >= 0);
        Q_ASSERT(mask.height() >= 0);

        const QColor transparent(0, 0, 0, 0);
        const QColor background_color(0, 80, 255, 60);
        const QColor object_color(255, 40, 40, 60);

        QImage result(
            mask.width()
            , mask.height()
            , QImage::Format_ARGB32
        );
        result.fill(transparent);

        for (int row = 0; row < result.height(); ++row) {
            for (int column = 0; column < result.width(); ++column) {
                switch (mask.at(row, column)) {
                case domain::MarkerLabel::None:
                    break;
                case domain::MarkerLabel::Background:
                    result.setPixelColor(column, row, background_color);
                    break;
                case domain::MarkerLabel::Object:
                    result.setPixelColor(column, row, object_color);
                    break;
                }
            }
        }

        return result;
    }
}