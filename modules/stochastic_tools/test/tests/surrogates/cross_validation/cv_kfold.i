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
[]

[Surrogates]
  [poly_reg]
    type = PolynomialRegressionSurrogate
    trainer = poly_reg_train
  []
[]

[CrossValidation]
  [KFold]
    [kfold]
      trainer = poly_reg_train
      surrogate = poly_reg
      num_rows = 200
      num_folds = 2
    []
    [kfold_shuffle]
      trainer = poly_reg_train
      surrogate = poly_reg
      num_rows = 200
      num_folds = 10
      shuffle = true
    []
  []
[]

[Outputs]
  execute_on = 'FINAL'
  csv = true
[]
