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
[]

[CrossValidation]
  [LeaveOneOut]
    [poly_reg_loo]
      trainer = poly_reg_train
      surrogate = poly_reg
      num_rows = 200
    []
    [poly_reg_loo_retrain]
      trainer = poly_reg_train
      surrogate = poly_reg
      num_rows = 200
      force_retrain = true
    []
    [poly_choas_loo]
      trainer = poly_chaos_train
      surrogate = poly_chaos
      num_rows = 200
      postprocessor_name = poly_chaos_loo
    []
  []
[]

[Outputs]
  execute_on = 'FINAL'
  csv = true
[]
