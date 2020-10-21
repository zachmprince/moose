//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "SumVectors.h"

registerMooseObject("MooseApp", SumVectors);

defineLegacyParams(SumVectors);

InputParameters
SumVectors::validParams()
{
  InputParameters params = GeneralVectorPostprocessor::validParams();
  params.addClassDescription("Sums each element of given set of vectorpostprocessor vectors.");

  params.addParam<std::string>("vector_name", "Name of the column vector in this object, default is \"sum\"");
  params.addRequiredParam<std::vector<VectorPostprocessorName>>("input_vpp_names", "Names of the vectorpostprocessors containing the vectors.");
  params.addRequiredParam<std::vector<std::string>>("input_vector_names", "Name of the vector to extract from the listed input_vpp_names");
  params.addParam<std::vector<Real>>("scale_factors", std::vector<Real>(1, 1.0), "Values to scale each vector by, one value means same factor for each vector.");
  return params;
}

SumVectors::SumVectors(const InputParameters & parameters)
  : GeneralVectorPostprocessor(parameters),
    _values(declareVector(isParamValid("vector_name") ? getParam<std::string>("vector_name") : "sum")),
    _vpp_names(getParam<std::vector<VectorPostprocessorName>>("input_vpp_names")),
    _vector_names(getParam<std::vector<std::string>>("input_vector_names")),
    _num_vector(_vpp_names.size()),
    _scale_factors(getParam<std::vector<Real>>("scale_factors"))
{
  if (_vector_names.size() != _num_vector)
    paramError("input_vector_names", "Number of vpp names must be the same as the number of vector names.");

  if (_scale_factors.size() != 1 && _scale_factors.size() != _num_vector)
    paramError("scale_factors", "Number of scale factors must either 1 or number of vectors.");
}

void
SumVectors::initialize()
{
  _values.clear();
  _input_vectors.clear();
  _input_vectors.reserve(_num_vector);
  for (unsigned int j = 0; j < _num_vector; ++j)
  {
    _input_vectors.push_back(&getVectorPostprocessorValueByName(_vpp_names[j], _vector_names[j]));
    bool distributed = isVectorPostprocessorDistributedByName(_vpp_names[j]);

    if (isDistributed() && !distributed)
      mooseError(_vpp_names[j], " is not distributed, but this VPP is.");
    else if (!isDistributed() && distributed)
      mooseError(_vpp_names[j], " is distributed, but this VPP is not.");
    else if (j == 0)
      _values.resize(_input_vectors[j]->size());
    else if (_input_vectors[j]->size() != _values.size())
      mooseError("Vector ", _vpp_names[j], "/", _vector_names[j], "has ", _input_vectors[j]->size(), " entries, but other vectors have ", _values.size(), " entries");
  }
}

void
SumVectors::execute()
{
  for (unsigned int i = 0; i < _values.size(); ++i)
    for (unsigned int j = 0; j < _num_vector; ++j)
      _values[i] += (*_input_vectors[j])[i] * (_scale_factors.size() == 1 ? _scale_factors[0] : _scale_factors[j]);
}
