#include "AppContext.hpp"

#include <memory>

#include <QQmlContext>

#include "application/diagnostics/Logging.hpp"
#include "presentation/image/BaseImageProvider.hpp"
#include "presentation/image/ResultImageProvider.hpp"
#include "presentation/qml/qml_names.hpp"

AppContext::AppContext(QQmlApplicationEngine& engine)
    : settings_repository_(
        QStringLiteral("random-walker")
        , QStringLiteral("random-walker")
    )
    , settings_service_(settings_repository_)
{
    random_walker::application::log_info(
        random_walker::application::log_category::bootstrap
        , "Creating application context"
    );

    auto base_image_provider = std::make_unique<BaseImageProvider>();
    auto result_image_provider = std::make_unique<ResultImageProvider>();

    segmentation_view_model_ = std::make_unique<SegmentationViewModel>(
        segmentation_executor_
        , settings_service_
        , *base_image_provider
        , *result_image_provider
    );

    engine.rootContext()->setContextProperty(
        qml_names::kSegmentationViewModel
        , segmentation_view_model_.get()
    );

    engine.addImageProvider(
        qml_names::kBaseImageProvider
        , base_image_provider.release()
    );
    engine.addImageProvider(
        qml_names::kResultImageProvider
        , result_image_provider.release()
    );

    random_walker::application::log_info(
        random_walker::application::log_category::bootstrap
        , "Application context created"
    );
}
