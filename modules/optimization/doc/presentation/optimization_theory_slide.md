# Theory of PDE-Constrained Inverse Optimization with Adjoints

!---

# Problem Formulation

!style fontsize=150%
!equation
\min_{\mathbf{p}} f\left(\mathbf{u},\mathbf{p}\right);\quad~\textrm{subject to}~\mathcal{R}\left(\mathbf{u},\mathbf{p}\right)=\mathbf{0}

- $f\left(\mathbf{u},\mathbf{p}\right)$ is the objective function

  - Scalar measure of misfit between simulation ($\mathbf{u}$) and expected (experimental) ($\widetilde{\mathbf{u}}$) values
  - Typically an $L_2$ difference between values:

  !equation
  \frac{1}{2} \sum_{i=1}^{N} \left( u_i - \widetilde{u}_i \right)^2

- $\mathcal{R}(\mathbf{u},\mathbf{p})$ is the constraint defined by the residual of the multiphysics simulation

  - Heat conduction as an example:

  !equation
  \mathcal{R}(T,\mathbf{p}) := \vec{\nabla}\cdot k(\mathbf{p})\vec{\nabla}T - q(\mathbf{p})

- $\mathbf{p}$ contains design variables (e.g. material properties or loads)
- $\mathbf{u}$ contains state variables (e.g. temperature and displacement fields)

!---

# Adjoint-Based Gradient Computation

- Gradient-based optimization algorithms require the computation of $df/d\mathbf{p}$
- Finite-difference is possible, but requires a model call for each parameter
- Adjoint-based computation:

  !equation
  \frac{df}{d\mathbf{p}} = \mathbf{\lambda}^\top\frac{\partial\mathcal{R}}{\partial\mathbf{p}}

  - $\mathbf{\lambda}$ adjoint solution of state variables
  - $\partial\mathcal{R}/\partial\mathbf{p}$ can be thought of as "derivative of residual w.r.t to each parameter"
  - +Note:+ $\partial\mathcal{R}/\partial\mathbf{p}$ is non-trivial to formulate for nonlinear problems

!---

# Adjoint Computation

!equation
\left(\frac{\partial\mathcal{R}}{\partial\mathbf{u}}\right)^\top \mathbf{\lambda}= -\left(\frac{\partial f}{\partial\mathbf{u}}\right)^\top

- Adjoint equation is linear as it depends on the already-computed forward solution ($\mathbf{u}$) and not the adjoint solution ($\mathbf{\lambda}$)
- $\partial\mathcal{R}/\partial\mathbf{u}$ is the Jacobian of the forward problem
- $\partial f/\partial\mathbf{u}$ is generally trivial to formulate

  - For the $L_2$ difference, $f$ can be re-cast as an integral:

  !equation
  f = \frac{1}{2} \sum_{i=1}^{N} \int_{\Omega} \delta(x - x_i) \left( u - \widetilde{u}_i \right)^2dx

  - Thus the source ends up being a `DiracKernel` at points described by $x_i$:

  !equation
  \frac{\partial f}{\partial\mathbf{u}} = \sum_{i=1}^{N}\left( u(x_i) - \widetilde{u}_i \right)

- Boundary conditions are homogenized, i.e. Dirichlet and Neumann values set to 0

!---

# Residual Derivative Computation

- To go back to the gradient definition: $df/d\mathbf{p} = \mathbf{\lambda}^\top\boxed{\partial\mathcal{R}/\partial\mathbf{p}}$
- Putting the residual in weak form and separating some terms:

  !equation
  \mathbf{\hat{R}}(u^*, u; \mathbf{p}) = \mathbf{\hat{J}}_{\Omega}(u^*, u; \mathbf{p}) + \mathbf{\hat{J}}_{\Gamma}(u^*, u; \mathbf{p}) - \mathbf{\hat{g}}_{\Omega}(u^*; \mathbf{p}) - \mathbf{\hat{g}}_{\Gamma}(u^*; \mathbf{p})

  - $\mathbf{\hat{J}}$ signifies linear or nonlinear terms and $\mathbf{\hat{g}}$ signifies source terms
  - $\Omega$ subscript are volumetric terms and $\Gamma$ subscript are boundary terms.

