#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QmlSeedLabel : public QObject
{
    Q_OBJECT

public:
    enum SeedLabel {
        Background
        , Object
    };
    Q_ENUM(SeedLabel)
};
