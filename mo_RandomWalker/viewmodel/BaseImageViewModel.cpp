#include "BaseImageViewModel.hpp"
#include "app/qml/qml_names.hpp"

BaseImageViewModel::BaseImageViewModel(BaseImageProvider* provider, QObject* parent)
	: QObject(parent), provider_(provider) {}

QString BaseImageViewModel::image_source() const {
	return image_loaded() ? QString("image://%1/processed").arg(qml_names::kBaseImageProvider) : QString();
}

bool BaseImageViewModel::image_loaded() const {
	return loaded_;
}

size_t BaseImageViewModel::image_version() const {
    return image_version_;
}

void BaseImageViewModel::on_image_opened(const QString& path) {
    QUrl url(path);
    QString filePath = url.isLocalFile() ? url.toLocalFile() : path;

    QImage img(filePath);
    if (img.isNull() || !provider_) {
        loaded_ = false;
        emit image_source_changed();
        emit image_loaded_changed();
        return;
    }
    QImage gray = to_grayscale(img);
    provider_->set_image(gray);
    ++image_version_;
    loaded_ = true;
	emit image_version_changed();
    emit image_source_changed();
    emit image_loaded_changed();
}

void BaseImageViewModel::on_image_cleared() {
    if (provider_) provider_->clear();
    loaded_ = false;
    emit image_source_changed();
    emit image_loaded_changed();
}
