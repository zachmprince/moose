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

class TestPerfGraphReporter : public GeneralReporter
{
public:
  static InputParameters validParams();
  TestPerfGraphReporter(const InputParameters & parameters);
  virtual void initialize() override {}
  virtual void finalize() override {}
  virtual void execute() override;

protected:
  bool checkPerfGraph(const nlohmann::json & json, std::ostringstream & msg) const;
  PerfGraph * const & _perf_graph;
};
