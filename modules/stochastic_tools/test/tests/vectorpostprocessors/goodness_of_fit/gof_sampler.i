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
  [test]
    type = MonteCarlo
    distributions = 'uniform uniform uniform uniform uniform uniform'
    num_rows = 200
    seed = 1993
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
  [data_test]
    type = GFunction
    sampler = test
    q_vector = '0 0.5 3 9 99 99'
    execute_on = initial
    outputs = none
  []
  [gof]
    type = GoodnessOfFit
    data_vpp = data_test
    data_vector = g_values
    model = 'poly_reg poly_chaos gp'
    sampler = test
    compute = 'rmse rsquared'
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
    max_degree = 2
  []
  [poly_chaos_train]
    type = PolynomialChaosTrainer
    sampler = sample
    results_vpp = data
    results_vector = g_values
    distributions = 'uniform uniform uniform uniform uniform uniform'
    order = 3
  []
  [gp_train]
    type = GaussianProcessTrainer
    covariance_function = covar
    standardize_params = true
    standardize_data = true
    sampler = sample
    results_vpp = data
    results_vector = g_values
  []
[]

[Covariance/covar]
  type=SquaredExponentialCovariance
  signal_variance = 1
  noise_variance = 0
  length_factor = '1 1'
[]

[Surrogates]
  [poly_reg]
    type = PolynomialRegressionSurrogate
    trainer = poly_reg_train
  []
  [poly_chaos]
    type = PolynomialChaos
    trainer = poly_chaos_train
  []
  [gp]
    type = GaussianProcess
    trainer = gp_train
  []
[]

[Outputs]
  execute_on = 'FINAL'
  csv = true
[]
