#pragma once

#include "model/algorithm/DistanceTransform.hpp"
#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/ImageGeometry.hpp"
#include "model/markers/GmmIntensity.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <variant>
#include <vector>

namespace random_walker::markers {

    enum class MarkerProposalError {
        EmptyImage
        , InvalidParameters
        , GmmFitFailed
    };

    using MarkerProposalOutcome =
        std::variant<domain::MarkerProposal, MarkerProposalError>;

    struct CandidatePixel {
        domain::PixelCoordinate position;
        domain::SeedLabel label = domain::SeedLabel::Background;
        double confidence = 0.0;
        bool operator==(const CandidatePixel&) const = default;
    };

    [[nodiscard]] inline int image_pixel_count(
        const domain::GrayImage& image
    ) noexcept {
        assert(!image.empty());
        assert(image.width() > 0);
        assert(image.height() > 0);
        return domain::pixel_count_as_int(image.width(), image.height());
    }

    [[nodiscard]] inline int flatten(
        domain::PixelCoordinate position
        , int width
    ) noexcept {
        assert(position.x >= 0);
        assert(position.y >= 0);
        assert(width > 0);
        assert(position.x < width);
        return domain::linear_pixel_index(position.y, position.x, width);
    }

    [[nodiscard]] inline domain::PixelCoordinate unflatten(
        int index
        , int width
    ) noexcept {
        assert(index >= 0);
        assert(width > 0);
        return {
            .x = index % width
            , .y = index / width
        };
    }

    [[nodiscard]] inline std::vector<double> image_intensity_samples(
        const domain::GrayImage& image
    ) {
        assert(!image.empty());

        std::vector<double> samples;
        samples.reserve(static_cast<std::size_t>(image_pixel_count(image)));
        for (int row = 0; row < image.height(); ++row) {
            for (int column = 0; column < image.width(); ++column) {
                samples.push_back(static_cast<double>(image.at(row, column)));
            }
        }
        return samples;
    }

    [[nodiscard]] inline std::vector<CandidatePixel> high_confidence_candidates(
        const domain::GrayImage& image
        , const GmmModel1d& model
        , const domain::AutoMarkerParameters& parameters
        , domain::MarkerProposalDiagnostics& diagnostics
    ) {
        assert(!image.empty());
        assert(is_valid(model));
        assert(domain::is_valid(parameters));

        std::vector<CandidatePixel> candidates;
        candidates.reserve(static_cast<std::size_t>(image_pixel_count(image)));

        for (int row = 0; row < image.height(); ++row) {
            for (int column = 0; column < image.width(); ++column) {
                const double intensity = static_cast<double>(image.at(row, column));
                const double object_probability = foreground_probability(
                    model
                    , intensity
                    , parameters.foreground_polarity
                );
                const double background_probability = 1.0 - object_probability;

                if (object_probability >= parameters.confidence_threshold) {
                    candidates.push_back({
                        .position = {.x = column, .y = row}
                        , .label = domain::SeedLabel::Object
                        , .confidence = object_probability
                    });
                } else if (background_probability >= parameters.confidence_threshold) {
                    candidates.push_back({
                        .position = {.x = column, .y = row}
                        , .label = domain::SeedLabel::Background
                        , .confidence = background_probability
                    });
                } else {
                    ++diagnostics.rejected_low_confidence_count;
                }
            }
        }

        return candidates;
    }

    [[nodiscard]] inline bool is_inside(
        domain::PixelCoordinate position
        , int width
        , int height
    ) noexcept {
        return position.x >= 0
            && position.y >= 0
            && position.x < width
            && position.y < height;
    }

