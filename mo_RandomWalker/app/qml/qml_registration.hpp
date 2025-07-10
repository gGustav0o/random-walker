#pragma once

#include <QQmlEngine>
#include <QMetaType>
#include <qqml.h>

#include "MetaAnnotations.hpp"

#include "app/enums/QmlEnums.hpp"
#include "app/data/Rect.hpp"
#include "viewmodel/SeedsListModel.hpp"
#include "viewmodel/ControlPanelViewModel.hpp"

_cold_ static inline void register_qml_types()
{
    qRegisterMetaType<SeedRect>("SeedRect");
    qmlRegisterUncreatableType<SeedRect>("App.Data", 1, 0, "seedRect", "Rect is a value-type, not creatable in QML");
    qmlRegisterUncreatableType<QmlSeedLabel>("App.Enums", 1, 0, "SeedLabel", "Enum only");
    qmlRegisterUncreatableType<SeedsListModel>("App.ViewModel", 1, 0, "SeedsListModel", "Seed Rect list model");
    //qmlRegisterUncreatableType<ControlPanelViewModel>("App.ViewModel", 1, 0, "ControlPanelViewModel", "");
}