[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [u]
  []
[]

[Kernels]
  [diff]
    type = Diffusion
    variable = u
  []
[]

[BCs]
  [left]
    type = DirichletBC
    variable = u
    boundary = left
    value = 0
  []
  [right]
    type = DirichletBC
    variable = u
    boundary = right
    value = 1
  []
[]

[VectorPostprocessors]
  [constant1]
    type = ConstantVectorPostprocessor
    value = '1.0 2.0 3.0; 4.0 5.0 6.0'
    vector_names = 'a b'
    execute_on = initial
    outputs = none
  []
  [constant2]
    type = ConstantVectorPostprocessor
    value = '7.0 8.0 9.0'
    execute_on = initial
    outputs = none
  []
  [sum]
    type = SumVectors
    vector_name = summing
    input_vpp_names = 'constant1 constant1 constant2'
    input_vector_names = 'a b value'
    scale_factors = '1.0 2.0 0.5'
    execute_on = timestep_end
  []
[]

[Executioner]
  type = Steady
[]

[Outputs]
  execute_on = 'timestep_end'
  csv = true
[]