    [[nodiscard]] inline std::vector<std::uint8_t> distance_safe_candidates(
        const std::vector<CandidatePixel>& candidates
        , std::span<const std::uint8_t> accepted_by_label
        , int width
        , int height
        , const domain::AutoMarkerParameters& parameters
        , domain::MarkerProposalDiagnostics& diagnostics
    ) {
        assert(width > 0);
        assert(height > 0);
        assert(domain::pixel_count(width, height) == accepted_by_label.size());
        assert(domain::is_valid(parameters));

        std::vector<std::uint8_t> candidate_is_safe(candidates.size(), 1U);
        if (parameters.minimum_boundary_distance <= 0) {
            return candidate_is_safe;
        }

        const double minimum_squared_distance =
            static_cast<double>(parameters.minimum_boundary_distance)
            * static_cast<double>(parameters.minimum_boundary_distance);

        for (const domain::SeedLabel label : {
                 domain::SeedLabel::Background
                 , domain::SeedLabel::Object
             }
        ) {
            std::vector<std::uint8_t> complement_mask(accepted_by_label.size(), 0U);
            for (std::size_t pixel_index = 0;
                 pixel_index < accepted_by_label.size();
                 ++pixel_index
            ) {
                complement_mask[pixel_index] =
                    accepted_by_label[pixel_index] == static_cast<std::uint8_t>(label)
                    ? 0U
                    : 1U;
            }

            const std::vector<double> squared_distances =
                algorithm::squared_euclidean_distance_transform(
                    width
                    , height
                    , complement_mask
                );

            for (std::size_t candidate_index = 0;
                 candidate_index < candidates.size();
                 ++candidate_index
            ) {
                const CandidatePixel& candidate = candidates[candidate_index];
                if (candidate.label != label) {
                    continue;
                }

                const double squared_distance =
                    squared_distances[static_cast<std::size_t>(
                        flatten(candidate.position, width)
                    )];
                if (squared_distance >= minimum_squared_distance) {
                    continue;
                }

                candidate_is_safe[candidate_index] = 0U;
                ++diagnostics.rejected_boundary_distance_count;
            }
        }

        return candidate_is_safe;
    }

    inline void append_clean_component_markers(
        domain::MarkerLabelMask& mask
        , const std::vector<CandidatePixel>& candidates
        , const std::vector<int>& component_indices
        , const domain::AutoMarkerParameters& parameters
        , domain::MarkerProposalDiagnostics& diagnostics
    ) {
        assert(!component_indices.empty());
        assert(domain::is_valid(parameters));

        if (component_indices.size()
            < static_cast<std::size_t>(parameters.minimum_component_area)
        ) {
            diagnostics.rejected_small_component_count += component_indices.size();
            return;
        }

        for (const int candidate_index : component_indices) {
            assert(candidate_index >= 0);
            assert(static_cast<std::size_t>(candidate_index) < candidates.size());
            const CandidatePixel& candidate =
                candidates[static_cast<std::size_t>(candidate_index)];

            mask.set(
                candidate.position.y
                , candidate.position.x
                , domain::marker_label_from_seed_label(candidate.label)
            );
        }
    }

    [[nodiscard]] inline domain::MarkerLabelMask clean_candidates(
        const std::vector<CandidatePixel>& candidates
        , const domain::GrayImage& image
        , const domain::AutoMarkerParameters& parameters
        , domain::MarkerProposalDiagnostics& diagnostics
    ) {
        assert(!image.empty());
        assert(domain::is_valid(parameters));

        const int width = image.width();
        const int height = image.height();
        const std::size_t image_pixel_count = domain::pixel_count(width, height);
        std::vector<int> candidate_index_by_pixel(image_pixel_count, -1);
        std::vector<std::uint8_t> accepted_by_label(image_pixel_count, 255U);

        for (std::size_t candidate_index = 0;
             candidate_index < candidates.size();
             ++candidate_index
        ) {
            const CandidatePixel& candidate = candidates[candidate_index];
            const int pixel_index = flatten(candidate.position, width);
            candidate_index_by_pixel[static_cast<std::size_t>(pixel_index)] =
                static_cast<int>(candidate_index);
            accepted_by_label[static_cast<std::size_t>(pixel_index)] =
                static_cast<std::uint8_t>(candidate.label);
        }

        const std::vector<std::uint8_t> candidate_is_safe = distance_safe_candidates(
            candidates
            , accepted_by_label
            , width
            , height
            , parameters
            , diagnostics
        );

        std::vector<std::uint8_t> visited(candidates.size(), 0U);
        domain::MarkerLabelMask mask(width, height);

        constexpr int kNeighborCount = 4;
        constexpr int dx[kNeighborCount] = {1, -1, 0, 0};
        constexpr int dy[kNeighborCount] = {0, 0, 1, -1};

        for (std::size_t start_index = 0;
             start_index < candidates.size();
             ++start_index
        ) {
            if (candidate_is_safe[start_index] == 0U) {
                continue;
            }
            if (visited[start_index] != 0U) {
                continue;
            }

            std::vector<int> component_indices;
            std::deque<int> queue;
            visited[start_index] = 1U;
            queue.push_back(static_cast<int>(start_index));

            while (!queue.empty()) {
                const int current_index = queue.front();
                queue.pop_front();
                component_indices.push_back(current_index);

                const CandidatePixel& current =
                    candidates[static_cast<std::size_t>(current_index)];
                for (int offset_index = 0; offset_index < kNeighborCount; ++offset_index) {
                    const domain::PixelCoordinate neighbor {
                        .x = current.position.x + dx[offset_index]
                        , .y = current.position.y + dy[offset_index]
                    };
                    if (!is_inside(neighbor, width, height)) {
                        continue;
                    }

                    const int neighbor_pixel_index = flatten(neighbor, width);
                    const int neighbor_candidate_index = candidate_index_by_pixel[
                        static_cast<std::size_t>(neighbor_pixel_index)
                    ];
                    if (neighbor_candidate_index < 0) {
                        continue;
                    }
                    if (visited[static_cast<std::size_t>(neighbor_candidate_index)] != 0U) {
                        continue;
                    }
                    if (candidate_is_safe[static_cast<std::size_t>(neighbor_candidate_index)]
                        == 0U
                    ) {
                        continue;
                    }
                    if (candidates[static_cast<std::size_t>(neighbor_candidate_index)].label
                        != current.label
                    ) {
                        continue;
                    }

                    visited[static_cast<std::size_t>(neighbor_candidate_index)] = 1U;
                    queue.push_back(neighbor_candidate_index);
                }
            }

            append_clean_component_markers(
                mask
                , candidates
                , component_indices
                , parameters
                , diagnostics
            );
        }

        return mask;
    }

