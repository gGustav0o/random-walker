#pragma once

#include <QString>

namespace qml_names
{
#define QML_LITERAL(name, str) inline const auto name = QStringLiteral(str)

    QML_LITERAL(kBaseImageProvider, "BaseImageProvider");
    QML_LITERAL(kResultImageProvider, "ResultImageProvider");
    QML_LITERAL(kSegmentationViewModel, "SegmentationViewModel");
}
