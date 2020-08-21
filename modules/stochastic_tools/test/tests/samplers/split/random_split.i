[StochasticTools]
[]

[Distributions]
  [d0]
    type = Uniform
    lower_bound = 0
    upper_bound = 1
  []
  [d1]
    type = Uniform
    lower_bound = 10
    upper_bound = 11
  []
  [d2]
    type = Uniform
    lower_bound = 100
    upper_bound = 101
  []
[]

[Samplers]
  [sample]
    type = MonteCarlo
    distributions = 'd0 d1 d2'
    num_rows = 10
  []
  [split]
    type = RandomSplit
    sampler = sample
    num_rows = 9
  []
  [split_repeat]
    type = RandomSplit
    sampler = sample
    num_rows = 15
    allow_repeated_rows = true
  []
[]

[VectorPostprocessors]
  [data1]
    type = SamplerData
    sampler = split
    execute_on = 'initial'
  []
  [data2]
    type = SamplerData
    sampler = split_repeat
    execute_on = 'initial'
  []
[]

[Outputs]
  execute_on = 'INITIAL'
  csv = true
[]