    [[nodiscard]] inline bool contains_manual_seed(
        domain::PixelCoordinate position
        , std::span<const domain::SeedRegion> manual_regions
    ) noexcept {
        return std::any_of(
            manual_regions.begin()
            , manual_regions.end()
            , [position](const domain::SeedRegion& region) {
                if (region.source != domain::SeedSource::User) {
                    return false;
                }
                return position.x >= region.area.x
                    && position.y >= region.area.y
                    && position.x < region.area.x + region.area.width
                    && position.y < region.area.y + region.area.height;
            }
        );
    }

    [[nodiscard]] inline domain::MarkerProposal remove_manual_conflicts(
        domain::MarkerProposal proposal
        , std::span<const domain::SeedRegion> manual_regions
    ) {
        for (int row = 0; row < proposal.mask.height(); ++row) {
            for (int column = 0; column < proposal.mask.width(); ++column) {
                if (!domain::is_seed_marker_label(proposal.mask.at(row, column))) {
                    continue;
                }

                if (contains_manual_seed({.x = column, .y = row}, manual_regions)) {
                    proposal.mask.set(row, column, domain::MarkerLabel::None);
                    ++proposal.diagnostics.rejected_manual_conflict_count;
                }
            }
        }

        proposal.diagnostics.proposed_seed_count = proposal.mask.seed_count();
        return proposal;
    }

    [[nodiscard]] inline MarkerProposalOutcome propose_gmm_markers(
        const domain::GrayImage& image
        , const domain::AutoMarkerParameters& parameters
    ) {
        if (image.empty()) {
            return MarkerProposalError::EmptyImage;
        }
        if (!domain::is_valid(parameters)) {
            return MarkerProposalError::InvalidParameters;
        }

        const std::vector<double> samples = image_intensity_samples(image);
        const GmmFitOutcome fit_outcome = fit_gmm(samples, parameters.gmm);
        if (!std::holds_alternative<GmmModel1d>(fit_outcome)) {
            return MarkerProposalError::GmmFitFailed;
        }
        const GmmModel1d& model = std::get<GmmModel1d>(fit_outcome);

        domain::MarkerProposalDiagnostics diagnostics {
            .gmm_iterations = static_cast<std::size_t>(model.iterations)
            , .gmm_converged = model.converged
        };
        const std::vector<CandidatePixel> candidates = high_confidence_candidates(
            image
            , model
            , parameters
            , diagnostics
        );
        domain::MarkerProposal proposal {
            .mask = clean_candidates(candidates, image, parameters, diagnostics)
            , .diagnostics = diagnostics
        };
        proposal.diagnostics.proposed_seed_count = proposal.mask.seed_count();
        return proposal;
    }
}
