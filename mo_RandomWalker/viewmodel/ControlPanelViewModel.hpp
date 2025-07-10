#pragma once

#include <QObject>

#include "app/enums/QmlEnums.hpp"

class ControlPanelViewModel : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QmlSeedLabel::SeedLabel selected_type READ selected_type WRITE set_selected_type NOTIFY selected_type_changed)

public:
	explicit ControlPanelViewModel(QObject* parent = nullptr);
	QmlSeedLabel::SeedLabel selected_type() const;
	void set_selected_type(QmlSeedLabel::SeedLabel type);

	Q_INVOKABLE void open_image(const QString& path);
	Q_INVOKABLE void clear_seeds();
	Q_INVOKABLE void clear();
	Q_INVOKABLE void start();

signals:
	void image_opened(const QString& path);
	void clear_seeds_requested();
	void selected_type_changed();
	void start_requested();
	void clear_requested();
	void seeds_changed();
private:
	QmlSeedLabel::SeedLabel selected_type_ = QmlSeedLabel::SeedLabel::Object;
};