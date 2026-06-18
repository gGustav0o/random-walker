#pragma once

#include <QObject>

#include "global.hpp"


class ModeSelectionViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool has_reference READ has_reference NOTIFY has_reference_changed)
    Q_PROPERTY(bool visible READ visible NOTIFY visible_changed)
    bool visible_;
    void set_visible(bool value) {
        if (visible_ == value) return;
        visible_ = value;
        emit visible_changed(visible_);
    }
public:
    bool visible() const { return visible_; }
signals:
    void visible_changed(bool);
public slots:
    void hide() { set_visible(false); }
    void show() { set_visible(true); }
    void on_request_set_visible(bool value) {
        if (visible_ == value) return;
        visible_ = value;
        emit visible_changed(visible_);
    }
public:
    explicit ModeSelectionViewModel(QObject* parent = nullptr) : QObject(parent), visible_(false){}
    bool has_reference() const { return has_reference_; }
    Q_INVOKABLE void select_with_reference() {
        set_has_reference(true);
        hide();
    }
    Q_INVOKABLE void select_without_reference() {
        set_has_reference(false);
        hide();
    }

signals:
    void has_reference_changed(bool);

private:
    bool has_reference_ = false;
    void set_has_reference(bool value) {
        has_reference_ = value;
        emit has_reference_changed(has_reference_);
    }
};
