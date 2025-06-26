[Mesh]
  [atr]
    type = ATRMeshGenerator
  []
[]

[Variables/u]
[]

[Kernels/diffusion]
  type = Diffusion
  variable = u
[]

[BCs]
  [inner]
    type = DirichletBC
    variable = u
    boundary = 0
    value = 0
  []
  [outer]
    type = DirichletBC
    variable = u
    boundary = 1
    value = 1
  []
[]

[Executioner]
  type = Steady
[]

[Outputs]
  exodus = true
[]
