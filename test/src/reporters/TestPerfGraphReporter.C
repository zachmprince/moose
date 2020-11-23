//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "TestPerfGraphReporter.h"
#include "PerfGraphReporter.h"

registerMooseObject("MooseTestApp", TestPerfGraphReporter);

InputParameters
TestPerfGraphReporter::validParams()
{
  InputParameters params = GeneralReporter::validParams();
  params.addRequiredParam<ReporterName>("perf_graph_reporter", "PerfGraphReporter name/value_name");
  return params;
}

TestPerfGraphReporter::TestPerfGraphReporter(const InputParameters & parameters)
  : GeneralReporter(parameters),
    _perf_graph(getReporterValue<PerfGraph *>("perf_graph_reporter"))
{
}

void
TestPerfGraphReporter::execute()
{
  if (_perf_graph)
  {
    std::cout << "process " << processor_id() << std::endl;
    nlohmann::json json;
    to_json(json, _perf_graph);

    std::ostringstream msg;
    msg << "Error in converting PerfGraph to JSON. Values are of wrong type or do not exist:" << std::endl;
    if (checkPerfGraph(json, msg))
      mooseError(msg.str());
  }
  else if (processor_id() == 0)
    paramError("perf_graph_reporter", "PerfGraph has not been constructed.");
}

bool
TestPerfGraphReporter::checkPerfGraph(const nlohmann::json & json, std::ostringstream & msg) const
{
  std::vector<std::string> keys = {"level", "calls", "self", "self_avg", "self_percent", "total", "total_avg", "total_percent", "num_children"};

  std::ostringstream sub_msg;
  bool err = false;

  // See if all relevant values for this node exist
  for (const auto & key : keys)
    if (!json[key].is_number())
    {
      sub_msg << "    '" << key << "'" << std::endl;
      err = true;
    }

  // See if children entries exist
  unsigned int num_child = json["num_children"].is_number() ? (unsigned int)json["num_children"] : 0;
  for (unsigned int i = 0; i < num_child; ++i)
  {
    std::ostringstream section;
    section << "child_";
    section << std::setw(MooseUtils::numDigits(num_child)) << std::setfill('0') << i;
    if (json[section.str()].is_null())
    {
      sub_msg << "    '" << section.str() << "'" << std::endl;
      err = true;
    }
  }

  // Check name value
  if (!json["name"].is_string())
  {
    msg << "  'name' not found:" << std::endl;
    err = true;
  }
  else if (err)
    msg << "  " << json["name"] << ":" << std::endl;

  msg << sub_msg.str();

  // Go through children
  for (unsigned int i = 0; i < num_child; ++i)
  {
    std::ostringstream section;
    section << "child_";
    section << std::setw(MooseUtils::numDigits(num_child)) << std::setfill('0') << i;
    if (!json[section.str()].is_null())
      err += checkPerfGraph(json[section.str()], msg);
  }

  return err;
}
