[StochasticTools]
[]

[Samplers/sample]
  type = CSVSampler
  samples_file = ../../samplers/csv/samples.csv
  execute_on = PRE_MULTIAPP_SETUP
  min_procs_per_row = 2
[]

[GlobalParams]
  sampler = sample
[]

[MultiApps/sub]
  type = SamplerFullSolveMultiApp
  input_files = sub_csv.i
  min_procs_per_app = 2
  mode = batch-reset
[]

[Controls/cli]
  type = MultiAppCommandLineControl
  multi_app = sub
  param_names = 'Postprocessors/pp1/scale_factor
                 Postprocessors/pp2/scale_factor
                 Postprocessors/pp3/scale_factor
                 Postprocessors/pp4/scale_factor
                 Postprocessors/pp5/scale_factor
                 Postprocessors/pp6/scale_factor'
[]

[Transfers]
  [rep]
    type = SamplerReporterTransfer
    multi_app = sub
    stochastic_reporter = reporter
    from_reporter = 'pp1/value pp2/value pp3/value pp4/value pp5/value pp6/value'
  []
[]

[VectorPostprocessors/vpp]
  type = StochasticResults
[]

[Reporters]
  [reporter]
    type = StochasticReporter
    outputs = none
  []
[]

[Outputs]
  csv = true
  execute_on = timestep_end
[]
