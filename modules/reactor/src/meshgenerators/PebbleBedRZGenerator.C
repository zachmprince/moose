//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PebbleBedRZGenerator.h"

#include "IndirectSort.h"
#include "LinearInterpolation.h"

#include "libmesh/replicated_mesh.h"
#include "libmesh/face_quad4.h"
#include "libmesh/boundary_info.h"

registerMooseObject("ReactorApp", PebbleBedRZGenerator);

InputParameters
PebbleBedRZGenerator::validParams()
{
  InputParameters params = MeshGenerator::validParams();
  params.addClassDescription("This generator can be used to create a 2D mesh of a pebble bed "
                             "reactor in RZ-cylindrical geometry.");

  // Core parameters
  params.addRequiredParam<Real>("core_height",
                                "Height of the pebble bed core region (excluding conus).");
  params.addRequiredParam<Real>("core_radius",
                                "Radius of the pebble bed core region (excluding conus).");
  params.addRequiredParam<unsigned int>(
      "core_num_axial_layers", "Number of axial layers in pebble bed core (exluding conus).");
  params.addRequiredParam<Real>("conus_height", "Height of conus above discharge chute.");
  params.addRequiredParam<Real>("conus_radius", "Radius of conus at discharge chute.");
  params.addRequiredParam<unsigned int>("conus_num_axial_layers",
                                        "Number of axial layers in conus.");
  params.addRequiredParam<std::vector<std::vector<Real>>>(
      "streamline_points",
      "Normalized R (radius) and Z (height) locations defining the streamline outer radii. A "
      "radius of 0 corresponds to the center of the core and 1 corresponds to the outer edge of "
      "the pebble bed region. A height of 0 corresponds to the bottom of the conus and 1 "
      "corresponds to the top of the core. Streamlines must "
      "be ordered from smallest to largest radius.");
  params.addParam<unsigned int>("streamline_axial_refinement",
                                1,
                                "Number of elements between each axial streamline segment.");
  params.addParam<unsigned int>(
      "streamline_radial_refinement", 1, "Number of elements radially for each streamline.");

  // Lower reactor parameters
  params.addRequiredParam<Real>("conus_channel_height", "Height of conus channel.");
  params.addParam<unsigned int>(
      "conus_channel_num_axial_layers", 6, "Number of elements axially in conus channel.");
  params.addRequiredParam<Real>("lower_plenum_height", "Height of lower plenum.");
  params.addParam<unsigned int>(
      "lower_plenum_num_axial_layers", 6, "Number of elements axially in lower plenum.");
  params.addRequiredParam<Real>("lower_reflector_height",
                                "Height of lower reflector below lower plenum.");
  params.addParam<unsigned int>(
      "lower_reflector_num_axial_layers",
      4,
      "Number of elements axially in lower reflector below lower plenum.");

  // Upper reactor parameters
  params.addRequiredParam<Real>("upper_cavity_height", "Height of upper cavity.");
  params.addParam<unsigned int>(
      "upper_cavity_num_axial_layers", 2, "Number of elements axially in upper cavity.");
  params.addRequiredParam<Real>("upper_reflector_height",
                                "Height of upper reflector above upper cavity.");
  params.addParam<unsigned int>(
      "upper_reflector_num_axial_layers",
      4,
      "Number of elements axially in upper reflector above the upper cavity.");

  // Control rod parameters
  params.addRequiredParam<Real>("control_rod_buffer_width",
                                "Width of buffer between control rod and core.");
  params.addParam<unsigned int>("control_rod_buffer_num_radial_layers",
                                2,
                                "Number of elements radially in control rod buffer.");
  params.addRequiredParam<Real>("control_rod_channel_width", "Width of control rod channel.");
  params.addParam<unsigned int>("control_rod_channel_num_radial_layers",
                                2,
                                "Number of elements radially in control rod channel.");

  // Radial reflector, inlet, and riser parameters
  params.addParam<unsigned int>(
      "radial_reflector_num_radial_layers",
      4,
      "Number of elements radially in radial reflector. Must be divisible by 2.");
  params.addRequiredParam<Real>("riser_radius",
                                "Distance from center of fore to inside edge of riser.");
  params.addRequiredParam<Real>("riser_width", "Width of coolant riser.");
  params.addParam<unsigned int>(
      "riser_num_radial_layers", 2, "Number of elements radially in coolant riser.");
  params.addRequiredParam<Real>("inlet_height", "Width of coolant inlet.");
  params.addRequiredParam<Real>("inlet_width", "Width of coolant inlet.");
  params.addParam<unsigned int>(
      "inlet_num_axial_layers", 2, "Number of elements axially in coolant inlet.");

  // Barrel and RPV parameters
  params.addRequiredParam<Real>("barrel_gap_width",
                                "Width of gap between barrel and radial reflector.");
  params.addParam<unsigned int>(
      "barrel_gap_num_radial_layers",
      2,
      "Number of radial elements in gap between barrel and radial reflector.");
  params.addRequiredParam<Real>("barrel_width", "Width of barrel.");
  params.addParam<unsigned int>(
      "barrel_num_radial_layers", 2, "Number of radial elements in barrel.");
  params.addRequiredParam<Real>("rpv_gap_width",
                                "Width of gap between reactor pressure vessel and barrel.");
  params.addParam<unsigned int>(
      "rpv_gap_num_radial_layers",
      2,
      "Number of radial elements in gap between reactor pressure vessel and barrel.");
  params.addRequiredParam<Real>("rpv_width", "Width of reactor pressure vessel.");
  params.addParam<unsigned int>(
      "rpv_num_radial_layers", 2, "Number of radial elements in reactor pressure vessel.");

  // Subdomain parameters
  params.addParam<std::vector<SubdomainName>>(
      "streamline_names", std::vector<SubdomainName>(), "Name of each streamline subdomain.");
  params.addParam<std::vector<SubdomainID>>(
      "streamline_ids", std::vector<SubdomainID>(), "ID of each streamline subdomain.");
  params.addParam<MultiMooseEnum>(
      "blocks", blockEnum(), "Parts of mesh to assign subdomain names and/or IDs");
  params.addParam<std::vector<SubdomainName>>(
      "block_names", std::vector<SubdomainName>(), "Name of each subdomain specified in 'blocks'");
  params.addParam<std::vector<SubdomainID>>(
      "block_ids", std::vector<SubdomainID>(), "ID of each subdomain specified in 'blocks'");

  // Boundary parameters
  params.addParam<MultiMooseEnum>(
      "boundaries", boundaryEnum(), "Parts of mesh to assign boundary names and/or IDs");
  params.addParam<std::vector<BoundaryName>>("boundary_names",
                                             "Name of each boundary specified in 'boundaries'");
  params.addParam<std::vector<BoundaryID>>("boundary_ids",
                                           "ID of each boundary specified in 'boundaries'");

  return params;
}

