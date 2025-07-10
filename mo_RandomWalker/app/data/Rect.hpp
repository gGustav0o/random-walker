#pragma once

#include <QObject>
#include <QMetaType>
#include <QPoint>

struct SeedRect {
    Q_GADGET
    Q_PROPERTY(QPoint top_left_ MEMBER top_left_)
    Q_PROPERTY(QPoint bottom_right_ MEMBER bottom_right_)
public:
    QPoint top_left_;
    QPoint bottom_right_;
};

Q_DECLARE_METATYPE(SeedRect)