#include "EventBus.hpp"

void EventBus::on_has_reference_changed(bool has_reference) {
	emit with_reference_tool_bar_request_set_visible(has_reference);
	emit without_reference_tool_bar_request_set_visible(!has_reference);
}

void EventBus::on_base_image_loaded_changed(bool loaded) {
	emit mode_selection_request_set_visible(loaded);
	emit control_panel_request_set_image_loaded(loaded);
	if (!loaded) {
		emit with_reference_tool_bar_request_set_visible(false);
		emit without_reference_tool_bar_request_set_visible(false);
		emit with_reference_control_panel_request_disable_steps();
		emit without_reference_control_panel_request_disable_steps();
	} else {
		emit with_reference_control_panel_request_reset_steps();
		emit without_reference_control_panel_request_reset_steps();
	}
}