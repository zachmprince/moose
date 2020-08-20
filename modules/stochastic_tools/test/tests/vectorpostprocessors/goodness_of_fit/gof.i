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
    model = 'poly_reg1 poly_reg2 poly_reg3 poly_chaos1 poly_chaos2'
    sampler = sample
    execute_on = timestep_end
    outputs = none
  []
  [gof]
    type = GoodnessOfFit
    data_vpp = data
    data_vector = g_values
    model_results_vpp = 'model_data model_data model_data model_data model_data'
    model_results_vector = 'poly_reg1 poly_reg2 poly_reg3 poly_chaos1 poly_chaos2'
    model = 'poly_reg1 poly_reg2 poly_reg3 poly_chaos1 poly_chaos2'
    compute = 'rmse rsquared adj-rsquared fstatistic pvalue'
    execute_on = final
  []
[]

[Trainers]
  [poly_reg1_train]
    type = PolynomialRegressionTrainer
    regression_type = ols
    sampler = sample
    results_vpp = data
    results_vector = g_values
    max_degree = 1
  []
  [poly_reg2_train]
    type = PolynomialRegressionTrainer
    regression_type = ols
    sampler = sample
    results_vpp = data
    results_vector = g_values
    max_degree = 2
  []
  [poly_reg3_train]
    type = PolynomialRegressionTrainer
    regression_type = ols
    sampler = sample
    results_vpp = data
    results_vector = g_values
    max_degree = 3
  []
  [poly_chaos1_train]
    type = PolynomialChaosTrainer
    sampler = sample
    results_vpp = data
    results_vector = g_values
    distributions = 'uniform uniform uniform uniform uniform uniform'
    order = 2
  []
  [poly_chaos2_train]
    type = PolynomialChaosTrainer
    sampler = sample
    results_vpp = data
    results_vector = g_values
    distributions = 'uniform uniform uniform uniform uniform uniform'
    order = 3
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
  [poly_reg3]
    type = PolynomialRegressionSurrogate
    trainer = poly_reg3_train
  []
  [poly_chaos1]
    type = PolynomialChaos
    trainer = poly_chaos1_train
  []
  [poly_chaos2]
    type = PolynomialChaos
    trainer = poly_chaos2_train
  []
[]

[Outputs]
  execute_on = 'FINAL'
  csv = true
[]