PebbleBedRZGenerator::PebbleBedRZGenerator(const InputParameters & parameters)
  : MeshGenerator(parameters),
    _core_height(getParam<Real>("core_height")),
    _core_radius(getParam<Real>("core_radius")),
    _core_naxial(getParam<unsigned int>("core_num_axial_layers")),
    _conus_height(getParam<Real>("conus_height")),
    _conus_radius(getParam<Real>("conus_radius")),
    _conus_naxial(getParam<unsigned int>("conus_num_axial_layers")),
    _streamline_naxial(_core_naxial + _conus_naxial),
    _streamline_nz(getParam<unsigned int>("streamline_axial_refinement")),
    _streamline_nr(getParam<unsigned int>("streamline_radial_refinement")),
    _conus_channel_height(getParam<Real>("conus_channel_height")),
    _conus_channel_naxial(getParam<unsigned int>("conus_channel_num_axial_layers")),
    _lower_plenum_height(getParam<Real>("lower_plenum_height")),
    _lower_plenum_naxial(getParam<unsigned int>("lower_plenum_num_axial_layers")),
    _lower_reflector_height(getParam<Real>("lower_reflector_height")),
    _lower_reflector_naxial(getParam<unsigned int>("lower_reflector_num_axial_layers")),
    _upper_cavity_height(getParam<Real>("upper_cavity_height")),
    _upper_cavity_naxial(getParam<unsigned int>("upper_cavity_num_axial_layers")),
    _upper_reflector_height(getParam<Real>("upper_reflector_height")),
    _upper_reflector_naxial(getParam<unsigned int>("upper_reflector_num_axial_layers")),
    _cr_buffer_width(getParam<Real>("control_rod_buffer_width")),
    _cr_buffer_nradial(getParam<unsigned int>("control_rod_buffer_num_radial_layers")),
    _cr_width(getParam<Real>("control_rod_channel_width")),
    _cr_nradial(getParam<unsigned int>("control_rod_channel_num_radial_layers")),
    _radial_reflector_nradial(getParam<unsigned int>("radial_reflector_num_radial_layers")),
    _riser_radius(getParam<Real>("riser_radius")),
    _riser_width(getParam<Real>("riser_width")),
    _riser_nradial(getParam<unsigned int>("riser_num_radial_layers")),
    _inlet_height(getParam<Real>("inlet_height")),
    _inlet_width(getParam<Real>("inlet_width")),
    _inlet_naxial(getParam<unsigned int>("inlet_num_axial_layers")),
    _barrel_gap_width(getParam<Real>("barrel_gap_width")),
    _barrel_gap_nradial(getParam<unsigned int>("barrel_gap_num_radial_layers")),
    _barrel_width(getParam<Real>("barrel_width")),
    _barrel_nradial(getParam<unsigned int>("barrel_num_radial_layers")),
    _rpv_gap_width(getParam<Real>("rpv_gap_width")),
    _rpv_gap_nradial(getParam<unsigned int>("rpv_gap_num_radial_layers")),
    _rpv_width(getParam<Real>("rpv_width")),
    _rpv_nradial(getParam<unsigned int>("rpv_num_radial_layers"))
{
  // Get normalized streamline edges and add middle and outer edges
  auto norm_streamline = getParam<std::vector<std::vector<Real>>>("streamline_points");
  _num_streamlines = norm_streamline.size() + 1;
  norm_streamline.insert(norm_streamline.begin(), {0, 1, 0, 0}); // insert middle edge
  norm_streamline.push_back({1,
                             1,
                             1,
                             _conus_height / (_core_height + _conus_height),
                             _conus_radius / _core_radius,
                             0}); // insert outer edge
  // Create streamline edges
  const Real conus_bot =
      _lower_reflector_height + _lower_plenum_height + _conus_channel_height - _conus_height;
  const Real conus_top = conus_bot + _conus_height;
  const Real core_top = conus_top + _core_height;
  _streamline_edges.assign(norm_streamline.size(), std::vector<Point>(_streamline_naxial + 1));
  for (unsigned int s = 0; s <= _num_streamlines; ++s)
  {
    // Transform inputted points
    const std::size_t np = norm_streamline[s].size() / 2;
    std::vector<Real> lir(np);
    std::vector<Real> liz(np);
    for (std::size_t p = 0; p < np; ++p)
    {
      lir[p] = norm_streamline[s][2 * p] * _core_radius;
      liz[p] = norm_streamline[s][2 * p + 1] * (_core_height + _conus_height) + conus_bot;
    }
    // Create interpolator
    std::vector<std::size_t> sorted_ind;
    Moose::indirectSort(liz.begin(), liz.end(), sorted_ind);
    Moose::applyIndices(lir, sorted_ind);
    Moose::applyIndices(liz, sorted_ind);
    LinearInterpolation li(liz, lir);
    // Loop through axial locations
    std::vector<Point> & edge = _streamline_edges[s];
    for (unsigned int i = 0; i <= _streamline_naxial; ++i)
    {
      if (i == 0)
        edge[i](1) = core_top;
      else if (i <= _core_naxial)
        edge[i](1) = edge[i - 1](1) - _core_height / _core_naxial;
      else
        edge[i](1) = edge[i - 1](1) - _conus_height / _conus_naxial;
      edge[i](0) = li.sample(edge[i](1));
    }
  }
  // Insert axial refinement
  for (auto & edge : _streamline_edges)
    for (unsigned int i = _streamline_naxial; i > 0; --i)
    {
      std::vector<Point> pt_insert(_streamline_nz - 1);
      for (unsigned int ii = 1; ii < _streamline_nz; ++ii)
      {
        const Real fac = (Real)ii / (Real)_streamline_nz;
        pt_insert[ii - 1] = (1.0 - fac) * edge[i - 1] + fac * edge[i];
      }
      edge.insert(edge.begin() + i, pt_insert.begin(), pt_insert.end());
    }
  // Insert radial refinement
  for (unsigned int s = _num_streamlines; s > 0; --s)
    for (unsigned int ss = 1; ss < _streamline_nr; ++ss)
    {
      const Real fac = (Real)ss / (Real)_streamline_nr;
      std::vector<Point> new_s(_streamline_edges[s].size());
      for (unsigned int i = 0; i < new_s.size(); ++i)
        new_s[i] = fac * _streamline_edges[s - 1][i] + (1.0 - fac) * _streamline_edges[s][i];
      _streamline_edges.insert(_streamline_edges.begin() + s, new_s);
    }

  // Gather subdomain parameters
  std::set<SubdomainID> used_ids = {0}; // Used to make sure we don't overwrite inputted ids
  // Streamline subdomain parameters
  const auto & snms = getParam<std::vector<SubdomainName>>("streamline_names");
  if (!snms.empty() && snms.size() != _num_streamlines)
    paramError("streamline_names",
               "The number of specified names must match the number of streamlines (",
               _num_streamlines,
               ").");
  const auto & sids = getParam<std::vector<SubdomainID>>("streamline_ids");
  if (!sids.empty() && sids.size() != _num_streamlines)
    paramError("streamline_ids",
               "The number of specified IDs must match the number of streamlines (",
               _num_streamlines,
               ").");
  used_ids.insert(sids.begin(), sids.end());
  // Other subdomain parameters
  const auto & blocks = getParam<MultiMooseEnum>("blocks");
  const auto & bnms = getParam<std::vector<SubdomainName>>("block_names");
  if (!bnms.empty() && blocks.size() != bnms.size())
    paramError("block_names", "'block_names' must be the same size as 'blocks'.");
  const auto & bids = getParam<std::vector<SubdomainID>>("block_ids");
  if (!bids.empty() && blocks.size() != bids.size())
    paramError("block_ids", "'block_ids' must be the same size as 'blocks'.");
  used_ids.insert(bids.begin(), bids.end());
  // Insert streamline subdomains
  for (unsigned int s = 0; s < _num_streamlines; ++s)
  {
    SubdomainName name = "streamline" + std::to_string(s + 1);
    auto & block = _block_map[name];
    block.first = snms.empty() ? name : snms[s];
    block.second = sids.empty() ? *used_ids.rbegin() + 1 : sids[s];
    used_ids.insert(block.second);
  }
  // Insert other subdomains
  for (const auto & enum_name : blocks.getNames())
  {
    SubdomainName name = MooseUtils::toLower(enum_name);
    auto it = std::find(blocks.begin(), blocks.end(), enum_name);
    const auto ind = std::distance(blocks.begin(), it);
    auto & block = _block_map[name];
    block.first = it == blocks.end() || bnms.empty() ? name : bnms[ind];
    block.second = it == blocks.end() || bnms.empty() ? *used_ids.rbegin() + 1 : bids[ind];
    used_ids.insert(block.second);
  }
  // Check integrity
  for (const auto & it1 : _block_map)
    for (const auto & it2 : _block_map)
    {
      if (it1.second.first == it2.second.first && it1.second.second != it2.second.second)
        mooseError("Subdomain with name '", it1.second.first, "' has multiple specified IDs");
      if (it1.second.first != it2.second.first && it1.second.second == it2.second.second)
        mooseError("Subdomain with ID '", it1.second.second, "' has multiple specified names");
    }

  // Boundary parameters
  used_ids = {0}; // Used to make sure we don't overwrite inputted ids
  const auto & boundaries = getParam<MultiMooseEnum>("boundaries");
  const auto & bdnms = getParam<std::vector<BoundaryName>>("boundary_names");
  if (!bdnms.empty() && boundaries.size() != bdnms.size())
    paramError("boundary_names", "'boundary_names' must be the same size as 'boundaries'.");
  const auto & bdids = getParam<std::vector<BoundaryID>>("boundary_ids");
  if (!bdids.empty() && boundaries.size() != bdids.size())
    paramError("boundary_ids", "'boundary_ids' must be the same size as 'boundaries'.");
  used_ids.insert(bdids.begin(), bdids.end());
  for (const auto & enum_name : boundaries.getNames())
  {
    BoundaryName name = MooseUtils::toLower(enum_name);
    auto it = std::find(boundaries.begin(), boundaries.end(), enum_name);
    const auto ind = std::distance(boundaries.begin(), it);
    auto & bnd = _boundary_map[name];
    bnd.first = it == blocks.end() || bdnms.empty() ? name : bdnms[ind];
    bnd.second = it == blocks.end() || bdnms.empty() ? *used_ids.rbegin() + 1 : bdids[ind];
    used_ids.insert(bnd.second);
  }
  // Add boundaries for streamlines
  _boundary_map.emplace("streamline_inlet",
                        std::make_pair("streamline_inlet", *used_ids.rbegin() + 1));
  _boundary_map.emplace("streamline_outlet",
                        std::make_pair("streamline_outlet", *used_ids.rbegin() + 2));
  // Check integrity
  for (const auto & it1 : _boundary_map)
    for (const auto & it2 : _boundary_map)
    {
      if (it1.second.first == it2.second.first && it1.second.second != it2.second.second)
        mooseError("Boundary with name '", it1.second.first, "' has multiple specified IDs");
      if (it1.second.first != it2.second.first && it1.second.second == it2.second.second)
        mooseError("Boundary with ID '", it1.second.second, "' has multiple specified names");
    }
}

