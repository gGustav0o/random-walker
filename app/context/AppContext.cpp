#include "AppContext.hpp"

#include <QQmlContext>

#include "app/qml/qml_names.hpp"


_cold_ void AppContext::connect_to_event_bus()
{
    QObject::connect(control_panel_view_model, &ControlPanelViewModel::base_image_open_requested, event_bus, &EventBus::base_image_open_requested);
    QObject::connect(control_panel_view_model, &ControlPanelViewModel::clear_requested, event_bus, &EventBus::clear_requested);
    QObject::connect(base_image_view_model, &BaseImageViewModel::image_loaded_changed, event_bus, &EventBus::on_base_image_loaded_changed);
    QObject::connect(mode_selection_panel_view_model, &ModeSelectionViewModel::has_reference_changed, event_bus, &EventBus::on_has_reference_changed);
}

_cold_ void AppContext::connect_from_event_bus()
{
    QObject::connect(event_bus, &EventBus::base_image_open_requested, base_image_view_model, &BaseImageViewModel::on_image_opened);
    QObject::connect(event_bus, &EventBus::clear_requested, base_image_view_model, &BaseImageViewModel::on_clear_base_image_requested);
    QObject::connect(event_bus, &EventBus::mode_selection_request_set_visible, mode_selection_panel_view_model, &ModeSelectionViewModel::on_request_set_visible);
    QObject::connect(event_bus, &EventBus::with_reference_tool_bar_request_set_visible, with_reference_tool_bar_view_model, &WithReferenceToolBarViewModel::on_request_set_visible);
    QObject::connect(event_bus, &EventBus::without_reference_tool_bar_request_set_visible, without_reference_tool_bar_view_model, &WithoutReferenceToolBarViewModel::on_request_set_visible);
    QObject::connect(event_bus, &EventBus::control_panel_request_set_image_loaded, control_panel_view_model, &ControlPanelViewModel::set_image_loaded);
    QObject::connect(event_bus, &EventBus::without_reference_control_panel_request_disable_steps, without_reference_tool_bar_view_model, &WithoutReferenceToolBarViewModel::block_steps);
    QObject::connect(event_bus, &EventBus::with_reference_control_panel_request_disable_steps, with_reference_tool_bar_view_model, &WithReferenceToolBarViewModel::block_steps);
    QObject::connect(event_bus, &EventBus::without_reference_control_panel_request_reset_steps, without_reference_tool_bar_view_model, &WithoutReferenceToolBarViewModel::reset_steps);
    QObject::connect(event_bus, &EventBus::with_reference_control_panel_request_reset_steps, with_reference_tool_bar_view_model, &WithReferenceToolBarViewModel::reset_steps);
}

_cold_ void AppContext::initialize(QQmlApplicationEngine* engine)
{
    this->engine = engine;

	create_image_providers();
    set_image_providers();
	create_view_models();
    connect_to_event_bus();
    connect_from_event_bus();
	set_context_properties();
}

_cold_ void AppContext::create_view_models()
{
    event_bus = new EventBus();

	base_image_view_model = new BaseImageViewModel(base_image_provider);
	control_panel_view_model = new ControlPanelViewModel();
    mode_selection_panel_view_model = new ModeSelectionViewModel();
    without_reference_tool_bar_view_model = new WithoutReferenceToolBarViewModel();
    with_reference_tool_bar_view_model = new WithReferenceToolBarViewModel();
}

_cold_ void AppContext::set_image_providers()
{
    engine->addImageProvider(qml_names::kBaseImageProvider, base_image_provider);
}

_cold_ void AppContext::create_image_providers()
{
    base_image_provider = new BaseImageProvider();
}

_cold_ void AppContext::set_context_properties()
{
    engine->rootContext()->setContextProperty(qml_names::kBaseImageViewModel, base_image_view_model);
    engine->rootContext()->setContextProperty(qml_names::kControlPanelViewModel, control_panel_view_model);
    engine->rootContext()->setContextProperty(qml_names::kModeSelectionlPanelViewModel, mode_selection_panel_view_model);
    engine->rootContext()->setContextProperty(qml_names::kWithoutReferenceToolBarViewModel, without_reference_tool_bar_view_model);
    engine->rootContext()->setContextProperty(qml_names::kWithReferenceToolBarViewModel, with_reference_tool_bar_view_model);
}
