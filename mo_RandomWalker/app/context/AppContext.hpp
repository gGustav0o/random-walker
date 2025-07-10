#pragma once

#include <QQmlApplicationEngine>

#include "MetaAnnotations.hpp"

#include "viewmodel/BaseImageViewModel.hpp"
#include "viewmodel/ControlPanelViewModel.hpp"
#include "viewmodel/ResultOverlayViewModel.hpp"
#include "viewmodel/SeedsOverlayViewModel.hpp"
#include "viewmodel/SettingsPanelViewModel.hpp"

#include "app/service/BaseImageProvider.hpp"

struct AppContext
{
    _cold_ void connect_components();
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
	SeedsOverlayViewModel* seeds_overlay_view_model = nullptr;
	SettingsPanelViewModel* settings_panel_view_model = nullptr;

	BaseImageProvider* base_image_provider = nullptr;
};
