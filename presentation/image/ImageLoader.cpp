#include "ImageLoader.hpp"

#include <QUrl>

namespace random_walker::presentation {
    ImageLoadOutcome load_image(const QString& path) {
        const QUrl url(path);
        const QString local_path = url.isLocalFile() ? url.toLocalFile() : path;
        QImage loaded(local_path);

        if (loaded.isNull()) {
            return application::ImageLoadError::Failed;
        }

        return loaded;
    }
}
