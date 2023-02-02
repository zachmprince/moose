# Introduction to PDE-Constrained Inverse Optimization

!---

# Physics Constrained Optimization

!row!
!col! width=50%

!style halign=center
Seismic Tomography Inversion

!media optimization/example_seismic.png style=width:80%;margin-left:auto;margin-right:auto;display:block;background-color:white

!style halign=center
[Optimization of fin spacing in heat exchanger](https://www.mdpi.com/1996-1073/10/11/1828)

!media optimization/example_heat_exchanger.png style=background-color:white
!col-end!

!col! width=50%


!style halign=center
[PDE Constrained Inverse Source Identification](https://www.osti.gov/biblio/1095940)

!media optimization/example_acoustic.png

!style halign=center
PDE Constrained Topology Optimization

!media optimization/example_topology.png style=background-color:white
!col-end!
!row-end!

!---

# PDE-Constrained Inverse Optimization

!media optimization/fig_optCycle.png

!---

# MOOSE Approach: Gradient-Based Optimization

- Gradient optimization methods typically require far fewer forward evaluations

  - MOOSE services applications with expensive, complex models
  - Many, if not most, problems are transient

- MOOSE flexibility and modularity allow us to efficiently compute gradients using adjoint solutions

- Cons of gradient-based approach:

  - Formulating adjoints of nonlinear, multiphysical problems can be extremely complex
  - Non-convexity of minimization causes issues without regularization
  - Transient adjoint require storage of full forward solution at every time step
