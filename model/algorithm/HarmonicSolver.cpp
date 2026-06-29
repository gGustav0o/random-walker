#include "HarmonicSolver.hpp"

#include <cassert>

#include <Eigen/SparseCholesky>

namespace random_walker::algorithm {
    HarmonicSolveOutcome solve_harmonic_dirichlet_problem(
        const PartitionedLaplacian& laplacian
        , const Eigen::VectorXd& boundary_values
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(
            laplacian.unknown_unknown_block.rows()
            == laplacian.unknown_unknown_block.cols()
        );
        assert(
            laplacian.unknown_boundary_block.rows()
            == laplacian.unknown_unknown_block.rows()
        );
        assert(
            laplacian.unknown_boundary_block.cols()
            == boundary_values.size()
        );

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report_indeterminate(domain::SegmentationStage::Factorizing);
        Eigen::SimplicialLLT<Eigen::SparseMatrix<double>> solver;
        solver.compute(laplacian.unknown_unknown_block);

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }
        if (solver.info() != Eigen::Success) {
            return domain::SegmentationError::LaplacianDecompositionFailed;
        }

        progress.report_indeterminate(
            domain::SegmentationStage::Solving
        );
        Eigen::VectorXd solution = solver.solve(
            -laplacian.unknown_boundary_block * boundary_values
        );

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }
        if (solver.info() != Eigen::Success) {
            return domain::SegmentationError::LinearSystemSolveFailed;
        }
        if (!solution.allFinite()) {
            return domain::SegmentationError::NonFiniteSolution;
        }

        assert(solution.size() == laplacian.unknown_unknown_block.rows());
        return solution;
    }
}