MultiMooseEnum
PebbleBedRZGenerator::blockEnum()
{
  return MultiMooseEnum(
      "conus_channel lower_plenum lower_reflector dicharge_chute upper_cavity upper_reflector "
      "cr_buffer_upper_reflector cr_buffer_upper_cavity cr_buffer_radial_reflector "
      "cr_channel_upper_reflector cr_channel_upper_cavity cr_channel_radial_reflector "
      "radial_reflector riser inlet "
      "riser_upper_cavity hotleg barrel_gap barrel rpv_gap rpv",
      "");
}

MultiMooseEnum
PebbleBedRZGenerator::boundaryEnum()
{
  return MultiMooseEnum("inlet outlet bottom top core_inner inner reactor_outer outer wall", "");
}

std::unique_ptr<MeshBase>
PebbleBedRZGenerator::generate()
{
  auto mesh = buildMeshBaseObject();
  dof_id_type node_id = 0;
  const BoundaryID ibid = Moose::INVALID_BOUNDARY_ID; // Convenience

  // Build core mesh with streamlines
  auto streamline_id = mesh->add_elem_integer("pebble_streamline_id");
  auto streamline_layer_id = mesh->add_elem_integer("pebble_streamline_layer_id");
  std::vector<std::vector<Node *>> streamline_nodes(_streamline_edges.size());
  for (unsigned int s = 0; s < streamline_nodes.size(); ++s)
  {
    streamline_nodes[s].resize(_streamline_edges[s].size());
    for (unsigned int i = 0; i < streamline_nodes[s].size(); ++i)
      streamline_nodes[s][i] = mesh->add_point(_streamline_edges[s][i], node_id++);
  }
  for (unsigned int s = 0; s < streamline_nodes.size() - 1; ++s)
    for (unsigned int i = 0; i < streamline_nodes[s].size() - 1; ++i)
    {
      Elem * elem = mesh->add_elem(new Quad4);
      elem->set_node(0) = streamline_nodes[s][i];         // Top left
      elem->set_node(1) = streamline_nodes[s][i + 1];     // Bottom left
      elem->set_node(2) = streamline_nodes[s + 1][i + 1]; // Bottom right
      elem->set_node(3) = streamline_nodes[s + 1][i];     // Top right

      const unsigned int ns = s / _streamline_nr;
      const unsigned int nl = i / _streamline_nz;
      elem->subdomain_id() = _block_map["streamline" + std::to_string(ns + 1)].second;
      elem->set_extra_integer(streamline_id, ns);
      elem->set_extra_integer(streamline_layer_id, nl);

      if (s == 0)
        mesh->get_boundary_info().add_side(
            elem, 0, {_boundary_map["core_inner"].second, _boundary_map["inner"].second});
      if (s == streamline_nodes.size() - 2 && i < _core_naxial * _streamline_nz)
        mesh->get_boundary_info().add_side(elem, 2, _boundary_map["wall"].second);
      if (i == 0)
        mesh->get_boundary_info().add_side(elem, 3, _boundary_map["streamline_inlet"].second);
      if (i == streamline_nodes[s].size() - 2)
        mesh->get_boundary_info().add_side(
            elem, 1, {_boundary_map["streamline_outlet"].second, _boundary_map["wall"].second});
    }

  // Build trapezoidal conus channel
  const Real channel_bot = _lower_reflector_height + _lower_plenum_height;
  const Real conus_chute_height = _conus_channel_height - _conus_height;
  const Real chute_dz = conus_chute_height / _conus_channel_naxial;
  const Real channel_dz = _conus_channel_height / _conus_channel_naxial;
  std::vector<std::vector<Node *>> conus_channel_nodes(_conus_naxial * _streamline_nz + 1);
  for (unsigned int i = 0; i < conus_channel_nodes.size(); ++i)
  {
    conus_channel_nodes[i].resize(_conus_channel_naxial + 1);
    // Radial location is defined by streamline axial location in conus
    Node * streamline_node = streamline_nodes.back().rbegin()[i];
    const Real r = (*streamline_node)(0);
    // Just copy pointer from streamline for top of channel
    conus_channel_nodes[i][0] = streamline_node;
    for (unsigned int c = 1; c < conus_channel_nodes[i].size(); ++c)
    {
      const Real left_z = conus_chute_height - c * chute_dz;
      const Real right_z = _conus_channel_height - c * channel_dz;
      const Real z =
          left_z + (r - _conus_radius) * (right_z - left_z) / (_core_radius - _conus_radius);
      conus_channel_nodes[i][c] = mesh->add_point(Point(r, z + channel_bot, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 conus_channel_nodes,
                 _block_map["conus_channel"].second,
                 {_boundary_map["wall"].second, ibid, _boundary_map["wall"].second, ibid});

  // Build square lower plenum
  const Real plenum_dz = _lower_plenum_height / _lower_plenum_naxial;
  std::vector<std::vector<Node *>> plenum_nodes(_conus_naxial * _streamline_nz + 1);
  for (unsigned int i = 0; i < plenum_nodes.size(); ++i)
  {
    plenum_nodes[i].resize(_lower_plenum_naxial + 1);
    plenum_nodes[i][0] = conus_channel_nodes[i].back();
    const Real r = (*plenum_nodes[i][0])(0);
    const Real z_start = (*plenum_nodes[i][0])(1);
    for (unsigned int j = 1; j < plenum_nodes[i].size(); ++j)
      plenum_nodes[i][j] = mesh->add_point(Point(r, z_start - j * plenum_dz, 0), node_id++);
  }
  buildQuadBlock(*mesh,
                 plenum_nodes,
                 _block_map["lower_plenum"].second,
                 {_boundary_map["wall"].second, _boundary_map["wall"].second, ibid, ibid});

  // Build left half of lower reflector
  const Real lower_reflector_dz = _lower_reflector_height / _lower_reflector_naxial;
  std::vector<std::vector<Node *>> left_lower_reflector_nodes(_conus_naxial * _streamline_nz + 1);
  for (unsigned int i = 0; i < left_lower_reflector_nodes.size(); ++i)
  {
    left_lower_reflector_nodes[i].resize(_lower_reflector_naxial + 1);
    left_lower_reflector_nodes[i][0] = plenum_nodes[i].back();
    const Real r = (*left_lower_reflector_nodes[i][0])(0);
    const Real z_start = (*left_lower_reflector_nodes[i][0])(1);
    for (unsigned int j = 1; j < left_lower_reflector_nodes[i].size(); ++j)
      left_lower_reflector_nodes[i][j] =
          mesh->add_point(Point(r, z_start - j * lower_reflector_dz, 0), node_id++);
  }
  buildQuadBlock(*mesh,
                 left_lower_reflector_nodes,
                 _block_map["lower_reflector"].second,
                 {ibid, _boundary_map["bottom"].second, ibid, ibid});

  // Build discharge chute
  std::vector<std::vector<Node *>> chute_nodes(streamline_nodes.size());
  // Insert nodes from channel, plenum, and reflector
  chute_nodes.back().insert(
      chute_nodes.back().end(), conus_channel_nodes[0].begin(), conus_channel_nodes[0].end());
  chute_nodes.back().insert(
      chute_nodes.back().end(), plenum_nodes[0].begin() + 1, plenum_nodes[0].end());
  chute_nodes.back().insert(chute_nodes.back().end(),
                            left_lower_reflector_nodes[0].begin() + 1,
                            left_lower_reflector_nodes[0].end());
  for (unsigned int i = 0; i < chute_nodes.size() - 1; ++i)
  {
    chute_nodes[i].resize(chute_nodes.back().size());
    chute_nodes[i][0] = streamline_nodes[i].back();
    for (unsigned int j = 1; j < chute_nodes[i].size(); ++j)
      chute_nodes[i][j] = mesh->add_point(
          Point((*chute_nodes[i][0])(0), (*chute_nodes.back()[j])(1), 0), node_id++);
  }
  buildQuadBlock(*mesh,
                 chute_nodes,
                 _block_map["dicharge_chute"].second,
                 {_boundary_map["inner"].second, _boundary_map["bottom"].second, ibid, ibid});

  // Build upper cavity
  const Real upper_cavity_dz = _upper_cavity_height / _upper_cavity_naxial;
  std::vector<std::vector<Node *>> upper_cavity_nodes(streamline_nodes.size());
  for (unsigned int i = 0; i < upper_cavity_nodes.size(); ++i)
  {
    upper_cavity_nodes[i].resize(_upper_cavity_naxial + 1);
    upper_cavity_nodes[i].back() = streamline_nodes[i][0];
    const Real r = (*streamline_nodes[i][0])(0);
    Real z = (*streamline_nodes[i][0])(1) + _upper_cavity_height;
    for (unsigned int j = 0; j < _upper_cavity_naxial; ++j)
      upper_cavity_nodes[i][j] = mesh->add_point(Point(r, z - j * upper_cavity_dz, 0), node_id++);
  }
  std::array<std::vector<BoundaryID>, 4> upper_cavity_bids;
  upper_cavity_bids[0] = {_boundary_map["inner"].second, _boundary_map["core_inner"].second};
  upper_cavity_bids[3] = {_boundary_map["wall"].second};
  buildQuadBlock(*mesh, upper_cavity_nodes, _block_map["upper_cavity"].second, upper_cavity_bids);

  // Build left part of upper reflector
  const Real upper_reflector_dz = _upper_reflector_height / _upper_reflector_naxial;
  std::vector<std::vector<Node *>> left_upper_reflector_nodes(upper_cavity_nodes.size());
  for (unsigned int i = 0; i < left_upper_reflector_nodes.size(); ++i)
  {
    left_upper_reflector_nodes[i].resize(_upper_reflector_naxial + 1);
    left_upper_reflector_nodes[i].back() = upper_cavity_nodes[i][0];
    const Real r = (*upper_cavity_nodes[i][0])(0);
    Real z = (*upper_cavity_nodes[i][0])(1) + _upper_reflector_height;
    for (unsigned int j = 0; j < _upper_reflector_naxial; ++j)
      left_upper_reflector_nodes[i][j] =
          mesh->add_point(Point(r, z - j * upper_reflector_dz, 0), node_id++);
  }
  buildQuadBlock(*mesh,
                 left_upper_reflector_nodes,
                 _block_map["upper_reflector"].second,
                 {_boundary_map["inner"].second, ibid, ibid, _boundary_map["top"].second});

  // Build bottom of control rod buffer
  const Real cr_buffer_dr = _cr_buffer_width / _cr_buffer_nradial;
  std::vector<std::vector<Node *>> bot_cr_buffer_nodes(_cr_buffer_nradial + 1);
  for (unsigned int i = 0; i < bot_cr_buffer_nodes.size(); ++i)
  {
    bot_cr_buffer_nodes[i].resize(_core_naxial * _streamline_nz + 1);
    const Real r = _core_radius + i * cr_buffer_dr;
    for (unsigned int j = 0; j < bot_cr_buffer_nodes[i].size(); ++j)
    {
      if (i == 0)
        bot_cr_buffer_nodes[i][j] = streamline_nodes.back()[j];
      else
        bot_cr_buffer_nodes[i][j] =
            mesh->add_point(Point(r, (*streamline_nodes.back()[j])(1), 0), node_id++);
    }
  }
  buildQuadBlock(*mesh, bot_cr_buffer_nodes, _block_map["cr_buffer_radial_reflector"].second);

  // Build bottom of control rod channel
  const Real cr_dr = _cr_width / _cr_nradial;
  std::vector<std::vector<Node *>> bot_cr_nodes(_cr_nradial + 1);
  for (unsigned int i = 0; i < bot_cr_nodes.size(); ++i)
  {
    bot_cr_nodes[i].resize(bot_cr_buffer_nodes.back().size());
    const Real r = _core_radius + _cr_buffer_width + i * cr_dr;
    for (unsigned int j = 0; j < bot_cr_nodes[i].size(); ++j)
    {
      if (i == 0)
        bot_cr_nodes[i][j] = bot_cr_buffer_nodes.back()[j];
      else
        bot_cr_nodes[i][j] =
            mesh->add_point(Point(r, (*bot_cr_buffer_nodes.back()[j])(1), 0), node_id++);
    }
  }
  buildQuadBlock(*mesh, bot_cr_nodes, _block_map["cr_channel_radial_reflector"].second);

  // Build right part of radial reflector
  const Real radref_radius = _riser_radius + _inlet_width;
  const Real riser_outer_radius = _riser_radius + _riser_width;
  const Real riser_bot =
      _lower_reflector_height + _lower_plenum_height + _conus_channel_height + _inlet_height;
  const Real radref_top = riser_bot - _inlet_height + _core_height;
  const Real right_radref_dr =
      (radref_radius - riser_outer_radius) / (_radial_reflector_nradial / 2);
  const unsigned int right_radref_naxial = _core_naxial * _streamline_nz - _inlet_naxial;
  const Real right_radref_dz = (radref_top - riser_bot) / right_radref_naxial;
  std::vector<std::vector<Node *>> right_radref_nodes(_radial_reflector_nradial / 2 + 1);
  for (unsigned int i = 0; i < right_radref_nodes.size(); ++i)
  {
    right_radref_nodes[i].resize(right_radref_naxial + 1);
    const Real r = riser_outer_radius + i * right_radref_dr;
    for (unsigned int j = 0; j < right_radref_nodes[i].size(); ++j)
    {
      const Real z = radref_top - j * right_radref_dz;
      right_radref_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 right_radref_nodes,
                 _block_map["radial_reflector"].second,
                 {ibid, ibid, _boundary_map["reactor_outer"].second, ibid});

  // Build riser
  const Real riser_dr = _riser_width / _riser_nradial;
  std::vector<std::vector<Node *>> riser_nodes(_riser_nradial + 1);
  riser_nodes.back() = right_radref_nodes[0];
  for (unsigned int i = 0; i < _riser_nradial; ++i)
  {
    riser_nodes[i].resize(riser_nodes.back().size());
    const Real r = _riser_radius + i * riser_dr;
    for (unsigned int j = 0; j < riser_nodes[i].size(); ++j)
    {
      const Real z = (*riser_nodes.back()[j])(1);
      riser_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 riser_nodes,
                 _block_map["riser"].second,
                 {_boundary_map["wall"].second, ibid, _boundary_map["wall"].second, ibid});

  // Build inlet
  const Real inlet_dz = _inlet_height / _inlet_naxial;
  std::vector<std::vector<Node *>> inlet_nodes(_riser_nradial + _radial_reflector_nradial / 2 + 1);
  for (unsigned int i = 0; i < inlet_nodes.size(); ++i)
  {
    inlet_nodes[i].resize(_inlet_naxial + 1);
    inlet_nodes[i][0] =
        i < _riser_nradial ? riser_nodes[i].back() : right_radref_nodes[i - _riser_nradial].back();
    const Real r = (*inlet_nodes[i][0])(0);
    for (unsigned int j = 1; j < inlet_nodes[i].size(); ++j)
    {
      const Real z = (*inlet_nodes[i][j - 1])(1) - inlet_dz;
      inlet_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  // Need to separate inlet quad build so the wall boundary can be applied to parts of it
  // First we'll do the riser side, which as the wall on the left and bottom
  std::vector<std::vector<Node *>> riser_inlet_nodes(inlet_nodes.begin(),
                                                     inlet_nodes.begin() + _riser_nradial + 1);
  buildQuadBlock(*mesh,
                 riser_inlet_nodes,
                 _block_map["inlet"].second,
                 {_boundary_map["wall"].second, _boundary_map["wall"].second, ibid, ibid});
  // Next we'll do the radial reflector side, which has the wall on the bottom and top
  std::vector<std::vector<Node *>> radref_inlet_nodes(inlet_nodes.begin() + _riser_nradial,
                                                      inlet_nodes.end());
  std::array<std::vector<BoundaryID>, 4> radref_inlet_bids;
  radref_inlet_bids[1] = {_boundary_map["wall"].second};
  radref_inlet_bids[2] = {_boundary_map["inlet"].second, _boundary_map["reactor_outer"].second};
  radref_inlet_bids[3] = {_boundary_map["wall"].second};
  buildQuadBlock(*mesh, radref_inlet_nodes, _block_map["inlet"].second, radref_inlet_bids);

  // Build left part of radial reflector
  const Real radref_inner_radius = _core_radius + _cr_buffer_width + _cr_width;
  const Real left_radref_dr =
      (_riser_radius - radref_inner_radius) / (_radial_reflector_nradial / 2);
  std::vector<std::vector<Node *>> left_radref_nodes(_radial_reflector_nradial / 2 + 1);
  left_radref_nodes[0] = bot_cr_nodes.back();
  left_radref_nodes.back() = riser_nodes[0];
  left_radref_nodes.back().insert(
      left_radref_nodes.back().end(), inlet_nodes[0].begin() + 1, inlet_nodes[0].end());
  for (unsigned int i = 1; i < left_radref_nodes.size() - 1; ++i)
  {
    left_radref_nodes[i].resize(left_radref_nodes[0].size());
    const Real r = radref_inner_radius + i * left_radref_dr;
    for (unsigned int j = 0; j < left_radref_nodes[i].size(); ++j)
    {
      const Real left_z = (*left_radref_nodes[0][j])(1);
      const Real right_z = (*left_radref_nodes.back()[j])(1);
      const Real z = left_z + (r - radref_inner_radius) * (right_z - left_z) /
                                  (_riser_radius - radref_inner_radius);
      left_radref_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh, left_radref_nodes, _block_map["radial_reflector"].second);

  // Build control buffer in upper cavity
  std::vector<std::vector<Node *>> buffer_upper_cavity_nodes(
      _cr_buffer_nradial + 1, std::vector<Node *>(_upper_cavity_naxial + 1));
  buffer_upper_cavity_nodes[0] = upper_cavity_nodes.back();
  for (unsigned int i = 1; i < buffer_upper_cavity_nodes.size(); ++i)
  {
    buffer_upper_cavity_nodes[i].back() = bot_cr_buffer_nodes[i][0];
    const Real r = (*buffer_upper_cavity_nodes[i].back())(0);
    for (unsigned int j = 0; j < _upper_cavity_naxial; ++j)
    {
      const Real z = (*buffer_upper_cavity_nodes[0][j])(1);
      buffer_upper_cavity_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 buffer_upper_cavity_nodes,
                 _block_map["cr_buffer_upper_cavity"].second,
                 {ibid, _boundary_map["wall"].second, ibid, _boundary_map["wall"].second});

  // Build top of control rod buffer
  std::vector<std::vector<Node *>> top_cr_buffer_nodes(
      _cr_buffer_nradial + 1, std::vector<Node *>(_upper_reflector_naxial + 1));
  top_cr_buffer_nodes[0] = left_upper_reflector_nodes.back();
  for (unsigned int i = 1; i < top_cr_buffer_nodes.size(); ++i)
  {
    top_cr_buffer_nodes[i].back() = buffer_upper_cavity_nodes[i][0];
    const Real r = (*top_cr_buffer_nodes[i].back())(0);
    for (unsigned int j = 0; j < _upper_reflector_naxial; ++j)
    {
      const Real z = (*top_cr_buffer_nodes[0][j])(1);
      top_cr_buffer_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 top_cr_buffer_nodes,
                 _block_map["cr_buffer_upper_reflector"].second,
                 {ibid, ibid, ibid, _boundary_map["top"].second});

  // Build control rod channel in upper cavity
  std::vector<std::vector<Node *>> cr_upper_cavity_nodes(
      _cr_nradial + 1, std::vector<Node *>(_upper_cavity_naxial + 1));
  cr_upper_cavity_nodes[0] = buffer_upper_cavity_nodes.back();
  for (unsigned int i = 1; i < cr_upper_cavity_nodes.size(); ++i)
  {
    cr_upper_cavity_nodes[i].back() = bot_cr_nodes[i][0];
    const Real r = (*cr_upper_cavity_nodes[i].back())(0);
    for (unsigned int j = 0; j < _upper_cavity_naxial; ++j)
    {
      const Real z = (*cr_upper_cavity_nodes[0][j])(1);
      cr_upper_cavity_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 cr_upper_cavity_nodes,
                 _block_map["cr_channel_upper_cavity"].second,
                 {ibid, _boundary_map["wall"].second, ibid, _boundary_map["wall"].second});

  // Build top of control rod channel
  std::vector<std::vector<Node *>> top_cr_nodes(_cr_nradial + 1,
                                                std::vector<Node *>(_upper_reflector_naxial + 1));
  top_cr_nodes[0] = top_cr_buffer_nodes.back();
  for (unsigned int i = 1; i < top_cr_buffer_nodes.size(); ++i)
  {
    top_cr_nodes[i].back() = cr_upper_cavity_nodes[i][0];
    const Real r = (*top_cr_nodes[i].back())(0);
    for (unsigned int j = 0; j < _upper_reflector_naxial; ++j)
    {
      const Real z = (*top_cr_nodes[0][j])(1);
      top_cr_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 top_cr_nodes,
                 _block_map["cr_channel_upper_reflector"].second,
                 {ibid, ibid, ibid, _boundary_map["top"].second});

  // Build riser to upper cavity
  std::vector<std::vector<Node *>> riser_upper_cavity_nodes(
      _radial_reflector_nradial / 2 + _riser_nradial + 1,
      std::vector<Node *>(_upper_cavity_naxial + 1));
  riser_upper_cavity_nodes[0] = cr_upper_cavity_nodes.back();
  for (unsigned int i = 1; i < riser_upper_cavity_nodes.size(); ++i)
  {
    riser_upper_cavity_nodes[i].back() = i <= _radial_reflector_nradial / 2
                                             ? left_radref_nodes[i][0]
                                             : riser_nodes[i - _radial_reflector_nradial / 2][0];
    const Real r = (*riser_upper_cavity_nodes[i].back())(0);
    for (unsigned int j = 0; j < _upper_cavity_naxial; ++j)
    {
      const Real z = (*riser_upper_cavity_nodes[0][j])(1);
      riser_upper_cavity_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  // Need to separate this block since the wall boundary only covers part of the bottom
  // First will be the radial reflector part
  std::vector<std::vector<Node *>> radref_upcav_nodes(
      riser_upper_cavity_nodes.begin(), riser_upper_cavity_nodes.end() - _riser_nradial);
  buildQuadBlock(*mesh,
                 radref_upcav_nodes,
                 _block_map["riser_upper_cavity"].second,
                 {ibid, _boundary_map["wall"].second, ibid, _boundary_map["wall"].second});
  // Next will be the riser part
  std::vector<std::vector<Node *>> riser_upcav_nodes(riser_upper_cavity_nodes.begin() +
                                                         _radial_reflector_nradial / 2,
                                                     riser_upper_cavity_nodes.end());
  buildQuadBlock(*mesh,
                 riser_upcav_nodes,
                 _block_map["riser_upper_cavity"].second,
                 {ibid, ibid, _boundary_map["wall"].second, _boundary_map["wall"].second});

  // Build middle part of upper reflector
  std::vector<std::vector<Node *>> mid_upper_reflector_nodes(
      _radial_reflector_nradial / 2 + _riser_nradial + 1,
      std::vector<Node *>(_upper_reflector_naxial + 1));
  mid_upper_reflector_nodes[0] = top_cr_nodes.back();
  for (unsigned int i = 1; i < mid_upper_reflector_nodes.size(); ++i)
  {
    mid_upper_reflector_nodes[i].back() = riser_upper_cavity_nodes[i][0];
    const Real r = (*mid_upper_reflector_nodes[i].back())(0);
    for (unsigned int j = 0; j < mid_upper_reflector_nodes[i].size() - 1; ++j)
    {
      const Real z = (*mid_upper_reflector_nodes[0][j])(1);
      mid_upper_reflector_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 mid_upper_reflector_nodes,
                 _block_map["upper_reflector"].second,
                 {ibid, ibid, ibid, _boundary_map["top"].second});

  // Build right part of upper reflector
  std::vector<std::vector<Node *>> right_upper_reflector_nodes(_radial_reflector_nradial / 2 + 1);
  right_upper_reflector_nodes[0].insert(right_upper_reflector_nodes[0].end(),
                                        mid_upper_reflector_nodes.back().begin(),
                                        mid_upper_reflector_nodes.back().end());
  right_upper_reflector_nodes[0].insert(right_upper_reflector_nodes[0].end(),
                                        riser_upper_cavity_nodes.back().begin() + 1,
                                        riser_upper_cavity_nodes.back().end());
  for (unsigned int i = 1; i < right_upper_reflector_nodes.size(); ++i)
  {
    right_upper_reflector_nodes[i].resize(right_upper_reflector_nodes[0].size());
    right_upper_reflector_nodes[i].back() = right_radref_nodes[i][0];
    const Real r = (*right_upper_reflector_nodes[i].back())(0);
    for (unsigned int j = 0; j < right_upper_reflector_nodes[i].size() - 1; ++j)
    {
      const Real z = (*right_upper_reflector_nodes[0][j])(1);
      right_upper_reflector_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 right_upper_reflector_nodes,
                 _block_map["upper_reflector"].second,
                 {ibid, ibid, _boundary_map["reactor_outer"].second, _boundary_map["top"].second});

  // Build top part of lower reflector
  std::vector<std::vector<Node *>> top_lower_reflector_nodes;
  // Copy right nodes from conus channel
  top_lower_reflector_nodes.push_back(conus_channel_nodes.back());
  // Copy bottom nodes from control rod buffer
  for (unsigned int i = 1; i < bot_cr_buffer_nodes.size(); ++i)
    top_lower_reflector_nodes.push_back({bot_cr_buffer_nodes[i].back()});
  // Copy bottom nodes from control rod channel
  for (unsigned int i = 1; i < bot_cr_nodes.size(); ++i)
    top_lower_reflector_nodes.push_back({bot_cr_nodes[i].back()});
  // Copy bottom nodes from left radial reflector
  for (unsigned int i = 1; i < left_radref_nodes.size(); ++i)
    top_lower_reflector_nodes.push_back({left_radref_nodes[i].back()});
  // Copy bottom nodes from inlet
  for (unsigned int i = 1; i < inlet_nodes.size(); ++i)
    top_lower_reflector_nodes.push_back({inlet_nodes[i].back()});
  for (unsigned int i = 1; i < top_lower_reflector_nodes.size(); ++i)
  {
    top_lower_reflector_nodes[i].resize(top_lower_reflector_nodes[0].size());
    const Real r = (*top_lower_reflector_nodes[i][0])(0);
    for (unsigned int j = 1; j < top_lower_reflector_nodes[i].size(); ++j)
    {
      const Real z = (*top_lower_reflector_nodes[0][j])(1);
      top_lower_reflector_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 top_lower_reflector_nodes,
                 _block_map["lower_reflector"].second,
                 {ibid, ibid, _boundary_map["reactor_outer"].second, ibid});

  // Build hotleg
  std::vector<std::vector<Node *>> hotleg_nodes(top_lower_reflector_nodes.size(),
                                                plenum_nodes.back());
  for (unsigned int i = 1; i < hotleg_nodes.size(); ++i)
  {
    hotleg_nodes[i][0] = top_lower_reflector_nodes[i].back();
    const Real r = (*hotleg_nodes[i][0])(0);
    for (unsigned int j = 1; j < hotleg_nodes[i].size(); ++j)
    {
      const Real z = (*hotleg_nodes[0][j])(1);
      hotleg_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  std::array<std::vector<BoundaryID>, 4> hotleg_bids;
  hotleg_bids[1] = {_boundary_map["wall"].second};
  hotleg_bids[2] = {_boundary_map["outlet"].second, _boundary_map["reactor_outer"].second};
  hotleg_bids[3] = {_boundary_map["wall"].second};
  buildQuadBlock(*mesh, hotleg_nodes, _block_map["hotleg"].second, hotleg_bids);

  // Build right part of lower reflector
  std::vector<std::vector<Node *>> right_lower_reflector_nodes(hotleg_nodes.size(),
                                                               left_lower_reflector_nodes.back());
  for (unsigned int i = 1; i < right_lower_reflector_nodes.size(); ++i)
  {
    right_lower_reflector_nodes[i][0] = hotleg_nodes[i].back();
    const Real r = (*right_lower_reflector_nodes[i][0])(0);
    for (unsigned int j = 1; j < right_lower_reflector_nodes[i].size(); ++j)
    {
      const Real z = (*right_lower_reflector_nodes[0][j])(1);
      right_lower_reflector_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(
      *mesh,
      right_lower_reflector_nodes,
      _block_map["lower_reflector"].second,
      {ibid, _boundary_map["bottom"].second, _boundary_map["reactor_outer"].second, ibid});

  // Build barrel gap
  std::vector<std::vector<Node *>> barrel_gap_nodes(_barrel_gap_nradial + 1);
  // Copy nodes from right part of upper reflector
  barrel_gap_nodes[0].insert(barrel_gap_nodes[0].end(),
                             right_upper_reflector_nodes.back().begin(),
                             right_upper_reflector_nodes.back().end());
  // Copy nodes from right part of radial reflector
  barrel_gap_nodes[0].insert(barrel_gap_nodes[0].end(),
                             right_radref_nodes.back().begin() + 1,
                             right_radref_nodes.back().end());
  // Copy nodes from inlet
  barrel_gap_nodes[0].insert(
      barrel_gap_nodes[0].end(), inlet_nodes.back().begin() + 1, inlet_nodes.back().end());
  // Copy nodes from top part of lower reflector
  barrel_gap_nodes[0].insert(barrel_gap_nodes[0].end(),
                             top_lower_reflector_nodes.back().begin() + 1,
                             top_lower_reflector_nodes.back().end());
  // Copy nodes from hotleg
  barrel_gap_nodes[0].insert(
      barrel_gap_nodes[0].end(), hotleg_nodes.back().begin() + 1, hotleg_nodes.back().end());
  // Copy nodes from right part of lower reflector
  barrel_gap_nodes[0].insert(barrel_gap_nodes[0].end(),
                             right_lower_reflector_nodes.back().begin() + 1,
                             right_lower_reflector_nodes.back().end());
  for (unsigned int i = 1; i < barrel_gap_nodes.size(); ++i)
  {
    barrel_gap_nodes[i].resize(barrel_gap_nodes[0].size());
    const Real r = (*barrel_gap_nodes[0][0])(0) + i * _barrel_gap_width / _barrel_gap_nradial;
    for (unsigned int j = 0; j < barrel_gap_nodes[i].size(); ++j)
    {
      const Real z = (*barrel_gap_nodes[0][j])(1);
      barrel_gap_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh, barrel_gap_nodes, _block_map["barrel_gap"].second);

  // Build barrel
  std::vector<std::vector<Node *>> barrel_nodes(_barrel_nradial + 1, barrel_gap_nodes.back());
  for (unsigned int i = 1; i < barrel_nodes.size(); ++i)
  {
    const Real r = (*barrel_nodes[0][0])(0) + i * _barrel_width / _barrel_nradial;
    for (unsigned int j = 0; j < barrel_nodes[i].size(); ++j)
    {
      const Real z = (*barrel_nodes[0][j])(1);
      barrel_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh, barrel_nodes, _block_map["barrel"].second);

  // Build RPV gap
  std::vector<std::vector<Node *>> rpv_gap_nodes(_rpv_gap_nradial + 1, barrel_nodes.back());
  for (unsigned int i = 1; i < rpv_gap_nodes.size(); ++i)
  {
    const Real r = (*rpv_gap_nodes[0][0])(0) + i * _rpv_gap_width / _rpv_gap_nradial;
    for (unsigned int j = 0; j < rpv_gap_nodes[i].size(); ++j)
    {
      const Real z = (*rpv_gap_nodes[0][j])(1);
      rpv_gap_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh, rpv_gap_nodes, _block_map["rpv_gap"].second);

  // Build RPV
  std::vector<std::vector<Node *>> rpv_nodes(_rpv_nradial + 1, rpv_gap_nodes.back());
  for (unsigned int i = 1; i < rpv_nodes.size(); ++i)
  {
    const Real r = (*rpv_nodes[0][0])(0) + i * _rpv_width / _rpv_nradial;
    for (unsigned int j = 0; j < rpv_nodes[i].size(); ++j)
    {
      const Real z = (*rpv_nodes[0][j])(1);
      rpv_nodes[i][j] = mesh->add_point(Point(r, z, 0), node_id++);
    }
  }
  buildQuadBlock(*mesh,
                 rpv_nodes,
                 _block_map["rpv"].second,
                 {ibid, ibid, _boundary_map["outer"].second, ibid});

  // Set Subdomain names
  for (const auto & it : _block_map)
    mesh->subdomain_name(it.second.second) = it.second.first;
  // Set Boundary names
  auto & boundary_info = mesh->get_boundary_info();
  for (const auto & it : _boundary_map)
  {
    boundary_info.sideset_name(it.second.second) = it.second.first;
    boundary_info.nodeset_name(it.second.second) = it.second.first;
  }

  mesh->prepare_for_use();
  return dynamic_pointer_cast<MeshBase>(mesh);
}

void
PebbleBedRZGenerator::buildQuadBlock(MeshBase & mesh,
                                     const std::vector<std::vector<Node *>> & nodes,
                                     SubdomainID block_id,
                                     std::array<BoundaryID, 4> boundary_ids)
{
  std::array<std::vector<BoundaryID>, 4> vec_bids;
  for (unsigned int i = 0; i < 4; ++i)
    if (boundary_ids[i] != Moose::INVALID_BOUNDARY_ID)
      vec_bids[i].push_back(boundary_ids[i]);
  buildQuadBlock(mesh, nodes, block_id, vec_bids);
}

void
PebbleBedRZGenerator::buildQuadBlock(MeshBase & mesh,
                                     const std::vector<std::vector<Node *>> & nodes,
                                     SubdomainID block_id,
                                     const std::array<std::vector<BoundaryID>, 4> & boundary_ids)
{
  auto & boundary_info = mesh.get_boundary_info();
  for (unsigned int i = 0; i < nodes.size() - 1; ++i)
    for (unsigned int j = 0; j < nodes[i].size() - 1; ++j)
    {
      Elem * elem = mesh.add_elem(new Quad4);
      elem->set_node(0) = nodes[i][j];         // Top left
      elem->set_node(1) = nodes[i][j + 1];     // Bottom left
      elem->set_node(2) = nodes[i + 1][j + 1]; // Bottom right
      elem->set_node(3) = nodes[i + 1][j];     // Top right

      elem->subdomain_id() = block_id;

      // Left
      if (i == 0 && !boundary_ids[0].empty())
        boundary_info.add_side(elem, 0, boundary_ids[0]);
      // Bottom
      if (j == nodes[i].size() - 2 && !boundary_ids[1].empty())
        boundary_info.add_side(elem, 1, boundary_ids[1]);
      // Right
      if (i == nodes.size() - 2 && !boundary_ids[2].empty())
        boundary_info.add_side(elem, 2, boundary_ids[2]);
      // Top
      if (j == 0 && !boundary_ids[3].empty())
        boundary_info.add_side(elem, 3, boundary_ids[3]);
    }
}
