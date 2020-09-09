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

[Postprocessors]
  [cv]
    type = CrossValidatedRSME
    data_vpp = data
    data_vector = g_values
    sampler = sample
    models = 'poly_reg1 poly_reg2'
    test_index = '101; 1'
    num_points = '100; 100'
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
[]

[Trainers]
  [poly_reg1_train]
    type = PolynomialRegressionTrainer
    regression_type = ols
    sampler = sample
    results_vpp = data
    results_vector = g_values
    max_degree = 2
    skip_index = 101
    num_skip = 100
  []
  [poly_reg2_train]
    type = PolynomialRegressionTrainer
    regression_type = ols
    sampler = sample
    results_vpp = data
    results_vector = g_values
    max_degree = 2
    skip_index = 1
    num_skip = 100
  []
[]

[Surrogates]
  [poly_reg1]
    type = PolynomialRegressionSurrogate
    trainer = poly_reg1_train
  []
  [poly_reg2]
    type = PolynomialRegressionSurrogate
    trainer = poly_reg2_train
  []
[]

[Outputs]
  execute_on = 'FINAL'
  csv = true
[]
