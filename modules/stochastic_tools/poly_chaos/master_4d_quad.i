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
  [Q_dist]
    type = UniformDistribution
    lower_bound = 9000
    upper_bound = 11000
  []
  [R_dist]
    type = UniformDistribution
    lower_bound = 0.09
    upper_bound = 0.11
  []
  [T_dist]
    type = UniformDistribution
    lower_bound = 290
    upper_bound = 310
  []
[]

[Samplers]
  [sample]
    type = MonteCarloSampler
    num_rows = 10000
    distributions = 'k_dist Q_dist R_dist T_dist'
    execute_on = timestep_end
  []
  [quadrature]
    type = QuadratureSampler
    distributions = 'k_dist Q_dist R_dist T_dist'
    # execute_on = INITIAL
    order = 5
    execute_on = 'PRE_MULTIAPP_SETUP'
  []
[]

[MultiApps]
  [quad_sub]
    type = SamplerFullSolveMultiApp
    input_files = sub.i
    sampler = quadrature
    # mode = batch-restore
  []
[]

[Transfers]
  # [quad]
  #   type = SamplerParameterTransfer
  #   multi_app = quad_sub
  #   sampler = quadrature
  #   parameters = 'Materials/conductivity/prop_values Kernels/source/value BCs/right/value'
  #   to_control = 'stochastic'
  # []
  [data]
    type = SamplerPostprocessorTransfer
    multi_app = quad_sub
    sampler = quadrature
    to_vector_postprocessor = storage
    from_postprocessor = avg
  []
[]

[Controls]
  [cmdline]
    type = MultiAppCommandLineControl
    multi_app = quad_sub
    sampler = quadrature
    param_names = 'Materials/conductivity/prop_values Kernels/source/value Mesh/xmax BCs/right/value'
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
    order = 5
    distributions = 'k_dist Q_dist R_dist T_dist'
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
