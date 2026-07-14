#include "HarmonicSolver.hpp"

#include <cassert>
#include <variant>

#include <Eigen/SparseCholesky>

#include "DirichletProblemValidation.hpp"

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
            const DirichletAnchoringOutcome anchoring_outcome =
                validate_dirichlet_anchoring(laplacian, cancellation);
            if (const auto* error =
                    std::get_if<domain::SegmentationError>(&anchoring_outcome)
            ) {
                return *error;
            }
            if (std::holds_alternative<domain::Cancelled>(anchoring_outcome)) {
                return domain::Cancelled {};
            }

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
