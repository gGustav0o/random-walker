#pragma once

#include <QQmlApplicationEngine>

#include "MetaAnnotations.hpp"

#include "viewmodel/BaseImageViewModel.hpp"
#include "viewmodel/ControlPanelViewModel.hpp"
#include "viewmodel/ResultOverlayViewModel.hpp"
#include "viewmodel/SettingsPanelViewModel.hpp"
#include "viewmodel/ModeSelectionPanelViewModel.hpp"
#include "viewmodel/WithReferenceToolBarViewModel.hpp"
#include "viewmodel/WithoutReferenceToolBarViewModel.hpp"
#include "app/service/EventBus.hpp"

#include "app/service/BaseImageProvider.hpp"

struct AppContext
{
    _cold_ void connect_to_event_bus();
    _cold_ void connect_from_event_bus();
    _cold_ void initialize(QQmlApplicationEngine* engine);
	_cold_ void create_view_models();
	_cold_ void set_image_providers();
	_cold_ void create_image_providers();
	_cold_ void set_context_properties();
	QQmlApplicationEngine* get_engine() const { return engine; }
private:
    QQmlApplicationEngine* engine = nullptr;

	BaseImageViewModel* base_image_view_model = nullptr;
	ControlPanelViewModel* control_panel_view_model = nullptr;
	ResultOverlayViewModel* result_overlay_view_model = nullptr;
	SettingsPanelViewModel* settings_panel_view_model = nullptr;
	ModeSelectionViewModel* mode_selection_panel_view_model = nullptr;
	WithReferenceToolBarViewModel* with_reference_tool_bar_view_model = nullptr;
	WithoutReferenceToolBarViewModel* without_reference_tool_bar_view_model = nullptr;
	EventBus* event_bus = nullptr;

	BaseImageProvider* base_image_provider = nullptr;
};
