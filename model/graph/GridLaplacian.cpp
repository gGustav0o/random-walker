#include "GridLaplacian.hpp"

#include "EdgeWeight.hpp"
#include "model/algorithm/IndexTypes.hpp"
#include "model/algorithm/IterationPolicy.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <Eigen/Core>

namespace random_walker::graph {
    namespace {
        enum class ForwardGridDirection {
            Right
            , Down
            , DownRight
            , DownLeft
        };

        struct GridGraphBuilder {
            std::vector<Eigen::Triplet<double>> triplets;
            Eigen::VectorXd degrees;
        };

        using GridBuildStepOutcome = std::optional<domain::Cancelled>;

        constexpr std::array<ForwardGridDirection, 2> kFourConnectedDirections = {
            ForwardGridDirection::Right
            , ForwardGridDirection::Down
        };

        constexpr std::array<ForwardGridDirection, 4> kEightConnectedDirections = {
            ForwardGridDirection::Right
            , ForwardGridDirection::Down
            , ForwardGridDirection::DownRight
            , ForwardGridDirection::DownLeft
        };

        void assert_grid_laplacian_preconditions(
            const domain::GrayImage& image
            , const domain::RandomWalkerParameters& parameters
        ) {
            assert(!image.empty());
            assert(domain::is_valid(parameters));
            assert(image.height() > 0);
            assert(image.width() > 0);
        }

        [[nodiscard]] int grid_pixel_count(int width, int height) noexcept {
            assert(width > 0);
            assert(height > 0);
            return width * height;
        }

        [[nodiscard]] std::size_t grid_triplet_capacity(
            int pixel_count
            , domain::PixelConnectivity connectivity
        ) noexcept {
            assert(pixel_count > 0);
            assert(domain::is_valid(connectivity));
            const std::size_t max_triplets_per_pixel =
                connectivity == domain::PixelConnectivity::Eight ? 9U : 5U;
            return static_cast<std::size_t>(pixel_count) * max_triplets_per_pixel;
        }

        [[nodiscard]] double graph_build_progress(
            int row
            , int height
        ) noexcept {
            assert(row >= 0);
            assert(height > 0);
            return static_cast<double>(row + 1) / static_cast<double>(height);
        }

        [[nodiscard]] GridGraphBuilder make_grid_graph_builder(
            int pixel_count
            , domain::PixelConnectivity connectivity
        ) {
            GridGraphBuilder result {
                .triplets = {}
                , .degrees = Eigen::VectorXd::Zero(pixel_count)
            };
            result.triplets.reserve(
                grid_triplet_capacity(pixel_count, connectivity)
            );
            return result;
        }

        [[nodiscard]] std::pair<int, int> offset(
            ForwardGridDirection direction
        ) noexcept {
            switch (direction) {
            case ForwardGridDirection::Right:
                return { 0, 1 };
            case ForwardGridDirection::Down:
                return { 1, 0 };
            case ForwardGridDirection::DownRight:
                return { 1, 1 };
            case ForwardGridDirection::DownLeft:
                return { 1, -1 };
            }

            assert(false && "Unhandled grid direction");
            return { 0, 0 };
        }

        [[nodiscard]] double edge_distance(
            ForwardGridDirection direction
        ) noexcept {
            switch (direction) {
            case ForwardGridDirection::Right:
            case ForwardGridDirection::Down:
                return 1.0;
            case ForwardGridDirection::DownRight:
            case ForwardGridDirection::DownLeft:
                return std::sqrt(2.0);
            }

            assert(false && "Unhandled grid direction");
            return 1.0;
        }

        [[nodiscard]] std::span<const ForwardGridDirection> forward_directions_for(
            domain::PixelConnectivity connectivity
        ) noexcept {
            switch (connectivity) {
            case domain::PixelConnectivity::Four:
                return kFourConnectedDirections;
            case domain::PixelConnectivity::Eight:
                return kEightConnectedDirections;
            }

            assert(false && "Unhandled pixel connectivity");
            return kFourConnectedDirections;
        }

        [[nodiscard]] constexpr bool is_inside(
            int row
            , int column
            , int height
            , int width
        ) noexcept {
            return row >= 0 && row < height && column >= 0 && column < width;
        }

        void add_undirected_edge(
            GridGraphBuilder& builder
            , algorithm::PixelIndex first_index
            , algorithm::PixelIndex second_index
            , double weight
        ) {
            assert(first_index.value >= 0);
            assert(static_cast<Eigen::Index>(first_index.value)
                < builder.degrees.size());
            assert(second_index.value >= 0);
            assert(static_cast<Eigen::Index>(second_index.value)
                < builder.degrees.size());
            assert(std::isfinite(weight));
            assert(weight >= 0.0);

            builder.triplets.emplace_back(
                first_index.value
                , second_index.value
                , -weight
            );
            builder.triplets.emplace_back(
                second_index.value
                , first_index.value
                , -weight
            );
            builder.degrees[first_index.value] += weight;
            builder.degrees[second_index.value] += weight;
        }

