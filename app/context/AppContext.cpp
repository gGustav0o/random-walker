#include "AppContext.hpp"

#include <memory>

#include <QQmlContext>

#include "app/qml/qml_names.hpp"
#include "app/service/BaseImageProvider.hpp"
#include "app/service/ResultImageProvider.hpp"

AppContext::AppContext(QQmlApplicationEngine& engine)
{
    auto base_image_provider = std::make_unique<BaseImageProvider>();
    auto result_image_provider = std::make_unique<ResultImageProvider>();

    segmentation_view_model_ = std::make_unique<SegmentationViewModel>(
        segmentation_executor_,
        *base_image_provider,
        *result_image_provider);

    engine.rootContext()->setContextProperty(
        qml_names::kSegmentationViewModel,
        segmentation_view_model_.get());

    engine.addImageProvider(
        qml_names::kBaseImageProvider,
        base_image_provider.release());
    engine.addImageProvider(
        qml_names::kResultImageProvider,
        result_image_provider.release());
}
