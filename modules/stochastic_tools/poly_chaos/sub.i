[Mesh]
  type = GeneratedMesh
  dim = 1
  nx = 1
  xmax = 1 # R
  elem_type = Edge3
[]

[Variables]
  [u]
    order = SECOND
    family = LAGRANGE
  []
[]

[Kernels]
  [conduction]
    type = MatDiffusion
    variable = u
    diffusivity = k
  []
  [source]
    type = BodyForce
    variable = u
    value = 1.0 # q
  []
[]

[Materials]
  [conductivity]
    type = GenericConstantMaterial
    prop_names = k # k
    prop_values = 2.0
  []
[]

[BCs]
  [right]
    type = DirichletBC
    variable = u
    boundary = right
    value = 0 # T_inf
  []
[]

[Executioner]
  type = Steady
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
[]

[Controls]
  [stochastic]
    type = SamplerReceiver
  []
[]

[Postprocessors]
  [avg]
    type = AverageNodalVariableValue
    variable = u
  []
  [max]
    type = NodalExtremeValue
    variable = u
    value_type = max
  []
[]

[Outputs]
[]
