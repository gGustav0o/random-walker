#pragma once

#include <QObject>
#include <QImage>

#include "app/service/BaseImageProvider.hpp"
#include "model/utils/ImageUtils.hpp"

class BaseImageViewModel : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString image_source READ image_source NOTIFY image_source_changed)
	Q_PROPERTY(bool image_loaded READ image_loaded NOTIFY image_loaded_changed)
	Q_PROPERTY(size_t image_version READ image_version NOTIFY image_version_changed)
public:
	explicit BaseImageViewModel(BaseImageProvider* provider = nullptr, QObject* parent = nullptr);
	QString image_source() const;
	bool image_loaded() const;
	size_t image_version() const;

public slots:
	void on_image_opened(const QString& path);
	void on_clear_base_image_requested();

signals:
	void image_source_changed();
	void image_loaded_changed(bool);
	void image_version_changed();
private:
	BaseImageProvider* provider_ = nullptr;
	bool loaded_ = false;
	size_t image_version_ = 0;
};