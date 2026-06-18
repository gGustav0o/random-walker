#pragma once

#include <QObject>

#include "app/enums/QmlEnums.hpp"

class ControlPanelViewModel : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool image_loaded READ image_loaded WRITE set_image_loaded NOTIFY image_loaded_changeed)
public:
	explicit ControlPanelViewModel(QObject* parent = nullptr);

	Q_INVOKABLE void open_image(const QString& path);
	Q_INVOKABLE void clear();

	bool image_loaded() const { return image_loaded_; }
	void set_image_loaded(bool value) { image_loaded_ = value; emit image_loaded_changeed(); }
signals:
	void base_image_open_requested(const QString& path);
	void selected_type_changed();
	void clear_requested();
	void image_loaded_changeed();
private:
	bool image_loaded_ = false;
};