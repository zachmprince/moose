//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PerfGraphReporter.h"

registerMooseObject("MooseApp", PerfGraphReporter);

InputParameters
PerfGraphReporter::validParams()
{
  InputParameters params = GeneralReporter::validParams();
  params.addClassDescription("Report performance information from simulation.");

  params.addParam<ReporterValueName>(
      "perf_graph_name", "perf_graph", "Desired name of reporter value.");
  params.addParam<bool>(
      "all_processes", false, "If true, a performance graph will be outputted for each processor.");
  return params;
}

PerfGraphReporter::PerfGraphReporter(const InputParameters & parameters)
  : GeneralReporter(parameters),
    _all_proc(getParam<bool>("all_processes")),
    _value(declareValue<PerfGraph *>("perf_graph_name",
                                     (_all_proc ? REPORTER_MODE_DISTRIBUTED : REPORTER_MODE_ROOT)))
{
  _value = nullptr;
  if (processor_id() == 0 || _all_proc)
    _value = &_perf_graph;
}

void
to_json(nlohmann::json & json, PerfGraph * const & perf_graph)
{
  perf_graph->print(json);
}
