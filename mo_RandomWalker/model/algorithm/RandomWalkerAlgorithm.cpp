#include "RandomWalkerAlgorithm.hpp"

#include "../graph/PixelGraph.hpp"

#include <Eigen/SparseCholesky>

#include <QDebug>

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <ranges>

namespace algorithm
{

    using Eigen::MatrixXd;
    using Eigen::MatrixXi;
    using Eigen::Matrix;
    using Eigen::VectorXd;
    using Eigen::SparseMatrix;
    using graph::PixelGraph;

    namespace
    {
        _pure_(int flatten(int row, int col, int width))
        {
            return row * width + col;
        }

        [[nodiscard]] std::pair<MatrixXi, std::unordered_set<int>> build_label_matrix(
            const std::vector<QPoint>& background
            , const std::vector<QPoint>& object
            , int height, int width) noexcept
        {
            MatrixXi labels = MatrixXi::Zero(height, width);
            std::unordered_set<int> labeled;

            for (const QPoint& p : background)
            {
                labels(p.y(), p.x()) = 0;
                labeled.insert(flatten(p.y(), p.x(), width));
            }
            for (const QPoint& p : object)
            {
                labels(p.y(), p.x()) = 1;
                labeled.insert(flatten(p.y(), p.x(), width));
            }

            return { labels, labeled };
        }

        [[nodiscard]] std::pair<std::vector<int>, std::unordered_map<int, int>> extract_unlabeled_indices(
            int N
            , const std::unordered_set<int>& labeled) noexcept
        {
            std::vector<int> unlabeled;
            std::unordered_map<int, int> index_map;

            for (size_t i = 0; i < N; ++i)
            {
                if (!labeled.contains(i))
                {
                    int pos = static_cast<int>(unlabeled.size());
                    unlabeled.push_back(i);
                    index_map[i] = pos;
                }
            }
            return { unlabeled, index_map };
        }

        [[nodiscard]] std::pair<SparseMatrix<double>, SparseMatrix<double>> split_laplacian(
            const SparseMatrix<double>& L
            , const std::unordered_set<int>& labeled
            , const std::unordered_map<int, int>& label_index
            , const std::unordered_map<int, int>& unlabeled_index) noexcept
        {
            const int n_u = static_cast<int>(unlabeled_index.size());
            const int n_l = static_cast<int>(label_index.size());

            SparseMatrix<double> L_uu(n_u, n_u);
            SparseMatrix<double> L_ul(n_u, n_l);

            std::vector<Eigen::Triplet<double>> triplets_uu, triplets_ul;

            for (size_t k = 0; k < L.outerSize(); ++k)
            {
                for (SparseMatrix<double>::InnerIterator it(L, k); it; ++it)
                {
                    int i = it.row();
                    int j = it.col();
                    double val = it.value();

                    const bool i_u = unlabeled_index.contains(i);
                    const bool j_u = unlabeled_index.contains(j);
                    const bool j_l = label_index.contains(j);

                    if (i_u && j_u)
                        triplets_uu.emplace_back(unlabeled_index.at(i), unlabeled_index.at(j), val);
                    else if (i_u && j_l)
                        triplets_ul.emplace_back(unlabeled_index.at(i), label_index.at(j), val);
                }
            }

            L_uu.setFromTriplets(triplets_uu.begin(), triplets_uu.end());
            L_ul.setFromTriplets(triplets_ul.begin(), triplets_ul.end());
            return { L_uu, L_ul };
        }

        _impure_(VectorXd build_rhs_vector(
            const MatrixXi& labels
            , const std::vector<int>& label_vec
            , int width))
        {
            const int n_l = static_cast<int>(label_vec.size());
            VectorXd x_l(n_l);
            for (size_t i = 0; i < n_l; ++i)
            {
                const int index = label_vec[i];
                const int row = index / width;
                const int col = index % width;
                x_l[i] = labels(row, col);
            }
            return x_l;
        }

        _impure_throwing_(VectorXd solve_sparse_system(
            const SparseMatrix<double>& L_uu
            , const SparseMatrix<double>& L_ul
            , const VectorXd& x_l))
        {
            Eigen::SimplicialLLT<SparseMatrix<double>> solver;
            solver.compute(L_uu);
            if (solver.info() != Eigen::Success) {
                qDebug() << "[RW] ERROR: Failed to decompose L_uu!";
                return VectorXd::Zero(L_uu.rows());
            }
            auto result = solver.solve(-L_ul * x_l);
            return result;
        }

         Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> assemble_segmentation(
            const MatrixXi& labels
            , const std::unordered_set<int>& labeled_indices
            , const std::unordered_map<int, int>& unlabeled_index
            , const VectorXd& x_u
            , int width
            , int height)
        {
            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> result(height, width);
            const int N = width * height;

            for (size_t i = 0; i < N; ++i)
            {
                int row = i / width;
                int col = i % width;

                if (labeled_indices.contains(i))
                {
                    result(row, col) = labels(row, col);
                }
                else
                {
                    const int u_idx = unlabeled_index.at(i);
                    result(row, col) = x_u[u_idx];
                }
            }

            return result;
        }

    } // namespace


    Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> run_random_walker(
            const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image
            , const std::vector<QPoint>& background
            , const std::vector<QPoint>& object)
    {
        return (run_random_walker_probabilities(image, background, object).array() >= 0.5).cast<uint8_t>();
    }

    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> run_random_walker_probabilities(
        const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image
        , const std::vector<QPoint>& background_seeds
        , const std::vector<QPoint>& object_seeds)
    {
        const int height = static_cast<int>(image.rows());
        const int width = static_cast<int>(image.cols());
        const int N = width * height;

        qDebug() << "[RW] Image size: " << width << "x" << height << ", total pixels: " << N;

        PixelGraph graph(image);
        const SparseMatrix<double> L = graph.laplacian();

        const auto [labels, labeled_indices] = build_label_matrix(background_seeds, object_seeds, height, width);
        qDebug() << "[RW] Background seeds: " << background_seeds.size()
            << ", Object seeds: " << object_seeds.size();
        qDebug() << "[RW] Labeled pixels: " << labeled_indices.size();

        const auto [unlabeled_indices, unlabeled_index] = extract_unlabeled_indices(N, labeled_indices);
        qDebug() << "[RW] Unlabeled pixels: " << unlabeled_indices.size();

        const std::vector<int> label_vec(labeled_indices.begin(), labeled_indices.end());
        std::unordered_map<int, int> label_index;
        for (size_t i = 0; i < static_cast<int>(label_vec.size()); ++i)
            label_index[label_vec[i]] = i;

        const auto [L_uu, L_ul] = split_laplacian(L, labeled_indices, label_index, unlabeled_index);
        const VectorXd x_l = build_rhs_vector(labels, label_vec, width);
        const VectorXd x_u = solve_sparse_system(L_uu, L_ul, x_l);
        if (!x_u.allFinite()) {
            qDebug() << "[RW] ERROR: x_u contains invalid values!";
        }

        const double min_val = x_u.minCoeff();
        const double max_val = x_u.maxCoeff();
        const double avg_val = x_u.mean();

        qDebug() << "[RW] x_u stats: min = " << min_val
            << ", max = " << max_val
            << ", mean = " << avg_val;

        auto result = assemble_segmentation(labels, labeled_indices, unlabeled_index, x_u, width, height);

        int count_zeros = (result.array() == 0).count();
        int count_ones = (result.array() == 1).count();

        qDebug() << "[RW] Result matrix: zeros = " << count_zeros
            << " , ones = " << count_ones;

        return result;
    }

} // namespace algorithm
