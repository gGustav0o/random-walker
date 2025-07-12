#include "ControlPanelViewModel.hpp"

ControlPanelViewModel::ControlPanelViewModel(QObject* parent)
	: QObject(parent)
{}

Q_INVOKABLE void ControlPanelViewModel::open_image(const QString& path) {
	emit base_image_open_requested(path);
}

Q_INVOKABLE void ControlPanelViewModel::clear() {
	emit clear_requested();
}