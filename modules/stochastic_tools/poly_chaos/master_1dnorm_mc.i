[Mesh]
  type = GeneratedMesh
  dim = 1
  nx = 1
  ny = 1
[]

[Variables]
  [u]
  []
[]

[Distributions]
  [k_dist]
    type = NormalDistribution
    mean = 5
    standard_deviation = 0.5
  []
[]

[Samplers]
  [sample]
    type = MonteCarloSampler
    num_rows = 10000
    distributions = 'k_dist'
    execute_on = INITIAL
  []
[]

[MultiApps]
  [sub]
    type = SamplerFullSolveMultiApp
    input_files = sub.i
    sampler = sample
    mode = batch-restore
  []
[]

[Transfers]
  [tran]
    type = SamplerParameterTransfer
    multi_app = sub
    sampler = sample
    parameters = 'Materials/conductivity/prop_values'
    to_control = 'stochastic'
  []
  [data]
    type = SamplerPostprocessorTransfer
    multi_app = sub
    sampler = sample
    to_vector_postprocessor = storage
    from_postprocessor = avg
  []
[]

[VectorPostprocessors]
  [storage]
    type = StochasticResults
    parallel_type = DISTRIBUTED
    samplers = sample
  []
  [pc_coeff]
    type = PolyChaosData
    pc_name = poly_chaos
    execute_on = final
  []
  [pc_samp]
    type = SurrogateTester
    model = poly_chaos
    sampler = sample
    output_samples = true
    execute_on = final
  []
[]

[UserObjects]
  [poly_chaos]
    type = PolynomialChaos
    execute_on = timestep_end
    order = 20
    distributions = 'k_dist'
    training_sampler = sample
    stochastic_results = storage
  []
[]

[Executioner]
  type = Steady
[]

[Problem]
  solve = false
  kernel_coverage_check = false
[]

[Outputs]
  [out]
    type = CSV
    execute_on = FINAL
  []
[]
