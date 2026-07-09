# Random Walker Segmentation Algorithm

## 1. Input Model

Let the input grayscale image be a function

$$
I : \Omega \to \{0,1,\dots,255\},
$$

where

$$
\Omega = \{0,\dots,H-1\} \times \{0,\dots,W-1\}.
$$

Implementation: `domain::GrayImage`.

For a pixel $s=(r,c)\in\Omega$, its intensity is denoted by $I_s$. Graph-level coordinates use `(row, column)`. The flat pixel index is

$$
\operatorname{idx}(r,c)=rW+c.
$$

The current segmentation problem is binary:

$$
\mathcal L = \{\mathrm{Background},\mathrm{Object}\}.
$$

Boundary values are encoded as

$$
\mathrm{Background} \mapsto 0,
\qquad
\mathrm{Object} \mapsto 1.
$$

Implementation: `build_boundary_conditions`.

## 2. Image Graph

The image is converted into a weighted undirected graph

$$
G=(V,E,w),
$$

where

$$
V = \{v_s \mid s\in\Omega\}.
$$

Implementation: `build_grid_laplacian`.

### 2.1. 4-Connectivity

For 4-connectivity, each pixel is connected to its orthogonal neighbors:

$$
\mathcal N_4(r,c)=\{(r,c+1),(r+1,c),(r,c-1),(r-1,c)\}\cap\Omega.
$$

Implementation: `PixelConnectivity::Four`.

During graph assembly, only forward directions `Right` and `Down` are enumerated. Each discovered edge is added as an undirected edge: both off-diagonal Laplacian entries receive $-w_{ij}$, and both endpoint degrees are incremented by $w_{ij}$.

Implementation: `add_undirected_edge`.

### 2.2. 8-Connectivity

For 8-connectivity, diagonal neighbors are added:

$$
\mathcal N_8(r,c)=\mathcal N_4(r,c)\cup
\{(r+1,c+1),(r+1,c-1),(r-1,c+1),(r-1,c-1)\}\cap\Omega.
$$

Implementation: `PixelConnectivity::Eight`.

The edge length is

$$
d_{ij}=\begin{cases}
1, & \text{orthogonal edge},\
\sqrt 2, & \text{diagonal edge}.
\end{cases}
$$

Implementation: `edge_distance`.

## 3. Edge Weights

