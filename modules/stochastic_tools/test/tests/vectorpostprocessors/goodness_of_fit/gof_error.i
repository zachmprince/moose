[StochasticTools]
[]

[Distributions/uniform]
  type = Uniform
  lower_bound = 0
  upper_bound = 1
[]

[Samplers]
  [sample]
    type = MonteCarlo
    distributions = 'uniform uniform uniform uniform uniform uniform'
    num_rows = 200
  []
[]

[VectorPostprocessors]
  [data]
    type = GFunction
    sampler = sample
    q_vector = '0 0.5 3 9 99 99'
    execute_on = initial
    outputs = none
  []
  [model_data]
    type = EvaluateSurrogate
    model = 'poly_reg'
    sampler = sample
    execute_on = timestep_end
    outputs = none
  []
  [gof1]
    type = GoodnessOfFit
    data_vpp = data
    data_vector = g_values
    model_results_vpp = 'model_data'
    model_results_vector = 'poly_reg'
    compute = 'adj-rsquared'
    execute_on = final
  []
  [gof2]
    type = GoodnessOfFit
    data_vpp = data
    data_vector = g_values
    sampler = sample
    compute = 'rsquared'
    execute_on = final
  []
[]

[Trainers]
  [poly_reg_train]
    type = PolynomialRegressionTrainer
    regression_type = ols
    sampler = sample
    results_vpp = data
    results_vector = g_values
    max_degree = 1
  []
[]

[Surrogates]
  [poly_reg]
    type = PolynomialRegressionSurrogate
    trainer = poly_reg_train
  []
[]

[Outputs]
  execute_on = 'FINAL'
  csv = true
[]