- Heat conduction as an example:

  !equation
  \mathbf{\hat{J}}_{\Omega}(T^*, T; \mathbf{p}) = \int_{\Omega}\vec{\nabla}T^*\cdot k(T,\mathbf{p})\vec{\nabla}Tdx \qquad
  \mathbf{\hat{J}}_{\Gamma}(u^*, u; \mathbf{p}) = \int_{\Gamma_c}T^*h(\mathbf{p})(T - T_{\infty})dx

  !equation
  \mathbf{\hat{g}}_{\Omega}(u^*; \mathbf{p}) = \int_{\Omega} u^* q_b(\mathbf{p})dx \qquad
  \mathbf{\hat{g}}_{\Omega}(u^*; \mathbf{p}) = \int_{\Gamma_n} u^* q_n(\mathbf{p})dx

- Parameter dependence on $\mathbf{\hat{J}}$ represents +material inversion+
- Parameter dependence on $\mathbf{\hat{g}}$ represents +force inversion+

!---

# Force Inversion Gradient

!equation
\mathbf{\hat{R}}(u^*, u; \mathbf{p}) = \mathbf{\hat{J}}_{\Omega}(u^*, u) + \mathbf{\hat{J}}_{\Gamma}(u^*, u) - \mathbf{\hat{g}}_{\Omega}(u^*; \mathbf{p}) - \mathbf{\hat{g}}_{\Gamma}(u^*; \mathbf{p})

!equation
\frac{\partial\mathbf{\hat{R}}}{\partial\mathbf{p}} = -\frac{\partial\mathbf{\hat{g}}_{\Omega}}{\partial\mathbf{p}} - \frac{\partial\mathbf{\hat{g}}_{\Gamma}}{\partial\mathbf{p}}

- Heat Conduction:

!equation
\frac{\partial\mathbf{\hat{R}}}{\partial\mathbf{p}} = -\int_{\Omega} u^* \frac{dq_b}{dp}dx - \int_{\Gamma_n} u^* \frac{dq_n}{d\mathbf{p}}dx

- Given that $\lambda$ is in the same space as $u^*$ we have:

!equation
\mathbf{\hat{\lambda}}^\top\frac{\partial\mathbf{\hat{R}}}{\partial\mathbf{p}} = -\int_{\Omega} \lambda \frac{dq_b}{dp}dx - \int_{\Gamma_n} \lambda \frac{dq_n}{d\mathbf{p}}dx

- Notice that the result is simply an integral of the adjoint solution scaled by the derivative of the source for each parameter

!---

# Material Inversion Gradient

!equation
\mathbf{\hat{R}}(u^*, u; \mathbf{p}) = \mathbf{\hat{J}}_{\Omega}(u^*, u) + \mathbf{\hat{J}}_{\Gamma}(u^*, u;\mathbf{p}) - \mathbf{\hat{g}}_{\Omega}(u^*) - \mathbf{\hat{g}}_{\Gamma}(u^*)

!equation
\frac{\partial\mathbf{\hat{R}}}{\partial\mathbf{p}} = \frac{\partial\mathbf{\hat{J}}_{\Omega}}{\partial\mathbf{p}} + \frac{\partial\mathbf{\hat{J}}_{\Gamma}}{\partial\mathbf{p}}

- Heat conduction:

!equation
\mathbf{\hat{\lambda}}^\top\frac{\partial\mathbf{\hat{R}}}{\partial\mathbf{p}}= \int_{\Omega}\vec{\nabla}\lambda\cdot\frac{dk}{d\mathbf{p}}\vec{\nabla}Tdx + \int_{\Gamma_c}\lambda\frac{dh}{d\mathbf{p}}(T - T_{\infty})dx

- Note that these contain an inner product between the forward and adjoint solutions

!---

# Transient Inverse Optimization

- Objective ($f(\mathbf{u},\mathbf{p})$) remains the same except measurement index ($i$) represents different times as well as location

  !equation
  f = \frac{1}{2} \sum_{i=1}^{N} \int_{0}^{t_{N_t}}\delta(t - t_i) \int_{\Omega} \delta(x - x_i) \left( u - \widetilde{u}_i \right)^2dxdt

- Residual now includes a time derivative term:

  !equation
  \mathbf{\hat{R}}(u^*, u; \mathbf{p}) = \mathbf{\hat{T}}\left(u^*, \frac{du}{dt}; \mathbf{p}\right) + \mathbf{\hat{J}}_{\Omega}(u^*, u; \mathbf{p}) + \mathbf{\hat{J}}_{\Gamma}(u^*, u; \mathbf{p}) - \mathbf{\hat{g}}_{\Omega}(u^*; \mathbf{p}) - \mathbf{\hat{g}}_{\Gamma}(u^*; \mathbf{p})

  Let's assume implicit-Euler stepping:

  !equation
  \mathbf{\hat{R}}_n(u^*, u_n; \mathbf{p}) = \mathbf{\hat{T}}_{\mathrm{i}}\left(u^*, u_n; \mathbf{p}\right) + \mathbf{\hat{T}}_{\mathrm{e}}\left(u^*, u_{n-1}; \mathbf{p}\right) + \mathbf{\hat{J}}_n(u^*, u_n; \mathbf{p}) - \mathbf{\hat{g}}_n(u^*; \mathbf{p}), \quad n=1,...,N_t

