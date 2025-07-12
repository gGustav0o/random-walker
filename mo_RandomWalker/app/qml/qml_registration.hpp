#pragma once

#include <QQmlEngine>
#include <QMetaType>
#include <qqml.h>

#include "MetaAnnotations.hpp"

#include "app/enums/QmlEnums.hpp"
#include "viewmodel/ControlPanelViewModel.hpp"

_cold_ static inline void register_qml_types()
{
    qmlRegisterUncreatableType<QmlSeedLabel>("App.Enums", 1, 0, "SeedLabel", "Enum only");
    //qmlRegisterUncreatableType<ControlPanelViewModel>("App.ViewModel", 1, 0, "ControlPanelViewModel", "");
}