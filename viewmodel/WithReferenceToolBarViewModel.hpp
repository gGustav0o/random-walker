#pragma once

#include <QObject>

#include "global.hpp"

class WithReferenceToolBarViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visible_changed)
    Q_PROPERTY(int current_step READ current_step NOTIFY current_step_changed)
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
    explicit WithReferenceToolBarViewModel(QObject* parent = nullptr) : QObject(parent), visible_(false) {};
    enum Step {
        LoadReference
        , Align
        , Preprocess
        , ROI
        , AbsDiff
        , Segment
        , Morphology
        , Cleanup
        , ContourAnalysis
        , Features
        , Compare
        , Classify
        , StepCount
        , AllDisabled
    };
    Q_ENUM(Step)

    int current_step() const { return current_step_; }

public slots:
    void reset_steps() {
        current_step_ = LoadReference;
        emit current_step_changed();
	}
    void block_steps() {
        current_step_ = AllDisabled;
        emit current_step_changed();
	}

	Q_INVOKABLE void on_load_reference();
	Q_INVOKABLE void on_align();
	Q_INVOKABLE void on_preprocess();
	Q_INVOKABLE void on_select_roi();
	Q_INVOKABLE void on_abs_diff();
	Q_INVOKABLE void on_segment();
	Q_INVOKABLE void on_morphology();
	Q_INVOKABLE void on_cleanup();
	Q_INVOKABLE void on_contour_analysis();
	Q_INVOKABLE void on_features();
	Q_INVOKABLE void on_compare();
	Q_INVOKABLE void on_classify();
signals:
    void current_step_changed();
private:
    int current_step_ = AllDisabled;
    void move_to_next_step();
};