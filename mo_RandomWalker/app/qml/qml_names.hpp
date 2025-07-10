#pragma once

#include <QString>

namespace qml_names
{
#define QML_LITERAL(name, str) inline const auto name = QStringLiteral(str)

    QML_LITERAL(kBaseImageProvider, "BaseImageProvider");
    QML_LITERAL(kBaseImageViewModel, "BaseImageViewModel");
    QML_LITERAL(kControlPanelViewModel, "ControlPanelViewModel");
}