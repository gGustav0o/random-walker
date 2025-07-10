#include "ControlPanelViewModel.hpp"

ControlPanelViewModel::ControlPanelViewModel(QObject* parent)
	: QObject(parent)
{}

void ControlPanelViewModel::set_selected_type(QmlSeedLabel::SeedLabel type)
{
	if (selected_type_ != type)
	{
		selected_type_ = type;
		emit selected_type_changed();
	}
}

Q_INVOKABLE void ControlPanelViewModel::open_image(const QString& path) {
	emit image_opened(path);
}

Q_INVOKABLE void ControlPanelViewModel::clear_seeds() {
	emit clear_seeds_requested();
}

Q_INVOKABLE void ControlPanelViewModel::clear() {
	emit clear_requested();
}

Q_INVOKABLE void ControlPanelViewModel::start() {
	emit start_requested();
}

QmlSeedLabel::SeedLabel ControlPanelViewModel::selected_type() const {
	return selected_type_;
}