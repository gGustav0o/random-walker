#include "ViewModelConversions.hpp"

#include <QtGlobal>

#include "viewmodel/SegmentationViewModel.hpp"

namespace random_walker::viewmodel {

    int view_connectivity(domain::PixelConnectivity connectivity) noexcept {
        switch (connectivity) {
        case domain::PixelConnectivity::Four:
            return SegmentationViewModel::FourConnectivity;
        case domain::PixelConnectivity::Eight:
            return SegmentationViewModel::EightConnectivity;
        }

        Q_ASSERT_X(false, "view_connectivity", "Unhandled connectivity");
        return SegmentationViewModel::FourConnectivity;
    }

    std::optional<domain::PixelConnectivity> domain_connectivity(
        int connectivity
    ) noexcept {
        switch (connectivity) {
        case SegmentationViewModel::FourConnectivity:
            return domain::PixelConnectivity::Four;
        case SegmentationViewModel::EightConnectivity:
            return domain::PixelConnectivity::Eight;
        default:
            return std::nullopt;
        }
    }

    int view_foreground_polarity(
        domain::ForegroundPolarity foreground_polarity
    ) noexcept {
        switch (foreground_polarity) {
        case domain::ForegroundPolarity::DarkObject:
            return SegmentationViewModel::DarkObjectForeground;
        case domain::ForegroundPolarity::BrightObject:
            return SegmentationViewModel::BrightObjectForeground;
        }

        Q_ASSERT_X(
            false
            , "view_foreground_polarity"
            , "Unhandled foreground polarity"
        );
        return SegmentationViewModel::BrightObjectForeground;
    }

    std::optional<domain::ForegroundPolarity> domain_foreground_polarity(
        int foreground_polarity
    ) noexcept {
        switch (foreground_polarity) {
        case SegmentationViewModel::DarkObjectForeground:
            return domain::ForegroundPolarity::DarkObject;
        case SegmentationViewModel::BrightObjectForeground:
            return domain::ForegroundPolarity::BrightObject;
        default:
            return std::nullopt;
        }
    }

    application::ApplicationError application_error(
        application::ExecutionError error
    ) noexcept {
        switch (error) {
        case application::ExecutionError::UnexpectedInternalFailure:
            return application::ApplicationError::UnexpectedInternalFailure;
        }

        Q_ASSERT_X(false, "application_error", "Unhandled execution error");
        return application::ApplicationError::UnexpectedInternalFailure;
    }
}