- Parameters $\mathbf{p}$ do not depend on time, but some parameters might only be relevant at certain times.

!---

# Transient Adjoint Computation

!style! fontsize=90%

- Let's assume a linear system, and we can define implicit and explicit operators:

  !equation
  \mathbf{A}_n\mathbf{u}_n = \mathbf{\hat{T}}_{\mathrm{i}}\left(u^*, u_n; \mathbf{p}\right) + \mathbf{\hat{J}}_j(u^*, u_n; \mathbf{p}), \qquad
  \mathbf{B}_{n-1}\mathbf{u}_{n-1} = \mathbf{\hat{T}}_{\mathrm{e}}\left(u^*, u_{n-1}; \mathbf{p}\right), \qquad \mathbf{g}_n = \mathbf{\hat{g}}_n(u^*; \mathbf{p})

- We then can formulate a full linear system that includes the time stepping:

  !equation
  \begin{bmatrix}
  \mathbf{A}_1 &              &                  &                  \\
  \mathbf{B}_1 & \mathbf{A}_2 &                  &                  \\
               & \ddots       & \ddots           &                  \\
               &              & \mathbf{B}_{N_t-1} & \mathbf{A}_{N_t} \\
  \end{bmatrix}
  \begin{bmatrix}
  \mathbf{u}_1 \\
  \mathbf{u}_2 \\
  \vdots \\
  \mathbf{u}_{N_t} \\
  \end{bmatrix}
  =
  \begin{bmatrix}
  \mathbf{g}_1 \\
  \mathbf{g}_2 \\
  \vdots \\
  \mathbf{g}_{N_t} \\
  \end{bmatrix}

- The adjoint equation is simply the transpose of this system (backwards time stepping):

  !equation
  \begin{bmatrix}
  \mathbf{A}^\top_1 & \mathbf{B}^\top_1 &                   &                         &                         \\
                    & \mathbf{A}^\top_2 & \mathbf{B}^\top_2 &                         &                         \\
                    &                   & \ddots            & \ddots                  &                         \\
                    &                   &                   & \mathbf{A}^\top_{N_t-1} & \mathbf{B}^\top_{N_t-1} \\
                    &                   &                   &                         & \mathbf{A}^\top_{N_t}   \\
  \end{bmatrix}
  \begin{bmatrix}
  \mathbf{\lambda}_1 \\
  \mathbf{\lambda}_2 \\
  \vdots \\
  \mathbf{\lambda}_{N_t-1} \\
  \mathbf{\lambda}_{N_t} \\
  \end{bmatrix}
  = -
  \begin{bmatrix}
  \partial\mathbf{f}/\partial\mathbf{u}_1 \\
  \partial\mathbf{f}/\partial\mathbf{u}_2 \\
  \vdots \\
  \partial\mathbf{f}/\partial\mathbf{u}_{N_t-1} \\
  \partial\mathbf{f}/\partial\mathbf{u}_{N_t} \\
  \end{bmatrix}

- Where the source is a DiracKernel in space and time (time steps must align with measurement times):

  !equation
  \frac{\partial f}{\partial\mathbf{u}_n} = \sum_{i=1}^{N}\delta(t_n - t_i)\left( u_n(x_i) - \widetilde{u}_i \right)

!style-end!

!---

# Transient Force Inversion Gradient

- Without loss of generality, we can represent our parameterized source as:

  !equation
  \mathbf{\hat{g}}_n(u^*;\mathbf{p}) = \int_{\Omega} \sum_{j}^{N_p} p_j\Phi_j(x,t_n)dxdt

- The gradient can then be computed as:

  !equation
  \mathbf{\hat{\lambda}}^\top\frac{\partial\mathbf{\hat{R}}}{\partial p_j} = \sum_{n=1}^{N_t}\frac{t_n-t_{n-1}}{2}\int_{\Omega}\left(\lambda_n\Phi_j(x,t_n) + \lambda_{n-1}\Phi_j(x,t_{n-1})\right)dx
