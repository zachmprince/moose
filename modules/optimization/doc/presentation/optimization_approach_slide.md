# Implementation and Syntax

!---

# MultiApp Approach

!media optimization/multiapp_workflow.png

!---

# Optimization Main App: Defining Objective

!row!
!col! width=69%
- `OptimizationReporter` object defines the objective function
- The parameter space is defined by a list of parameter groups in `parameter_names`

  - Each parameter group has a number of values defined by `num_values`
  - Each parameter has an initial condition defined by `initial_condition`
  - Each parameter has an upper and lower point defined by `lower_bounds` and `upper_bounds` (used only with bounded optimization algorithms)

- Expected, or measurement, locations and values can either be defined at input or with a CSV file
!col-end!

!col! width=1%
!!
!col-end!

!col! width=30%
```
[OptimizationReporter]
  type = OptimizationReporter
  parameter_names = 'parameter_results'
  num_values = '1'
  initial_condition = '500'
  lower_bounds = '0.1'
  upper_bounds = '10000'
  measurement_points = '0.2 0.2 0
                        0.8 0.6 0
                        0.2 1.4 0
                        0.8 1.8 0'
  measurement_values = '226 254 214 146'
[]
```

```
[OptimizationReporter]
  type = OptimizationReporter
  parameter_names = 'parameter_results'
  num_values = '1'
  initial_condition = '500'
  lower_bounds = '0.1'
  upper_bounds = '10000'
  measurement_file = 'measurementData.csv'
  file_xcoord = 'coordx'
  file_ycoord ='y'
  file_zcoord = 'z'
  file_value = 'measured_value'
[]
```
!col-end!
!row-end!

!---

# Optimization Main App: Executing MultiApps

!row!
!col! width=69%
- Forward and adjoint apps are exlusively defined with `FullSolveMultiApp` (even with transient optimization)
- The module includes two execution flags: `FORWARD` and `ADJOINT`
- These flags +must+ be used or algorithms won't run the applications properly
!col-end!

!col! width=1%
!!
!col-end!

!col! width=30%
```
[MultiApps]
  [forward]
    type = FullSolveMultiApp
    input_files = forward.i
    execute_on = FORWARD
  []
  [adjoint]
    type = FullSolveMultiApp
    input_files = adjoint.i
    execute_on = ADJOINT
  []
[]
```
!col-end!
!row-end!

!---

# Optimization Main App: Transfering Data

!row!
!col! width=69%
- `OptimizationReporter` defines various vector reporter values to store data
- Vectors computed by `OptimizationReporter`:

  - Vector for each parameter group
  - `measurement_xcoord` / `ycoord` / `zcoord` / `time` / `values` are the measurement locations and values
  - `misfit_values` is $u_i - \tilde{u}_i$

- Vectors stored in `OptimizationReporter`:

  - `simulation_values` values is the forward solution evaluated at measurement locations
  - `adjoint` is the computed gradient, i.e. $\lambda^\top\partial \mathbf{R}/\partial \mathbf{p}$

- `MultiAppReporterTransfer` is used to transfer these quantities between apps
!col-end!

!col! width=1%
!!
!col-end!

!style fontsize=80%
!col! width=30%
```
[Transfers]
  [toForward_measument]
    type = MultiAppReporterTransfer
    to_multi_app = forward
    from_reporters = 'OptimizationReporter/measurement_xcoord
                      OptimizationReporter/measurement_ycoord
                      OptimizationReporter/measurement_zcoord
                      OptimizationReporter/measurement_time
                      OptimizationReporter/measurement_values
                      OptimizationReporter/parameter_results'
    to_reporters = 'measure_data/measurement_xcoord
                    measure_data/measurement_ycoord
                    measure_data/measurement_zcoord
                    measure_data/measurement_time
                    measure_data/measurement_values
                    params/q'
  []
  [toAdjoint]
    type = MultiAppReporterTransfer
    to_multi_app = adjoint
    from_reporters = 'OptimizationReporter/measurement_xcoord
                      OptimizationReporter/measurement_ycoord
                      OptimizationReporter/measurement_zcoord
                      OptimizationReporter/measurement_time
                      OptimizationReporter/misfit_values
                      OptimizationReporter/parameter_results'
    to_reporters = 'misfit/measurement_xcoord
                    misfit/measurement_ycoord
                    misfit/measurement_zcoord
                    misfit/measurement_time
                    misfit/misfit_values
                    params/q'
  []
  [fromForward]
    type = MultiAppReporterTransfer
    from_multi_app = forward
    from_reporters = 'measure_data/simulation_values'
    to_reporters = 'OptimizationReporter/simulation_values'
  []
  [fromadjoint]
    type = MultiAppReporterTransfer
    from_multi_app = adjoint
    from_reporters = 'gradient_vpp/inner_product'
    to_reporters = 'OptimizationReporter/adjoint'
  []
[]
```
!col-end!
!row-end!

!---

# Optimization Main App: Defining Solver

!row!
!col! width=69%
- `Optimize` is an executioner responsible for:

  - Creating the TAO solver
  - Executing objects on `FORWARD` and `ADJOINT`
  - Sending parameter values to `OptimizationReporter`
  - Retrieving objective value and gradients from `OptimizationReporter`

