#include "ErrorPresenter.hpp"

#include <variant>

#include <QtGlobal>

namespace random_walker::presentation {

    namespace {
        [[nodiscard]] QString application_error_message(
            application::ApplicationError error
        ) {
            using Error = application::ApplicationError;

            switch (error) {
            case Error::UnexpectedInternalFailure:
                return QStringLiteral("Unexpected internal application error.");
            }

            Q_ASSERT_X(
                false
                , "application_error_message"
                , "Unhandled application error"
            );
            return QStringLiteral("Unknown application error.");
        }

        [[nodiscard]] QString image_load_error_message(
            application::ImageLoadError error
        ) {
            using Error = application::ImageLoadError;

            switch (error) {
            case Error::Failed:
                return QStringLiteral("Failed to load the selected image.");
            }

            Q_ASSERT_X(
                false
                , "image_load_error_message"
                , "Unhandled image load error"
            );
            return QStringLiteral("Unknown image loading error.");
        }

        [[nodiscard]] QString settings_error_message(
            application::SettingsError error
        ) {
            using Error = application::SettingsError;

            switch (error) {
            case Error::InvalidSettings:
                return QStringLiteral("Application settings are invalid.");
            case Error::SaveFailed:
                return QStringLiteral("Failed to save application settings.");
            }

            Q_ASSERT_X(
                false
                , "settings_error_message"
                , "Unhandled settings error"
            );
            return QStringLiteral("Unknown settings error.");
        }


        [[nodiscard]] QString auto_marker_error_message(
            application::AutoMarkerError error
        ) {
            using Error = application::AutoMarkerError;

            switch (error) {
            case Error::EmptyImage:
                return QStringLiteral("No image is loaded.");
            case Error::InvalidParameters:
                return QStringLiteral("Automatic marker parameters are invalid.");
            case Error::ProposalFailed:
                return QStringLiteral("Failed to propose automatic markers.");
            }

            Q_ASSERT_X(
                false
                , "auto_marker_error_message"
                , "Unhandled auto marker error"
            );
            return QStringLiteral("Unknown automatic marker error.");
        }
        [[nodiscard]] QString segmentation_error_message(
            domain::SegmentationError error
        ) {
            using Error = domain::SegmentationError;

            switch (error) {
            case Error::EmptyImage:
                return QStringLiteral("No image is loaded.");
            case Error::ImageTooLarge:
                return QStringLiteral("The selected image is too large.");
            case Error::InvalidBeta:
                return QStringLiteral(
                    "Beta is outside the supported range.");
            case Error::InvalidDistancePower:
                return QStringLiteral(
                    "Distance power is outside the supported range.");
            case Error::InvalidConnectivity:
                return QStringLiteral(
                    "Pixel connectivity is not supported.");
            case Error::MissingBackgroundSeeds:
                return QStringLiteral("At least one background seed is required.");
            case Error::MissingObjectSeeds:
                return QStringLiteral("At least one object seed is required.");
            case Error::SeedOutOfBounds:
                return QStringLiteral("A seed lies outside the image.");
            case Error::ConflictingSeedLabels:
                return QStringLiteral(
                    "Background and object seeds must not overlap.");
            case Error::LaplacianDecompositionFailed:
                return QStringLiteral("Failed to decompose the graph Laplacian.");
            case Error::LinearSystemSolveFailed:
                return QStringLiteral("Failed to solve the Random Walker system.");
            case Error::NonFiniteSolution:
                return QStringLiteral("The Random Walker solution is not finite.");
            }

            Q_ASSERT_X(
                false
                , "segmentation_error_message"
                , "Unhandled segmentation error"
            );
            return QStringLiteral("Unknown segmentation error.");
        }
    }

    QString error_message(const application::UserError& error) {
        if (const auto* application_error =
                std::get_if<application::ApplicationError>(&error)
        ) {
            return application_error_message(*application_error);
        }

        if (const auto* image_load_error =
                std::get_if<application::ImageLoadError>(&error)
        ) {
            return image_load_error_message(*image_load_error);
        }

        if (const auto* settings_error =
                std::get_if<application::SettingsError>(&error)
        ) {
            return settings_error_message(*settings_error);
        }

        if (const auto* auto_marker_error =
                std::get_if<application::AutoMarkerError>(&error)
        ) {
            return auto_marker_error_message(*auto_marker_error);
        }

        if (const auto* segmentation_error =
                std::get_if<domain::SegmentationError>(&error)
        ) {
            return segmentation_error_message(*segmentation_error);
        }

        Q_ASSERT_X(false, "error_message", "Unhandled user error variant");
        return QStringLiteral("Unknown application error.");
    }
}
