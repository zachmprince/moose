//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "GeneralReporter.h"
#include "ReporterContext.h"
#include "PerfGraph.h"
#include "nlohmann/json.h"

class PerfGraph;

class PerfGraphReporter : public GeneralReporter
{
public:
  static InputParameters validParams();
  PerfGraphReporter(const InputParameters & parameters);
  virtual void initialize() override {}
  virtual void finalize() override {}
  virtual void execute() override {}

protected:
  const bool & _all_proc;
  PerfGraph *& _value;
};

void to_json(nlohmann::json & json, PerfGraph * const & perf_graph);
