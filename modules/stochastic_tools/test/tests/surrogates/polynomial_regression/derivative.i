[StochasticTools]
[]

[Distributions/uniform]
  type = Uniform
[]

[Samplers]
  [sample]
    type = CartesianProduct
    linear_space_items = '0 0.1 11
                          0 0.1 11
                          0 0.1 11'
  []
  [test]
    type = MonteCarlo
    distributions = 'uniform uniform uniform'
    num_rows = 5
  []
[]

[VectorPostprocessors]
  [values]
    type = GFunction
    sampler = sample
    q_vector = '0 0 0'
    execute_on = INITIAL
    outputs = none
  []
[]

[Reporters]
  [results]
    type = SurrogateDerivativeTester
    model = surrogate
    sampler = test
    execute_on = final
    parallel_type = ROOT
  []
[]

[Surrogates]
  [surrogate]
    type = PolynomialRegressionSurrogate
    trainer = train
  []
[]

[Trainers]
  [train]
    type = PolynomialRegressionTrainer
    regression_type = "ols"
    sampler = sample
    response = values/g_values
    max_degree = 3
  []
[]

[Outputs]
  json = true
  execute_on = FINAL
[]
