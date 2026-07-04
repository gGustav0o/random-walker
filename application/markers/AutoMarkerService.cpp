#include "AutoMarkerService.hpp"

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "application/diagnostics/Logging.hpp"
#include "model/markers/GmmMarkerProposal.hpp"

namespace random_walker::application {
    namespace {
        [[nodiscard]] std::string_view marker_proposal_error_name(
            markers::MarkerProposalError error
        ) noexcept {
            using Error = markers::MarkerProposalError;
            switch (error) {
            case Error::EmptyImage:
                return "EmptyImage";
            case Error::InvalidParameters:
                return "InvalidParameters";
            case Error::GmmFitFailed:
                return "GmmFitFailed";
            }

            return "Unknown";
        }

        [[nodiscard]] std::string_view foreground_polarity_name(
            domain::ForegroundPolarity polarity
        ) noexcept {
            using Polarity = domain::ForegroundPolarity;
            switch (polarity) {
            case Polarity::DarkObject:
                return "DarkObject";
            case Polarity::BrightObject:
                return "BrightObject";
            }

            return "Unknown";
        }

        [[nodiscard]] std::string_view bool_name(bool value) noexcept {
            return value ? "true" : "false";
        }

        [[nodiscard]] bool contains_only_manual_seed_regions(
            std::span<const domain::SeedRegion> seed_regions
        ) noexcept {
            return std::all_of(
                seed_regions.begin()
                , seed_regions.end()
                , [](const domain::SeedRegion& region) {
                    return region.source == domain::SeedSource::User;
                }
            );
        }

        [[nodiscard]] AutoMarkerError auto_marker_error(
            markers::MarkerProposalError error
        ) noexcept {
            using Error = markers::MarkerProposalError;
            switch (error) {
            case Error::EmptyImage:
                return AutoMarkerError::EmptyImage;
            case Error::InvalidParameters:
                return AutoMarkerError::InvalidParameters;
            case Error::GmmFitFailed:
                return AutoMarkerError::ProposalFailed;
            }

            return AutoMarkerError::ProposalFailed;
        }

        [[nodiscard]] std::string proposal_start_message(
            const domain::GrayImage& image
            , const domain::AutoMarkerParameters& parameters
            , std::span<const domain::SeedRegion> manual_seed_regions
        ) {
            return std::string("Auto marker proposal started: image=")
                + std::to_string(image.width())
                + "x"
                + std::to_string(image.height())
                + ", manual_seed_regions="
                + std::to_string(manual_seed_regions.size())
                + ", confidence_threshold="
                + std::to_string(parameters.confidence_threshold)
                + ", minimum_component_area="
                + std::to_string(parameters.minimum_component_area)
                + ", erosion_radius="
                + std::to_string(parameters.erosion_radius)
                + ", foreground_polarity="
                + std::string(foreground_polarity_name(parameters.foreground_polarity))
                + ", gmm_component_count="
                + std::to_string(parameters.gmm.component_count)
                + ", gmm_max_iterations="
                + std::to_string(parameters.gmm.max_iterations)
                + ", gmm_convergence_tolerance="
                + std::to_string(parameters.gmm.convergence_tolerance)
                + ", gmm_minimum_variance="
                + std::to_string(parameters.gmm.minimum_variance);
        }

        [[nodiscard]] std::string proposal_success_message(
            const domain::MarkerProposalDiagnostics& diagnostics
        ) {
            return std::string("Auto marker proposal completed: proposed_seed_count=")
                + std::to_string(diagnostics.proposed_seed_count)
                + ", rejected_low_confidence="
                + std::to_string(diagnostics.rejected_low_confidence_count)
                + ", rejected_small_component="
                + std::to_string(diagnostics.rejected_small_component_count)
                + ", rejected_erosion="
                + std::to_string(diagnostics.rejected_erosion_count)
                + ", rejected_empty_core_component="
                + std::to_string(diagnostics.rejected_empty_core_component_count)
                + ", rejected_manual_conflict="
                + std::to_string(diagnostics.rejected_manual_conflict_count)
                + ", gmm_iterations="
                + std::to_string(diagnostics.gmm_iterations)
                + ", gmm_converged="
                + std::string(bool_name(diagnostics.gmm_converged));
        }
    }

    AutoMarkerProposalOutcome AutoMarkerService::propose(
        const domain::GrayImage& image
        , const domain::AutoMarkerParameters& parameters
        , std::span<const domain::SeedRegion> manual_seed_regions
    ) const {
        assert(contains_only_manual_seed_regions(manual_seed_regions));

        log_info(
            log_category::auto_markers
            , proposal_start_message(image, parameters, manual_seed_regions)
        );

        markers::MarkerProposalOutcome outcome = markers::propose_gmm_markers(
            image
            , parameters
        );
        if (const auto* error = std::get_if<markers::MarkerProposalError>(&outcome)) {
            log_warning(
                log_category::auto_markers
                , std::string("Auto marker proposal failed: error=")
                    + std::string(marker_proposal_error_name(*error))
            );
            return auto_marker_error(*error);
        }

        domain::MarkerProposal proposal = markers::remove_manual_conflicts(
            std::get<domain::MarkerProposal>(std::move(outcome))
            , manual_seed_regions
        );
        log_info(
            log_category::auto_markers
            , proposal_success_message(proposal.diagnostics)
        );
        return proposal;
    }
}
