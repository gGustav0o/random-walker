#include "AppContext.hpp"

#include <QQmlContext>

#include "app/qml/qml_names.hpp"


_cold_ void AppContext::connect_components()
{
    QObject::connect(control_panel_view_model, &ControlPanelViewModel::image_opened, base_image_view_model, &BaseImageViewModel::on_image_opened);
    QObject::connect(control_panel_view_model, &ControlPanelViewModel::clear_requested, base_image_view_model, &BaseImageViewModel::on_image_cleared);
}

_cold_ void AppContext::initialize(QQmlApplicationEngine* engine)
{
    this->engine = engine;

	create_image_providers();
    set_image_providers();
	create_view_models();
    connect_components();
	set_context_properties();
}

_cold_ void AppContext::create_view_models()
{
	base_image_view_model = new BaseImageViewModel(base_image_provider);
	control_panel_view_model = new ControlPanelViewModel();
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
}
