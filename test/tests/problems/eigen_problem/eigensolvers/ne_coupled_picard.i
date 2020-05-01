[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = 0
  xmax = 10
  ymin = 0
  ymax = 10
  elem_type = QUAD4
  nx = 8
  ny = 8
[]

[Variables]
  [./u]
    order = FIRST
    family = LAGRANGE
    # set this so that the Picard initial norm is not zero
    initial_condition = 1
  [../]
[]

[AuxVariables]
  [./T]
    order = FIRST
    family = LAGRANGE
    # set this so that the Picard initial norm is not zero
    initial_condition = 1
  [../]
  [./power]
    order = FIRST
    family = LAGRANGE
  [../]
[]

[Kernels]
  [./diff]
    type = DiffMKernel
    variable = u
    mat_prop = diffusion
    offset = 0.0
  [../]

  [./rhs]
    type = Reaction
    variable = u
    extra_vector_tags = 'eigen'
  [../]
[]

[AuxKernels]
  [./power_ak]
    type = NormalizationAux
    variable = power
    source_variable = u
    normalization = unorm
    # this coefficient will affect the eigenvalue.
    normal_factor = 10
    execute_on = timestep_end
  [../]
[]

[BCs]
  [./homogeneous]
    type = DirichletBC
    variable = u
    boundary = '0 1 2 3'
    value = 0
  [../]

  [./eigenU]
    type = EigenDirichletBC
    variable = u
    boundary = '0 1 2 3'
  [../]
[]

[Materials]
  [./dc]
    type = VarCouplingMaterial
    var = T
    block = 0
    base = 1.0
    coef = 1.0
  [../]
[]

[Executioner]
  type = Eigenvalue
  matrix_free = true
  solve_type = NEWTON
  eigen_problem_type = GEN_NON_HERMITIAN
  picard_max_its = 10
  picard_rel_tol = 1e-6
[]

[Postprocessors]
  [./unorm]
    type = ElementIntegralVariablePostprocessor
    variable = u
    execute_on = linear
  [../]
[]

[VectorPostprocessors]
  [./eigenvalues]
    type = Eigenvalues
    execute_on = 'timestep_end'
  [../]
[]

[Outputs]
  csv = true
  execute_on = 'timestep_end'
[]

[MultiApps]
  [./sub]
    type = FullSolveMultiApp
    input_files = ne_coupled_picard_sub.i
    execute_on = timestep_end
  [../]
[]

[Transfers]
  [./T_from_sub]
    type = MultiAppNearestNodeTransfer
    direction = from_multiapp
    multi_app = sub
    source_variable = T
    variable = T
  [../]
  [./power_to_sub]
    type = MultiAppNearestNodeTransfer
    direction = to_multiapp
    multi_app = sub
    source_variable = power
    variable = power
  [../]
[]