        template<EdgeWeightFunction WeightFunction>
        void add_forward_edges_for_pixel(
            GridGraphBuilder& builder
            , const domain::GrayImage& image
            , int row
            , int column
            , domain::PixelConnectivity connectivity
            , const WeightFunction& edge_weight
        ) {
            const int height = image.height();
            const int width = image.width();
            const int pixel_count = grid_pixel_count(width, height);

            const algorithm::PixelIndex source_index =
                algorithm::flatten_pixel_index(row, column, width);
            assert(source_index.value >= 0);
            assert(source_index.value < pixel_count);
            const std::uint8_t source_intensity = image.at(row, column);

            for (const ForwardGridDirection direction :
                forward_directions_for(connectivity)) {
                const auto [row_offset, column_offset] = offset(direction);
                const int neighbor_row = row + row_offset;
                const int neighbor_column = column + column_offset;

                if (!is_inside(neighbor_row, neighbor_column, height, width)) {
                    continue;
                }

                const algorithm::PixelIndex target_index =
                    algorithm::flatten_pixel_index(
                        neighbor_row
                        , neighbor_column
                        , width
                    );
                assert(target_index.value >= 0);
                assert(target_index.value < pixel_count);
                const double weight = edge_weight(
                    source_intensity
                    , image.at(neighbor_row, neighbor_column)
                    , edge_distance(direction)
                );

                add_undirected_edge(builder, source_index, target_index, weight);
            }
        }

        template<EdgeWeightFunction WeightFunction>
        [[nodiscard]] GridBuildStepOutcome add_grid_edges(
            GridGraphBuilder& builder
            , const domain::GrayImage& image
            , domain::PixelConnectivity connectivity
            , const WeightFunction& edge_weight
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            const int height = image.height();
            const int width = image.width();

            progress.report(domain::SegmentationStage::BuildingGraph, 0.0);
            for (int row = 0; row < height; ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                for (int column = 0; column < width; ++column) {
                    if (algorithm::should_poll_cancellation(
                            static_cast<std::size_t>(column)
                        )
                        && cancellation.stop_requested()
                    ) {
                        return domain::Cancelled {};
                    }

                    add_forward_edges_for_pixel(
                        builder
                        , image
                        , row
                        , column
                        , connectivity
                        , edge_weight
                    );
                }

                progress.report(
                    domain::SegmentationStage::BuildingGraph
                    , graph_build_progress(row, height)
                );
            }

            return std::nullopt;
        }

        [[nodiscard]] GridBuildStepOutcome add_laplacian_diagonal(
            GridGraphBuilder& builder
            , const domain::CancellationToken& cancellation
        ) {
            const int degree_count = static_cast<int>(builder.degrees.size());
            assert(static_cast<Eigen::Index>(degree_count) == builder.degrees.size());

            for (int index = 0; index < degree_count; ++index) {
                if (
                    algorithm::should_poll_cancellation(
                        static_cast<std::size_t>(index)
                    )
                    && cancellation.stop_requested()
                ) {
                    return domain::Cancelled {};
                }

                const algorithm::PixelIndex diagonal_index { .value = index };
                assert(diagonal_index.value >= 0);
                assert(static_cast<Eigen::Index>(diagonal_index.value)
                    < builder.degrees.size());
                builder.triplets.emplace_back(
                    diagonal_index.value
                    , diagonal_index.value
                    , builder.degrees[diagonal_index.value]
                );
            }

            return std::nullopt;
        }

        [[nodiscard]] Eigen::SparseMatrix<double> assemble_laplacian(
            const GridGraphBuilder& builder
            , int pixel_count
        ) {
            Eigen::SparseMatrix<double> laplacian(pixel_count, pixel_count);
            laplacian.setFromTriplets(
                builder.triplets.begin()
                , builder.triplets.end()
            );
            assert(laplacian.rows() == pixel_count);
            assert(laplacian.cols() == pixel_count);
            return laplacian;
        }
    }

    GridLaplacianOutcome build_grid_laplacian(
        const domain::GrayImage& image
        , const domain::RandomWalkerParameters& parameters
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert_grid_laplacian_preconditions(image, parameters);

        const int height = image.height();
        const int width = image.width();
        const int pixel_count = grid_pixel_count(width, height);
        GridGraphBuilder builder = make_grid_graph_builder(
            pixel_count
            , parameters.connectivity
        );
        const ExponentialDistanceEdgeWeight edge_weight {
            .parameters = edge_weight_parameters_from(parameters)
        };

        if (const auto cancelled = add_grid_edges(
                builder
                , image
                , parameters.connectivity
                , edge_weight
                , cancellation
                , progress
            ); cancelled.has_value()
        ) {
            return *cancelled;
        }

        if (const auto cancelled = add_laplacian_diagonal(
                builder
                , cancellation
            ); cancelled.has_value()
        ) {
            return *cancelled;
        }

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        Eigen::SparseMatrix<double> laplacian = assemble_laplacian(
            builder
            , pixel_count
        );

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report(domain::SegmentationStage::BuildingGraph, 1.0);
        return laplacian;
    }
}
