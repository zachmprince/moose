[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [./u]
  [../]
[]

[AuxVariables]
  [./v]
  [../]
[]

[Kernels]
  [./diff]
    type = Diffusion
    variable = u
  [../]
  [./td]
    type = TimeDerivative
    variable = u
  [../]
  [./cf]
    type = CoupledForce
    coef = 10000
    variable = u
    v=v
  [../]
[]

[BCs]
  [./left]
    type = DirichletBC
    variable = u
    boundary = left
    value = 0
  [../]
  [./right]
    type = DirichletBC
    variable = u
    boundary = right
    value = 1
  [../]
[]

[Executioner]
  type = Transient
  num_steps = 10
  dt = 0.2

  solve_type = 'PJFNK'

  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
[]

[Outputs]
  exodus = true
[]

[MultiApps]
  [./sub_app]
    type = TransientMultiApp
    input_files = 'petsc_problem_transient.i'
    app_type = ExternalPetscSolverApp
    library_path = '../../../../external_petsc_solver/lib'
  [../]
[]

[Transfers]
  [./fromsub]
    type = MultiAppNearestNodeTransfer
    direction = from_multiapp
    multi_app = sub_app
    source_variable = u
    variable = v
    fixed_meshes = true
  [../]
[]
