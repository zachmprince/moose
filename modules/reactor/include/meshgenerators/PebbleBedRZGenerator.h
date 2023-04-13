//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "MeshGenerator.h"

class PebbleBedRZGenerator : public MeshGenerator
{
public:
  static InputParameters validParams();

  PebbleBedRZGenerator(const InputParameters & parameters);

  std::unique_ptr<MeshBase> generate() override;

protected:
  static MultiMooseEnum blockEnum();
  static MultiMooseEnum boundaryEnum();

  // left bottom right top
  static void buildQuadBlock(MeshBase & mesh,
                             const std::vector<std::vector<Node *>> & nodes,
                             SubdomainID block_id,
                             std::array<BoundaryID, 4> boundary_ids = {Moose::INVALID_BOUNDARY_ID,
                                                                       Moose::INVALID_BOUNDARY_ID,
                                                                       Moose::INVALID_BOUNDARY_ID,
                                                                       Moose::INVALID_BOUNDARY_ID});
  static void buildQuadBlock(MeshBase & mesh,
                             const std::vector<std::vector<Node *>> & nodes,
                             SubdomainID block_id,
                             const std::array<std::vector<BoundaryID>, 4> & boundary_ids);

  const Real _core_height;
  const Real _core_radius;
  const unsigned int _core_naxial;
  const Real _conus_height;
  const Real _conus_radius;
  const unsigned int _conus_naxial;
  const unsigned int _streamline_naxial;
  const unsigned int _streamline_nz;
  const unsigned int _streamline_nr;
  const Real _conus_channel_height;
  const unsigned int _conus_channel_naxial;
  const Real _lower_plenum_height;
  const unsigned int _lower_plenum_naxial;
  const Real _lower_reflector_height;
  const unsigned int _lower_reflector_naxial;
  const Real _upper_cavity_height;
  const unsigned int _upper_cavity_naxial;
  const Real _upper_reflector_height;
  const unsigned int _upper_reflector_naxial;
  const Real _cr_buffer_width;
  const unsigned int _cr_buffer_nradial;
  const Real _cr_width;
  const unsigned int _cr_nradial;
  const unsigned int _radial_reflector_nradial;
  const Real _riser_radius;
  const Real _riser_width;
  const unsigned int _riser_nradial;
  const Real _inlet_height;
  const Real _inlet_width;
  const unsigned int _inlet_naxial;
  const Real _barrel_gap_width;
  const unsigned int _barrel_gap_nradial;
  const Real _barrel_width;
  const unsigned int _barrel_nradial;
  const Real _rpv_gap_width;
  const unsigned int _rpv_gap_nradial;
  const Real _rpv_width;
  const unsigned int _rpv_nradial;

  unsigned int _num_streamlines;
  std::vector<std::vector<Point>> _streamline_edges;
  std::map<std::string, std::pair<SubdomainName, SubdomainID>> _block_map;
  std::map<std::string, std::pair<BoundaryName, BoundaryID>> _boundary_map;
};