- `tao_solver` defines the optimization algorithm being used see [https://mooseframework.inl.gov/source/executioners/Optimize.html](https://mooseframework.inl.gov/source/executioners/Optimize.html) for more details
!col-end!

!col! width=1%
!!
!col-end!

!col! width=30%
```
[Executioner]
  type = Optimize
  tao_solver = taoblmvm
  petsc_options_iname = '-tao_gatol'
  petsc_options_value = '1e-6'
  verbose = true
[]
```
!col-end!
!row-end!

!---

# Forward Model App

!row!
!col! width=69%
- `Reporters` are used to create storage for transferred quantities

  - `ConstantReporter` creates storage for parameter values
  - `OptimizationData` creates storage for measurement locations, values, and evaluates the solution at these points.

- Functions derived from `OptimizationFunction` are able to take in the parameter vector to define a functional dependence on parameters

  - `ParsedOptimizationFunction` is used for parameters that have global support
  - `NearestReporterCoordinatesFunction` is used for parameters with compact support
!col-end!

!col! width=1%
!!
!col-end!

!col! width=30%
```
[Reporters]
  [params]
    type = ConstantReporter
    real_vector_names = 'q'
    real_vector_values = '0' # Dummy value
  []
  [measure_data]
    type = OptimizationData
    variable = temperature
  []
[]
```

```
[Functions]
  [volumetric_heat_func]
    type = ParsedOptimizationFunction
    expression = q
    param_symbol_names = 'q'
    param_vector_name = 'params/q'
  []
[]
```
!col-end!
!row-end!

!---

# Adjoint Model App

!row!
!col! width=65%
- `Reporters` are used to create storage for transferred quantities

  !style! fontsize=80%
  - `ConstantReporter` creates storage for parameter values
  - `OptimizationData` creates storage for measurement locations and misfit values.
  !style-end!

- `ReporterPointSource` implements the DiracKernel for the adjoint source
- `OptimizationFunction`s are able to take the derivative of the function w.r.t parameters.
- `OptimizationFunctionInnerProduct`'s take the inner product of the adjoint variable and the derivative of the residual, based on how the `OptimizationFunction` is applied

  !style! fontsize=80%
  - `ElementOptimizationSourceFunctionInnerProduct`
  - `ElementOptimizationDiffusionCoefFunctionInnerProduct`
  - `SideOptimizationNeumannFunctionInnerProduct`
  !style-end!
!col-end!

!col! width=5%
!!
!col-end!

!style! fontsize=80%
!col! width=30%
```
[Reporters]
  [params]
    type = ConstantReporter
    real_vector_names = 'q'
    real_vector_values = '0' # Dummy value
  []
  [misfit]
    type = OptimizationData
  []
[]

[DiracKernels]
  [pt]
    type = ReporterPointSource
    variable = adjoint_T
    x_coord_name = misfit/measurement_xcoord
    y_coord_name = misfit/measurement_ycoord
    z_coord_name = misfit/measurement_zcoord
    value_name = misfit/misfit_values
  []
[]

[Functions]
  [volumetric_heat_func]
    type = ParsedOptimizationFunction
    expression = q
    param_symbol_names = 'q'
    param_vector_name = 'params/q'
  []
[]

[VectorPostprocessors]
  [gradient_vpp]
    type = ElementOptimizationSourceFunctionInnerProduct
    variable = adjoint_T
    function = volumetric_heat_func
  []
[]
```
!col-end!
!style-end!
!row-end!

!---

# Mesh-Based Parameter Definition

!style halign=center
Defining two-meshes, one for parameters and one for the physics

!row!
!col! width=50%
```
[Mesh]
  [gmg]
    type = GeneratedMeshGenerator
    dim = 2
    nx = 2
    ny = 2
  []
  parallel_type = REPLICATED
[]
```

```
[Mesh]
  [gmg]
    type = GeneratedMeshGenerator
    dim = 2
    nx = 10
    ny = 10
  []
[]
```
!col-end!

!col! width=50%
!media optimization/mesh_overlay.png style=width:80%;margin-left:auto;margin-right:auto;display:block
!col-end!
!row-end!

!---

# Mesh-Based Parameter Definition (cont.)

!row!
!col! width=69%
- New `OptimizationReporter` that defines parameters as solutions on the provided mesh

  - `families` defines the if the parameter definition is elemental or nodal
  - `orders` defines the interpolation of the value across the element
  - Parameter vector created represents the DOFs of this solution

- `ParameterMeshFunction` interpolates based on the parameter values

  - `exodus_mesh` is the same mesh as in the `OptimizationReporter`
  - `parameter_name` is the parameter vector transferred from the main app
!col-end!

!col! width=1%
!!
!col-end!

!col! width=30%
```
[OptimizationReporter]
  type = ParameterMeshOptimization
  parameter_names = 'source'
  parameter_meshes = 'parameter_mesh_in.e'
  families = 'FIRST'
  orders = 'LAGRANGE'

  measurement_points = ...
  measurement_values = ...
[]
```

```
[Functions]
  [src_func]
    type = ParameterMeshFunction
    exodus_mesh = parameter_mesh_in.e
    parameter_name = src_rep/vals
  []
[]
```
!col-end!
!row-end!
