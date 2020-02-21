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
    type = UniformDistribution
    lower_bound = 1
    upper_bound = 10
  []
[]

[Samplers]
  [sample]
    type = MonteCarloSampler
    num_rows = 10000
    distributions = 'k_dist'
    execute_on = timestep_end
  []
  [quadrature]
    type = QuadratureSampler
    distributions = k_dist
    execute_on = INITIAL
    order = 20
  []
[]

[MultiApps]
  [quad_sub]
    type = SamplerFullSolveMultiApp
    input_files = sub.i
    sampler = quadrature
    mode = batch-restore
  []
[]

[Transfers]
  [quad]
    type = SamplerParameterTransfer
    multi_app = quad_sub
    sampler = quadrature
    parameters = 'Materials/conductivity/prop_values'
    to_control = 'stochastic'
  []
  [data]
    type = SamplerPostprocessorTransfer
    multi_app = quad_sub
    sampler = quadrature
    to_vector_postprocessor = storage
    from_postprocessor = avg
  []
[]

[VectorPostprocessors]
  [storage]
    type = StochasticResults
    parallel_type = REPLICATED
    samplers = quadrature
  []
  [pc_coeff]
    type = PolyChaosData
    pc_name = poly_chaos
    execute_on = final
  []
  [pc_samp]
    type = PolyChaosSample
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
    distributions = k_dist
    training_sampler = quadrature
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