An edge weight encodes local similarity between adjacent pixels. Larger weights make label propagation easier across that edge. This follows the weighted-graph formulation of Random Walker segmentation [Grady 2006](https://doi.org/10.1109/TPAMI.2006.233).

Implementation: `EdgeWeightInput`, `GlobalBetaEdgeWeight`, `LocalVarianceNormalizedEdgeWeight`.

The current project supports three edge-weight models.

## 3.1. Global Beta Weight

For an edge $(i,j)$, define

$$
\Delta I_{ij}=I_i-I_j.
$$

The global-beta edge weight is

$$
w_{ij}=\frac{\exp\left(-\beta (\Delta I_{ij})^2\right)}{d_{ij}^{p}}.
$$

Parameters:

$$
\beta \in [10^{-6},10^{-1}],
\qquad
p\in[0,2].
$$

Defaults:

$$
\beta=10^{-3},
\qquad
p=0.
$$

Implementation: `compute_global_beta_edge_weight`.


For $p=0$, geometric edge length has no effect because $d_{ij}^{0}=1$. For $p>0$, diagonal edges in 8-connectivity are penalized relative to orthogonal edges.

The factor

$$
\exp\left(-\beta (I_i-I_j)^2\right)
$$

is the classical Gaussian contrast weight used in Random Walker segmentation [Grady 2006](https://doi.org/10.1109/TPAMI.2006.233). The multiplier $d_{ij}^{-p}$ is a project-level geometric extension for distance-aware 8-connectivity.

## 3.2. Local Variance Normalized Weight

In `LocalVarianceNormalized`, the global $\beta$ parameter is not used. A local variance estimate is computed for every pixel.

For a pixel $s=(r,c)$ and window radius $R$, define the boundary-clipped square window

$$
\mathcal W_R(s)=
\{t=(u,v)\in\Omega \mid |u-r|\le R,\ |v-c|\le R\}.
$$

Implementation: `raw_local_window_variance`.

The raw local variance is the population variance

$$
\widehat\sigma^2(s)=
\frac{1}{|\mathcal W_R(s)|}\sum_{t\in\mathcal W_R(s)} I_t^2
-
\left(
\frac{1}{|\mathcal W_R(s)|}\sum_{t\in\mathcal W_R(s)} I_t
\right)^2.
$$

The effective local variance is clamped from below:

$$
\sigma^2(s)=\max\{\widehat\sigma^2(s),\sigma^2_{\min}\}.
$$

Implementation: `local_window_variance`.

For an edge $(i,j)$, the edge variance is the arithmetic mean of endpoint variances:

$$
\sigma^2_{ij}=\frac{\sigma^2(i)+\sigma^2(j)}{2}.
$$

Implementation: `edge_local_variance`.

The edge weight is

$$
w_{ij}=\frac{\exp\left(-\frac{(\Delta I_{ij})^2}{2\sigma^2_{ij}}\right)}{d_{ij}^{p}}.
$$

Implementation: `compute_local_variance_normalized_edge_weight`.

Parameters:

$$
R\in\{1,\dots,16\},
\qquad
\sigma^2_{\min}\in[10^{-6},255^2],
\qquad
p\in[0,2].
$$

The minimum variance can be manual or automatic. In automatic mode, all raw local variances are computed first. Then the quantile

$$
q_\alpha\left(\{\widehat\sigma^2(s)\mid s\in\Omega\}\right)
$$

is selected, where

$$
\alpha\in[0.01,0.50].
$$

The final minimum variance is

$$
\sigma^2_{\min}=\operatorname{clamp}
\left(q_\alpha,10^{-6},255^2\right).
$$

Implementation: `local_variance_quantile`, `estimate_minimum_variance`, `build_effective_local_contrast_scale`.

**Warning.** This is not a direct implementation of the Bhattacharyya-weight framework from [Drees et al. 2022](https://arxiv.org/abs/2206.00947). That paper motivates noise/local-statistics-aware Random Walker weights. The project implements a simpler local Gaussian normalization based on window variance. The citation is therefore a motivation and context reference, not a claim of exact implementation.


## 3.3. Edge-Local Contrast Scale Model

This model is intentionally formulated as a local scale model, not as an adaptive beta model.

For an edge $(i,j)\in E$, define the intensity jump

$$
\delta_{ij}=|I_i-I_j|.
$$

The adaptive weight is defined as

$$
w_{ij}=\frac{\exp\left(-\frac{\delta_{ij}^{2}}{2\sigma_{ij}^{2}}\right)}{d_{ij}^{p}},
$$

where $\sigma_{ij}>0$ is an edge-local contrast scale. Equivalently, a local $\beta_{ij}$ would be $\beta_{ij}=1/(2\sigma_{ij}^{2})$, but the implementation models $\sigma_{ij}$ directly because it has the statistical meaning of an expected local intra-region edge difference scale.

For a grid edge $e=(i,j)$, let

$$
m(e)=\frac{i+j}{2}
$$

be its geometric midpoint. To avoid floating-point midpoint comparisons in code, midpoint coordinates are represented in doubled coordinates:

$$
\tilde m(e)=i+j.
$$

Implementation: `GridEdge`, `doubled_midpoint`.

For radius $R$, the edge-local neighborhood of a center edge $e$ is

$$
\mathcal E_R(e)=\left\{e'\in E \mid
\left\lVert \tilde m(e')-\tilde m(e) \right\rVert_\infty \le 2R
\right\}.
$$

Implementation: `belongs_to_edge_local_neighborhood`.

The intended robust scale estimator is

$$
\sigma_{ij}=\max\left\{Q_q\left(\{\delta_{uv}\mid (u,v)\in\mathcal E_R(i,j)\}\right),\sigma_{\min}\right\},
$$

with

$$
R\in\{1,\dots,16\},\qquad
q\in[0.25,0.50],\qquad
\sigma_{\min}\in[10^{-6},255].
$$

Implementation: `estimate_edge_local_contrast_scale`, `EdgeLocalContrastScaleMap`, `compute_edge_local_contrast_normalized_edge_weight`. Parameters: `EdgeLocalContrastScaleParameters`.

This estimator uses local neighboring edge differences rather than raw intensity variance inside a pixel window. The distinction matters: a pixel-window variance can be inflated by a true object boundary crossing the window, whereas an edge-difference quantile estimates the local scale in the same measurement space that appears in the edge weight.

## 4. Graph Laplacian

The weighted graph defines a graph Laplacian

$$
L\in\mathbb R^{|V|\times|V|}.
$$

For $i\ne j$:

$$
L_{ij}=\begin{cases}
-w_{ij}, & (i,j)\in E,\
0, & (i,j)\notin E.
\end{cases}
$$

The diagonal is

$$
L_{ii}=\sum_{j:(i,j)\in E}w_{ij}.
$$

Equivalently,

$$
L=D-W,
$$

where $D$ is the degree matrix and $W$ is the weighted adjacency matrix.

Implementation: `add_undirected_edge`, `add_laplacian_diagonal`, `assemble_laplacian`.

## 5. Boundary Conditions and Node Partition

Let

$$
M\subset V
$$

be the set of marked pixels. These markers may be manual or automatic. The unknown set is

$$
U=V\setminus M.
$$

For $m\in M$:

$$
x_m=\begin{cases}
0, & m\text{ is marked as Background},\
1, & m\text{ is marked as Object}.
\end{cases}
$$

Implementation: `BoundaryConditions`.

Let $x_M$ be the vector of known boundary values and $x_U$ the vector of unknown values. After reordering nodes into unknown and boundary groups, the Laplacian is partitioned as

$$
L=
\begin{pmatrix}
L_{UU} & L_{UM}\
L_{MU} & L_{MM}
\end{pmatrix}.
$$

Implementation: `partition_nodes`, `partition_laplacian`, `PartitionedLaplacian`.

## 6. Discrete Dirichlet Problem

Random Walker segmentation can be interpreted as a first-hitting probability problem on a weighted graph. Equivalently, it is a discrete harmonic Dirichlet problem with fixed seed values [Grady 2006](https://doi.org/10.1109/TPAMI.2006.233). The connection between hitting probabilities, harmonic functions, and electrical potentials is classical; see [Doyle and Snell 1984](https://math.dartmouth.edu/~doyle/docs/walks/walks.pdf).

For unknown nodes, the system is

$$
L_{UU}x_U=-L_{UM}x_M.
$$

Implementation: `solve_harmonic_dirichlet_problem`.

The implementation solves the sparse system using `Eigen::SimplicialLLT<Eigen::SparseMatrix<double>>`, i.e. sparse Cholesky factorization for a symmetric positive definite matrix.

### 6.1. Correctness Conditions

Theoretically, $L_{UU}$ is positive definite if every unknown connected component has a path to at least one seed node through edges of positive conductance.

**Important:** topological grid connectivity alone is insufficient. In exact mathematics, exponential weights are strictly positive. In floating-point arithmetic, very small weights may underflow to numerical zero. Therefore, in practice, every unknown component must be connected to the seed set by a path whose edge weights do not vanish numerically. Otherwise Cholesky factorization may fail.

For ordinary images and reasonable parameters, this condition usually holds on a rectangular 4- or 8-connected grid with at least one seed pixel. It is not guaranteed for all parameters and all contrast configurations.

If factorization or solving fails, the algorithm reports one of the following errors:

- `LaplacianDecompositionFailed`;
- `LinearSystemSolveFailed`;
- `NonFiniteSolution`.

Implementation: `HarmonicSolveOutcome`, `SegmentationError`.

### 6.2. Fully Constrained Image

If

$$
U=\varnothing,
$$

the linear system is not solved. The probability map is assembled directly from boundary values.

Implementation: `handle_fully_constrained_image`.

## 7. Probability Map and Binary Mask

The solver produces a probability map

$$
P:\Omega\to\mathbb R.
$$

For marked pixels,

$$
P_s=x_s,\quad s\in M.
$$

For unknown pixels,

$$
P_s=(x_U)_s,\quad s\in U.
$$

In the ideal Random Walker model, $P_s\in[0,1]$ and $P_s$ is the probability of reaching an object seed before a background seed.

Implementation: `assemble_probability_map`.

The binary mask is obtained by thresholding:

$$
B_s=\begin{cases}
1, & P_s\ge 0.5,\
0, & P_s<0.5.
\end{cases}
$$

Implementation: `threshold_probabilities`.

## 8. Automatic Marker Proposal

Automatic marker placement is not part of the classical Random Walker algorithm. It is a preprocessing step that proposes high-confidence seed pixels. The user can inspect these markers before running segmentation.

Implementation: `propose_gmm_markers`.

## 8.1. Intensity Mixture Model

All image intensities are treated as samples

$$
X=\{x_n\}_{n=1}^{N},
\qquad
x_n\in\{0,\dots,255\},
\qquad
N=WH.
$$

The model is a one-dimensional two-component Gaussian mixture:

$$
p(x\mid\theta)=\sum_{k=1}^{2}\pi_k\,\mathcal N(x\mid\mu_k,\sigma_k^2),
$$

where

$$
\pi_k\ge0,
\qquad
\sum_{k=1}^{2}\pi_k=1,
\qquad
\sigma_k^2\ge\sigma^2_{\min,GMM}.
$$

Implementation: `GmmModel1d`, `GaussianComponent1d`.

The component count is fixed:

$$
K=2.
$$

Initialization:

$$
\mu_1=\min X,
\qquad
\mu_2=\max X,
\qquad
\pi_1=\pi_2=0.5.
$$

The initial variance of both components is the population variance of all samples, clamped by the GMM minimum variance.

Implementation: `initialize_gmm`.

## 8.2. EM Estimation

The GMM parameters are estimated using the Expectation-Maximization algorithm. EM estimates maximum likelihood parameters in latent-variable models, including finite mixture models [Dempster, Laird, Rubin 1977](https://doi.org/10.1111/j.2517-6161.1977.tb01600.x).

Let $z_n\in\{1,2\}$ be the latent component assignment for sample $x_n$.

E-step:

$$
\gamma_{nk}=P(z_n=k\mid x_n,\theta)=
\frac{\pi_k\mathcal N(x_n\mid\mu_k,\sigma_k^2)}
{\sum_{j=1}^{2}\pi_j\mathcal N(x_n\mid\mu_j,\sigma_j^2)}.
$$

M-step:

$$
N_k=\sum_{n=1}^{N}\gamma_{nk},
$$

$$
\pi_k=\frac{N_k}{N},
$$

$$
\mu_k=\frac{1}{N_k}\sum_{n=1}^{N}\gamma_{nk}x_n,
$$

$$
\sigma_k^2=
\max\left\{
\sigma^2_{\min,GMM},
\frac{1}{N_k}\sum_{n=1}^{N}\gamma_{nk}(x_n-\mu_k)^2
\right\}.
$$

Iterations stop when

$$
|\ell^{(t)}-\ell^{(t-1)}|\le\varepsilon,
$$

where

$$
\ell(\theta)=\sum_{n=1}^{N}\log\left(
\sum_{k=1}^{2}\pi_k\mathcal N(x_n\mid\mu_k,\sigma_k^2)
\right).
$$

A maximum iteration count is also enforced. After fitting, components are sorted by mean.

Implementation: `fit_gmm`, `log_likelihood`.

## 8.3. Foreground Component Selection

The foreground component is selected from the fitted means:

$$
k_F=\begin{cases}
\arg\min_k\mu_k, & \mathrm{DarkObject},\
\arg\max_k\mu_k, & \mathrm{BrightObject}.
\end{cases}
$$

Implementation: `foreground_component_index`.

The foreground probability for intensity $x$ is

$$
P_F(x)=P(z=k_F\mid x,\theta).
$$

The background probability is

$$
P_B(x)=1-P_F(x).
$$

Implementation: `foreground_probability`, `posterior_probabilities`.

## 8.4. High-Confidence Candidates

Let $\tau$ be the confidence threshold:

$$
\tau\in[0.5,1.0],
\qquad
\tau_{default}=0.98.
$$

An object-marker candidate is created if

$$
P_F(I_s)\ge\tau.
$$

A background-marker candidate is created if

$$
P_B(I_s)\ge\tau.
$$

Otherwise the pixel is rejected as low-confidence.

Implementation: `high_confidence_candidates`.

## 8.5. Connected Components and Erosion-Like Cleanup

Candidates are split into connected components by label. Current auto-marker cleanup uses 4-connectivity regardless of the Random Walker connectivity selected by the user.

Implementation: `clean_candidates`.

For each component $C$:

1. If

$$
|C| < A_{\min},
$$

the component is rejected as too small.

2. For each $s\in C$, define the square neighborhood

$$
\mathcal W_\rho(s)=\{t\in\Omega\mid |t_r-s_r|\le\rho,\ |t_c-s_c|\le\rho\}.
$$

The pixel survives cleanup iff

$$
\forall t\in\mathcal W_\rho(s):\quad t\in C\ \text{and}\ t\text{ has the same label as }s.
$$

If the square neighborhood crosses the image boundary, the pixel is rejected.

Implementation: `survives_erosion`, `append_clean_component_markers`.

This is equivalent to binary erosion with a square structuring element on the accepted candidate mask. Connected-component processing is historically related to [Rosenfeld and Pfaltz 1966](https://doi.org/10.1145/321356.321357).

**Theoretical warning.** GMM plus connected components plus erosion does not prove marker correctness. It is a high-confidence intensity heuristic. If object and background are not well separated by intensity, the GMM may produce confident but wrong marker proposals.

GrabCut [Rother, Kolmogorov, Blake 2004](https://www.microsoft.com/en-us/research/publication/grabcut-interactive-foreground-extraction-using-iterated-graph-cuts/) is relevant only as a related example of GMM-based foreground/background modelling in interactive segmentation. The project does not implement GrabCut and does not solve graph-cut energy.

## 9. References

1. Leo Grady. [Random Walks for Image Segmentation](https://doi.org/10.1109/TPAMI.2006.233). IEEE Transactions on Pattern Analysis and Machine Intelligence, 28(11), 1768-1783, 2006.
2. Peter G. Doyle, J. Laurie Snell. [Random Walks and Electric Networks](https://math.dartmouth.edu/~doyle/docs/walks/walks.pdf). Mathematical Association of America, 1984.
3. Dominik Drees, Florian Eilers, Ang Bian, Xiaoyi Jiang. [A Bhattacharyya Coefficient-Based Framework for Noise Model-Aware Random Walker Image Segmentation](https://arxiv.org/abs/2206.00947). arXiv:2206.00947, 2022.
4. A. P. Dempster, N. M. Laird, D. B. Rubin. [Maximum Likelihood from Incomplete Data via the EM Algorithm](https://doi.org/10.1111/j.2517-6161.1977.tb01600.x). Journal of the Royal Statistical Society: Series B, 39(1), 1-22, 1977.
5. Carsten Rother, Vladimir Kolmogorov, Andrew Blake. [GrabCut: Interactive Foreground Extraction using Iterated Graph Cuts](https://www.microsoft.com/en-us/research/publication/grabcut-interactive-foreground-extraction-using-iterated-graph-cuts/). ACM Transactions on Graphics, 23(3), 309-314, 2004.
6. Azriel Rosenfeld, John L. Pfaltz. [Sequential Operations in Digital Picture Processing](https://doi.org/10.1145/321356.321357). Journal of the ACM, 13(4), 471-494, 1966.
