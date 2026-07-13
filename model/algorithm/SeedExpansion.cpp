#include "SeedExpansion.hpp"

#include "IterationPolicy.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace random_walker::algorithm {
    namespace {
        using SeedCountOutcome = std::variant<std::size_t, domain::Cancelled>;

        [[nodiscard]] std::size_t seed_pixel_count(
            const domain::SeedRegion& region
        ) noexcept {
            assert(region.area.width > 0);
            assert(region.area.height > 0);
            return static_cast<std::size_t>(region.area.width)
                * static_cast<std::size_t>(region.area.height);
        }

        [[nodiscard]] SeedCountOutcome count_seed_pixels(
            std::span<const domain::SeedRegion> regions
            , const domain::CancellationToken& cancellation
        ) {
            std::size_t result = 0;
            for (const domain::SeedRegion& region : regions) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                result += seed_pixel_count(region);
            }

            return result;
        }

        [[nodiscard]] std::size_t flatten(
            int row
            , int column
            , int width
        ) noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(width > 0);
            assert(column < width);
            return static_cast<std::size_t>(
                domain::linear_pixel_index(row, column, width)
            );
        }

        [[nodiscard]] domain::Seed automatic_seed(
            int row
            , int column
            , domain::MarkerLabel label
        ) noexcept {
            assert(domain::is_seed_marker_label(label));
            return {
                .position = {.x = column, .y = row}
                , .label = domain::seed_label_from_marker_label(label)
                , .source = domain::SeedSource::Automatic
                , .confidence = 1.0
            };
        }

        [[nodiscard]] bool has_expected_marker_mask_geometry(
            const domain::MarkerLabelMask& mask
            , int image_width
            , int image_height
        ) noexcept {
            return mask.empty()
                || (mask.width() == image_width
                    && mask.height() == image_height);
        }

        [[nodiscard]] bool is_region_inside_image(
            const domain::SeedRegion& region
            , int image_width
            , int image_height
        ) noexcept {
            const domain::PixelRectangle& area = region.area;
            return area.width > 0
                && area.height > 0
                && area.x >= 0
                && area.y >= 0
                && area.x <= image_width - area.width
                && area.y <= image_height - area.height;
        }

        void mark_manual_seed_pixels(
            std::vector<std::uint8_t>& manual_seed_pixels
            , std::span<const domain::SeedRegion> regions
            , int image_width
            , int image_height
        ) {
            assert(image_width > 0);
            assert(image_height > 0);
            assert(manual_seed_pixels.size() == domain::pixel_count(
                image_width
                , image_height
            ));

            for (const domain::SeedRegion& region : regions) {
                assert(is_region_inside_image(
                    region
                    , image_width
                    , image_height
                ));
                const domain::PixelRectangle& area = region.area;
                for (int row = area.y; row < area.y + area.height; ++row) {
                    for (int column = area.x;
                         column < area.x + area.width;
                         ++column
                    ) {
                        manual_seed_pixels[flatten(row, column, image_width)] =
                            std::uint8_t {1};
                    }
                }
            }
        }

        [[nodiscard]] SeedCountOutcome count_constraint_seed_pixels(
            const domain::SegmentationConstraints& constraints
            , const std::vector<std::uint8_t>& manual_seed_pixels
            , int image_width
            , int image_height
            , const domain::CancellationToken& cancellation
        ) {
            assert(has_expected_marker_mask_geometry(
                constraints.automatic_markers
                , image_width
                , image_height
            ));

            SeedCountOutcome manual_count_outcome = count_seed_pixels(
                constraints.manual_seed_regions
                , cancellation
            );
            if (std::holds_alternative<domain::Cancelled>(
                    manual_count_outcome
                )
            ) {
                return domain::Cancelled {};
            }

            std::size_t result = std::get<std::size_t>(manual_count_outcome);
            const domain::MarkerLabelMask& mask =
                constraints.automatic_markers;
            if (mask.empty()) {
                return result;
            }

            for (int row = 0; row < mask.height(); ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (int column = 0; column < mask.width(); ++column) {
                    if (should_poll_cancellation(static_cast<std::size_t>(column))
                        && cancellation.stop_requested()
                    ) {
                        return domain::Cancelled {};
                    }

                    if (manual_seed_pixels[flatten(row, column, image_width)]
                        != std::uint8_t {0}
                    ) {
                        continue;
                    }

                    if (domain::is_seed_marker_label(mask.at(row, column))) {
                        ++result;
                    }
                }
            }

            return result;
        }

        void report_expansion_progress(
            std::size_t expanded_count
            , std::size_t seed_count
            , const domain::ProgressReporter& progress
        ) {
            progress.report(
                domain::SegmentationStage::ExpandingSeeds
                , seed_count == 0
                    ? 1.0
                    : static_cast<double>(expanded_count)
                        / static_cast<double>(seed_count)
            );
        }
    }

    SeedExpansionOutcome expand_seed_constraints(
        const domain::SegmentationConstraints& constraints
        , int image_width
        , int image_height
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(image_width > 0);
        assert(image_height > 0);
        assert(has_expected_marker_mask_geometry(
            constraints.automatic_markers
            , image_width
            , image_height
        ));

        std::vector<std::uint8_t> manual_seed_pixels(
            domain::pixel_count(image_width, image_height)
            , std::uint8_t {0}
        );
        mark_manual_seed_pixels(
            manual_seed_pixels
            , constraints.manual_seed_regions
            , image_width
            , image_height
        );

        const SeedCountOutcome seed_count_outcome =
            count_constraint_seed_pixels(
                constraints
                , manual_seed_pixels
                , image_width
                , image_height
                , cancellation
            );
        if (std::holds_alternative<domain::Cancelled>(seed_count_outcome)) {
            return domain::Cancelled {};
        }
        const std::size_t seed_count = std::get<std::size_t>(
            seed_count_outcome
        );

        std::vector<domain::Seed> result;
        result.reserve(seed_count);
        std::size_t expanded_count = 0;
        progress.report(domain::SegmentationStage::ExpandingSeeds, 0.0);

        for (const domain::SeedRegion& region :
            constraints.manual_seed_regions
        ) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            assert(is_region_inside_image(region, image_width, image_height));

            const domain::PixelRectangle& area = region.area;
            for (int row = area.y; row < area.y + area.height; ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (int column = area.x;
                     column < area.x + area.width;
                     ++column
                ) {
                    if (should_poll_cancellation(static_cast<std::size_t>(column))
                        && cancellation.stop_requested()
                    ) {
                        return domain::Cancelled {};
                    }

                    result.push_back({
                        .position = {.x = column, .y = row}
                        , .label = region.label
                        , .source = region.source
                        , .confidence = region.confidence
                    });
                    ++expanded_count;
                }

                report_expansion_progress(
                    expanded_count
                    , seed_count
                    , progress
                );
            }
        }

        const domain::MarkerLabelMask& mask = constraints.automatic_markers;
        if (!mask.empty()) {
            for (int row = 0; row < mask.height(); ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (int column = 0; column < mask.width(); ++column) {
                    if (should_poll_cancellation(static_cast<std::size_t>(column))
                        && cancellation.stop_requested()
                    ) {
                        return domain::Cancelled {};
                    }

                    if (manual_seed_pixels[flatten(row, column, image_width)]
                        != std::uint8_t {0}
                    ) {
                        continue;
                    }

                    const domain::MarkerLabel label = mask.at(row, column);
                    if (!domain::is_seed_marker_label(label)) {
                        continue;
                    }

                    result.push_back(automatic_seed(row, column, label));
                    ++expanded_count;
                }

                report_expansion_progress(
                    expanded_count
                    , seed_count
                    , progress
                );
            }
        }

        assert(result.size() == seed_count);
        progress.report(domain::SegmentationStage::ExpandingSeeds, 1.0);
        return result;
    }
}
