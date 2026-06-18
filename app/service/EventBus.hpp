#pragma once

#include <QObject>

class EventBus : public QObject {
	Q_OBJECT
public:
	explicit EventBus(QObject* parent = nullptr) : QObject(parent) {}

public slots:
	void on_base_image_loaded_changed(bool);
	void on_has_reference_changed(bool);
signals:
	void clear_requested();
	void mode_selection_request_set_visible(bool);
	void base_image_open_requested(const QString& path);
	void with_reference_tool_bar_request_set_visible(bool);
	void without_reference_tool_bar_request_set_visible(bool);
	void control_panel_request_set_image_loaded(bool);
	void with_reference_control_panel_request_disable_steps();
	void without_reference_control_panel_request_disable_steps();
	void with_reference_control_panel_request_reset_steps();
	void without_reference_control_panel_request_reset_steps();
};