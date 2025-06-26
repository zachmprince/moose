//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ATRMeshGenerator.h"

#include "CastUniquePointer.h"
#include "FillBetweenPointVectorsTools.h"

#include "libmesh/boundary_info.h"
#include "libmesh/face_quad4.h"
#include "libmesh/mesh_modification.h"

registerMooseObject("ReactorApp", ATRMeshGenerator);

const Point ATRMeshGenerator::northwest_trap(0.0, 30.533);
const Point ATRMeshGenerator::north_trap(15.267, 15.267);
const Point ATRMeshGenerator::northeast_trap(30.533, 0.0);
const Point ATRMeshGenerator::east_trap(15.267, -15.267);
const Point ATRMeshGenerator::southeast_trap(0.0, -30.533);
const Point ATRMeshGenerator::south_trap(-15.267, -15.267);
const Point ATRMeshGenerator::southwest_trap(-30.533, 0.0);
const Point ATRMeshGenerator::west_trap(-15.267, 15.267);
const Point ATRMeshGenerator::center_trap(0.0, 0.0);

InputParameters
ATRMeshGenerator::validParams()
{
  InputParameters params = MeshGenerator::validParams();
  return params;
}

ATRMeshGenerator::ATRMeshGenerator(const InputParameters & parameters)
  : MeshGenerator(parameters), _base_fuel_element(_num_azimuthal_segments)
{
}

std::unique_ptr<MeshBase>
ATRMeshGenerator::generate()
{
  std::unique_ptr<ReplicatedMesh> mesh = buildReplicatedMesh(2);
  generateFuel(*mesh);
  generateFluxTraps(*mesh);
  return mesh;
}

void
ATRMeshGenerator::generateFuel(ReplicatedMesh & mesh)
{
  std::vector<Point> centers = {northwest_trap,
                                north_trap,
                                northeast_trap,
                                east_trap,
                                southeast_trap,
                                south_trap,
                                southwest_trap,
                                west_trap,
                                northwest_trap};
  _fuel_elements.resize(40);
  for (unsigned int c = 0, k = 0; c < centers.size(); ++c)
  {
    const auto & center = centers[c];
    unsigned int num = 0;
    if (c == 0 || c == centers.size() - 1)
      num = 3;
    else if (c % 2 == 0)
      num = 6;
    else
      num = 4;

    for (unsigned int e = 0; e < num; ++e, ++k)
    {
      if (k == 0)
        _fuel_elements[k] = std::make_unique<FuelElement>(_base_fuel_element, center);
      else
        _fuel_elements[k] =
            std::make_unique<FuelElement>(_base_fuel_element, center, _fuel_elements[k - 1].get());
    }
  }

  for (auto & element : _fuel_elements)
    element->generate(mesh);

  FuelElement::generateSidePlate(mesh,
                                 *_fuel_elements.back(),
                                 *_fuel_elements.front(),
                                 _base_fuel_element.numSidePlateSegments() * 2);

#ifndef NDEBUG
  for (auto & element : _fuel_elements)
    mooseAssert(element->numThetas() == _num_azimuthal_segments + 1,
                "Side plates were not generated properly.");
#endif
}

void
ATRMeshGenerator::generateFluxTraps(ReplicatedMesh & mesh)
{
  _flux_traps.resize(7);
  _flux_traps[0] = std::make_unique<FluxTrap1>(northwest_trap, _fuel_elements);
  _flux_traps[1] = std::make_unique<FluxTrap2>(north_trap, _fuel_elements);
  _flux_traps[2] = std::make_unique<FluxTrap2>(west_trap, _fuel_elements);
  _flux_traps[3] = std::make_unique<FluxTrap2>(southwest_trap, _fuel_elements);
  _flux_traps[4] = std::make_unique<FluxTrap2>(southeast_trap, _fuel_elements);
  _flux_traps[5] = std::make_unique<FluxTrap3>(northeast_trap, _fuel_elements);

  for (auto & flux_trap : _flux_traps)
    if (flux_trap)
      flux_trap->generate(mesh);
}

ATRMeshGenerator::BaseFuelElement::BaseFuelElement(unsigned int num_azi_segments)
{
  // Generate radial features -------------------------------------------------
  auto plate_thickness = [](unsigned int p)
  { return p == 0 ? 0.2032 : (p == num_plates - 1 ? 0.254 : 0.127); };
  // Fuel meat thickness: All of them are 0.02in
  constexpr Real fuel_thickness = 0.0508;
  // Gap thickness: 0.078in between each plate
  constexpr Real gap_thickness = 0.19812;
  // First fuel plate has 3.015in radius
  constexpr Real inner_radius = 7.6581;
  // Get the four radii for each plate: inner clad, inner fuel, outer fuel, outer clad
  for (const auto & p : make_range(num_plates))
  {
    _radii(p, 0) = p == 0 ? inner_radius : (_radii(p - 1, 3) + gap_thickness); // Inner clad
    _radii(p, 1) = _radii(p, 0) + (plate_thickness(p) - fuel_thickness) / 2;   // Inner fuel
    _radii(p, 2) = _radii(p, 1) + fuel_thickness;                              // Outer fuel
    _radii(p, 3) = _radii(p, 0) + plate_thickness(p);                          // Outer clad
  }

  // Generate azimuthal features ---------------------------------------------
  // Fuel plate arc lengths are explicitly given in the benchmark (in);
  // unfortunately, they do not create a consistent angle.
  constexpr std::array<Real, num_plates> fuel_arc_length = {3.937,
                                                            4.73202,
                                                            4.98602,
                                                            5.24256,
                                                            5.49656,
                                                            5.7531,
                                                            6.0071,
                                                            6.26364,
                                                            6.51764,
                                                            6.77418,
                                                            7.02818,
                                                            7.28472,
                                                            7.53872,
                                                            7.79526,
                                                            8.04926,
                                                            8.3058,
                                                            8.5598,
                                                            8.71474,
                                                            8.61314};
  // Side plate thickness: 0.187in
  constexpr Real side_plate_thickness = 0.47498;
  // Nominal delta theta
  const Real dtheta_nominal = libMesh::pi / 4 / num_azi_segments;
  // Number of segments for side plate
  _num_side_plate_segments = (unsigned int)(side_plate_thickness / inner_radius / dtheta_nominal);
  mooseAssert(_num_side_plate_segments > 0, "Side plate too thin.");
  // Gather azimuthal locations for each plate
  _thetas.resize(Eigen::NoChange, num_azi_segments - _num_side_plate_segments * 2 + 1);
  for (const auto & p : make_range(num_plates))
  {
    // Radius used to compute arc angles
    const Real fuel_center_radius = (_radii(p, 1) + _radii(p, 2)) / 2;

    // Number of fuel meat segments
    unsigned int & num_fuel_segments = _num_fuel_segments[p];
    num_fuel_segments = (unsigned int)(fuel_arc_length[p] / fuel_center_radius / dtheta_nominal);
    mooseAssert(num_fuel_segments > 0, "Fuel meat too narrow.");
    mooseAssert((num_fuel_segments + _num_side_plate_segments * 2) < num_azi_segments,
                "Side plate gap too narrow.");
    // Number of gap segments
    unsigned int num_gap_segments =
        num_azi_segments - num_fuel_segments - _num_side_plate_segments * 2;
    if (num_gap_segments % 2 > 0)
    {
      num_gap_segments++;
      num_fuel_segments--;
    }
    num_gap_segments /= 2;

    // Fuel angle
    const Real fuel_theta = fuel_arc_length[p] / fuel_center_radius;
    // Side plate angle
    const Real side_plate_theta = side_plate_thickness / fuel_center_radius;
    // Gap arc angle based on remaining theta
    const Real gap_theta = libMesh::pi / 8 - fuel_theta / 2 - side_plate_theta;

    // Generate theta locations for this plate
    for (const auto & j : make_range(_thetas.cols()))
    {
      if (j == 0)
        _thetas(p, j) = side_plate_theta;
      else if (j <= num_gap_segments || j > (num_gap_segments + num_fuel_segments))
        _thetas(p, j) = _thetas(p, j - 1) + gap_theta / num_gap_segments;
      else
        _thetas(p, j) = _thetas(p, j - 1) + fuel_theta / num_fuel_segments;
    }
  }
}

Point
ATRMeshGenerator::BaseFuelElement::getPoint(dof_id_type i,
                                            dof_id_type j,
                                            Point center,
                                            Real angle) const
{
  const auto p = i / 4;
  const auto radius = _radii(p, i % 4);
  const auto theta = _thetas(p, j);
  return Point(std::sin(theta + angle), std::cos(theta + angle)) * radius + center;
}

unsigned int
ATRMeshGenerator::BaseFuelElement::getSubdomain(dof_id_type i, dof_id_type j) const
{
  const auto ri = i % 4;
  if (ri == 3)
    return 2; // Water
  else if (ri != 1)
    return 1; // Clad

  const auto p = i / 4;
  const unsigned int num_gap_segments = (_thetas.cols() - _num_fuel_segments[p] - 1) / 2;
  if (j < num_gap_segments || j >= num_gap_segments + _num_fuel_segments[p])
    return 1; // Clad
  else
    return 0; // Fuel
}

ATRMeshGenerator::FuelElement::FuelElement(const BaseFuelElement & base,
                                           Point center,
                                           FuelElement * previous)
  : _base(base), _center(center), _previous(previous)
{
  if (_previous)
  {
    _angle = _previous->angle();
    if (_center.relative_fuzzy_equals(_previous->center()))
    {
      if (_previous->flipped())
        _angle -= libMesh::pi / 4;
      else
        _angle += libMesh::pi / 4;
      _flipped = previous->flipped();
    }
    else
    {
      _angle += libMesh::pi;
      _flipped = !previous->flipped();
    }
  }
}

dof_id_type
ATRMeshGenerator::FuelElement::numThetas() const
{
  return _base.numThetas() +
         (_left_side_plate_nodes.size() + _right_side_plate_nodes.size()) / _base.numRadii();
}

Node *
ATRMeshGenerator::FuelElement::getNode(dof_id_type i, dof_id_type j) const
{
  mooseAssert(!_nodes.empty(), "Nodes have not been constructed yet.");
  mooseAssert(i < numRadii(), "i too large");
  mooseAssert(j < numThetas(), "j too large");
  const auto k = i * numThetas() + j;
  mooseAssert((_nodes.size() + _left_side_plate_nodes.size() + _right_side_plate_nodes.size()) > k,
              "Nodes have not properly been constructed yet.");

  if (_left_side_plate_nodes.empty() && _right_side_plate_nodes.empty())
    return _nodes[k];

  const auto num_left_thetas = _left_side_plate_nodes.size() / numRadii();
  const auto num_right_thetas = _right_side_plate_nodes.size() / numRadii();
  if (j < num_left_thetas)
    return _left_side_plate_nodes[i * num_left_thetas + j];
  else if (j >= (num_left_thetas + _base.numThetas()))
    return _right_side_plate_nodes[i * num_right_thetas + j - num_left_thetas - _base.numThetas()];
  else
    return _nodes[i * _base.numThetas() + j - num_left_thetas];
}

std::vector<Node *>
ATRMeshGenerator::FuelElement::getSideNodes(unsigned int side) const
{
  if (_flipped)
    side += side >= 2 ? -2 : 2;
  mooseAssert(side < 4, "Unknown side.");
  const dof_id_type num_nodes = side % 2 == 0 ? numThetas() : numRadii();
  std::vector<Node *> nodes(num_nodes);
  for (const auto & k : make_range(num_nodes))
  {
    const auto ind = _flipped ? num_nodes - k - 1 : k;
    if (side == 0)
      nodes[ind] = getNode(0, k);
    else if (side == 1)
      nodes[ind] = getNode(k, numThetas() - 1);
    else if (side == 2)
      nodes[ind] = getNode(numRadii() - 1, k);
    else
      nodes[ind] = getNode(k, 0);
  }

  return nodes;
}

void
ATRMeshGenerator::FuelElement::generate(ReplicatedMesh & mesh)
{
  auto & boundary_info = mesh.get_boundary_info();

  _nodes.resize(_base.numPoints());
  for (dof_id_type i = 0, k = 0; i < _base.numRadii(); ++i)
    for (dof_id_type j = 0; j < _base.numThetas(); ++j, ++k)
      _nodes[k] = mesh.add_point(_base.getPoint(i, j, _center, _angle));

  const BoundaryID inner_id = _flipped ? 1 : 0;
  const BoundaryID outer_id = _flipped ? 0 : 1;
  for (dof_id_type i = 0; i < _base.numRadii() - 1; ++i)
    for (dof_id_type j = 0; j < _base.numThetas() - 1; ++j)
    {
      auto elem = mesh.add_elem(std::make_unique<Quad4>());
      FillBetweenPointVectorsTools::buildQuadElement(elem,
                                                     getNode(i, j),
                                                     getNode(i, j + 1),
                                                     getNode(i + 1, j + 1),
                                                     getNode(i + 1, j),
                                                     _base.getSubdomain(i, j));

      if (i == 0)
        boundary_info.add_side(elem, 0, inner_id);
      if (i == _base.numRadii() - 2)
        boundary_info.add_side(elem, 2, outer_id);
    }

  if (_previous)
    generateSidePlate(mesh, *_previous, *this, _base.numSidePlateSegments() * 2);
}

void
ATRMeshGenerator::FuelElement::generateSidePlate(ReplicatedMesh & mesh,
                                                 FuelElement & left,
                                                 FuelElement & right,
                                                 unsigned int num_segments)
{
  const auto left_nodes = left.getSideNodes(1);
  const auto right_nodes = right.getSideNodes(3);
  mooseAssert(!left_nodes.empty(), "Can't find side nodes.");
  mooseAssert(left_nodes.size() == right_nodes.size(), "Number of side nodes don't match.");

  std::vector<Node *> new_nodes(left_nodes.size() * (num_segments - 1));
  for (dof_id_type i = 0, k = 0; i < left_nodes.size(); ++i)
    for (dof_id_type j = 1; j < num_segments; ++j, ++k)
    {
      const Real frac = (Real)j / (Real)num_segments;
      const Point pt = *left_nodes[i] * (1.0 - frac) + *right_nodes[i] * frac;
      new_nodes[k] = mesh.add_point(pt);
    }

  auto & boundary_info = mesh.get_boundary_info();
  auto get_node = [&](dof_id_type i, dof_id_type j)
  {
    if (j == 0)
      return left_nodes[i];
    else if (j == num_segments)
      return right_nodes[i];
    else
      return new_nodes[i * (num_segments - 1) + (j - 1)];
  };
  for (const auto i : make_range(left_nodes.size() - 1))
    for (const auto j : make_range(num_segments))
    {
      auto elem = mesh.add_elem(std::make_unique<Quad4>());
      FillBetweenPointVectorsTools::buildQuadElement(
          elem, get_node(i, j), get_node(i, j + 1), get_node(i + 1, j + 1), get_node(i + 1, j), 1);

      if (i == 0)
        boundary_info.add_side(elem, 0, 0);
      if (i == (left_nodes.size() - 2))
        boundary_info.add_side(elem, 2, 1);
    }

  left.addSidePlateNodes(new_nodes, /*is_right=*/true);
  right.addSidePlateNodes(new_nodes, /*is_right=*/false);
}

void
ATRMeshGenerator::FuelElement::addSidePlateNodes(const std::vector<Node *> & new_nodes,
                                                 bool is_right)
{
  is_right = is_right != _flipped;
  std::vector<Node *> & side_plate_nodes =
      is_right ? _right_side_plate_nodes : _left_side_plate_nodes;

  const unsigned int num_side_plate_segments = (new_nodes.size() / _base.numRadii() + 1) / 2;
  side_plate_nodes.resize(_base.numRadii() * num_side_plate_segments);

  for (std::size_t i = 0, k = 0; i < _base.numRadii(); ++i)
    for (std::size_t j = 0; j < num_side_plate_segments; ++j, ++k)
    {
      const auto col = is_right ? j : j + num_side_plate_segments - 1;
      auto knew = i * (num_side_plate_segments - 1) * 2 + col;
      if (_flipped)
        knew = (new_nodes.size() - 1) - knew;
      side_plate_nodes[k] = new_nodes[knew];
    }
}

ATRMeshGenerator::FluxTrap::FluxTrap(
    Point center, const std::vector<std::unique_ptr<FuelElement>> & fuel_elements)
  : _center(center)
{
  using namespace ATRMeshing;
  // Gather fuel elements with matching centers
  std::vector<std::size_t> local_fuel_elements;
  std::size_t num_fuel_element_nodes = 1;
  for (const auto i : index_range(fuel_elements))
    if (_center.relative_fuzzy_equals(fuel_elements[i]->center()))
    {
      local_fuel_elements.push_back(i);
      num_fuel_element_nodes += fuel_elements[i]->numThetas() - 1;
    }
  if (local_fuel_elements[0] == 0)
  {
    mooseAssert(local_fuel_elements.size() == 6, "Unexpected number of fuel elements.");
    const auto half = local_fuel_elements.size() / 2;
    for (const auto e : make_range(half))
      std::swap(local_fuel_elements[e], local_fuel_elements[e + half]);
  }
  if (fuel_elements[local_fuel_elements[0]]->flipped())
    std::reverse(local_fuel_elements.begin(), local_fuel_elements.end());

  // Gather the nodes from the elements
  _fuel_element_nodes.resize(num_fuel_element_nodes);
  for (std::size_t e = 0, k = 0; e < local_fuel_elements.size(); ++e)
  {
    const auto & element = *fuel_elements[local_fuel_elements[e]];
    const unsigned int side = element.flipped() ? 2 : 0;
    auto side_elements = element.getSideNodes(side);
    if (element.flipped())
      std::reverse(side_elements.begin(), side_elements.end());
    for (std::size_t j = 0; j < side_elements.size() - 1; ++j, ++k)
      _fuel_element_nodes[k] = side_elements[j];
    if (e == local_fuel_elements.size() - 1)
      _fuel_element_nodes[k++] = side_elements.back();
  }

  // Get radius from BaseFuelElement
  {
    BaseFuelElement base_fuel_element(40);
    const Point inner_point = base_fuel_element.getPoint(0, 0, Point(), 0.0);
    _radius = inner_point.norm();
  }

  // Figure out how many point to add around rim
  const Real front_angle = computeAngle(*_fuel_element_nodes.front(), center);
  const Real back_angle = computeAngle(*_fuel_element_nodes.back(), center);
  const Real fuel_dtheta = front_angle > back_angle ? front_angle - back_angle
                                                    : 2 * libMesh::pi - (back_angle - front_angle);
  mooseAssert(fuel_dtheta > 0, "Something went wrong sorting fuel element nodes.");
  dof_id_type total_num_segments =
      (dof_id_type)(2 * libMesh::pi / fuel_dtheta * (num_fuel_element_nodes - 1));
  if (total_num_segments % 8 > 0)
    total_num_segments += 8 - total_num_segments % 8;
  const dof_id_type num_nonfuel_segments = total_num_segments - (num_fuel_element_nodes - 1);

  // Add angles
  _nonfuel_rim_angles.resize(num_nonfuel_segments - 1);
  Real theta = back_angle;
  const Real dtheta = (2 * libMesh::pi - fuel_dtheta) / num_nonfuel_segments;
  for (auto & angle : _nonfuel_rim_angles)
  {
    theta -= dtheta;
    if (theta < 0)
      theta += 2 * libMesh::pi;
    angle = theta;
  }
}

std::vector<Node *>
ATRMeshGenerator::FluxTrap::getRimNodes(ReplicatedMesh & mesh)
{
  using namespace ATRMeshing;
  std::vector<Node *> rim_nodes(numRimNodes());
  for (const auto j : index_range(_fuel_element_nodes))
    rim_nodes[j] = _fuel_element_nodes[j];

  if (_nonfuel_rim_nodes.size() < _nonfuel_rim_angles.size())
  {
    _nonfuel_rim_nodes.resize(_nonfuel_rim_angles.size());
    for (const auto jj : index_range(_nonfuel_rim_angles))
      _nonfuel_rim_nodes[jj] =
          mesh.add_point(computePoint(_nonfuel_rim_angles[jj], _radius, _center));
  }

  for (const auto jj : index_range(_nonfuel_rim_nodes))
    rim_nodes[jj + _fuel_element_nodes.size()] = _nonfuel_rim_nodes[jj];

  return rim_nodes;
}

Real
ATRMeshGenerator::FluxTrap::getAngle(dof_id_type j) const
{
  using namespace ATRMeshing;
  mooseAssert(j < numRimNodes(), "Index to large.");

  const Real dtheta = 2 * libMesh::pi / numRimNodes();

  Real angle = computeAngle(*_fuel_element_nodes.front(), _center) - j * dtheta;
  if (angle < 0)
    angle += 2 * libMesh::pi;
  return angle;
}

void
ATRMeshGenerator::FluxTrap1::generate(ReplicatedMesh & mesh)
{
  using namespace ATRMeshing;
  const auto layer_radii = layerRadii();
  mooseAssert(std::is_sorted(layer_radii.rbegin(), layer_radii.rend()),
              "Layer radii are not sorted properly.");
  std::vector<std::vector<Node *>> outer_nodes(layer_radii.size() + 1,
                                               std::vector<Node *>(numRimNodes()));
  outer_nodes[0] = getRimNodes(mesh);
  for (const auto i : make_range(layer_radii.size()))
    for (const auto j : make_range(numRimNodes()))
      outer_nodes[i + 1][j] = mesh.add_point(computePoint(getAngle(j), layer_radii[i], _center));

  for (const auto i : make_range(layer_radii.size()))
    for (const auto j : make_range(numRimNodes()))
    {
      auto elem = mesh.add_elem(std::make_unique<Quad4>());
      const auto jp1 = j < numRimNodes() - 1 ? j + 1 : 0;
      FillBetweenPointVectorsTools::buildQuadElement(elem,
                                                     outer_nodes[i + 1][j],
                                                     outer_nodes[i + 1][jp1],
                                                     outer_nodes[i][jp1],
                                                     outer_nodes[i][j],
                                                     3);
    }

  const Real avg_layer_thickness = (layer_radii.front() - layer_radii.back()) / layer_radii.size();
  const unsigned int num_layers = std::ceil(layer_radii.back() / 2 / avg_layer_thickness);
  meshDiskWithQuad(mesh, outer_nodes.back(), num_layers, layer_radii.back(), _center, 4);
}

void
ATRMeshGenerator::FluxTrap3::generate(ReplicatedMesh & mesh)
{
  using namespace ATRMeshing;
  constexpr std::array<Real, 4> layer_radii = {7.46125, 6.82625, 6.511925, 6.181725};
  mooseAssert(std::is_sorted(layer_radii.rbegin(), layer_radii.rend()),
              "Layer radii are not sorted properly.");
  std::vector<std::vector<Node *>> outer_nodes(layer_radii.size() + 1,
                                               std::vector<Node *>(numRimNodes()));
  outer_nodes[0] = getRimNodes(mesh);
  for (const auto i : make_range(layer_radii.size()))
    for (const auto j : make_range(numRimNodes()))
      outer_nodes[i + 1][j] = mesh.add_point(computePoint(getAngle(j), layer_radii[i], _center));

  for (const auto i : make_range(layer_radii.size()))
    for (const auto j : make_range(numRimNodes()))
    {
      auto elem = mesh.add_elem(std::make_unique<Quad4>());
      const auto jp1 = j < numRimNodes() - 1 ? j + 1 : 0;
      FillBetweenPointVectorsTools::buildQuadElement(elem,
                                                     outer_nodes[i + 1][j],
                                                     outer_nodes[i + 1][jp1],
                                                     outer_nodes[i][jp1],
                                                     outer_nodes[i][j],
                                                     3);
    }
}

namespace ATRMeshing
{
Real
computeAngle(const Point & pt, const Point & center)
{
  const Point rel_pt = pt - center;
  Real theta = std::atan2(rel_pt(1), rel_pt(0));
  if (theta < 0)
    theta += 2 * libMesh::pi;
  return theta;
}

Point
computePoint(Real angle, Real radius, const Point & center)
{
  return Point(std::cos(angle), std::sin(angle)) * radius + center;
}

void
meshDiskWithQuad(ReplicatedMesh & mesh,
                 std::vector<Node *> radial_nodes,
                 unsigned int num_layers,
                 Real radius,
                 Point center,
                 SubdomainID subdomain_id)
{
  mooseAssert(radial_nodes.size() % 4 == 0, "Number of nodes must be divisible by 4.");
  const auto nseg = radial_nodes.size() / 4;
  const Real theta0 = computeAngle(*radial_nodes.front(), center) * 180 / libMesh::pi;

  std::vector<std::vector<Node *>> square_nodes(nseg + 1, std::vector<Node *>(nseg + 1));
  const Real side_length = radius;
  const auto rmat = RealTensorValue::intrinsic_rotation_matrix(theta0 + 135, 0.0, 0.0);
  for (const auto i : make_range(nseg + 1))
    for (const auto j : make_range(nseg + 1))
    {
      const Real u = -1.0 + 2.0 * Real(j) / nseg;
      const Real v = -1.0 + 2.0 * Real(i) / nseg;
      Point pt(u, v);
      pt *= side_length / 2;
      pt = rmat * pt + center;
      square_nodes[i][j] = mesh.add_point(pt);
    }

  for (const auto i : make_range(nseg))
    for (const auto j : make_range(nseg))
    {
      auto elem = mesh.add_elem(std::make_unique<Quad4>());
      FillBetweenPointVectorsTools::buildQuadElement(elem,
                                                     square_nodes[i][j],
                                                     square_nodes[i][j + 1],
                                                     square_nodes[i + 1][j + 1],
                                                     square_nodes[i + 1][j],
                                                     subdomain_id);
    }

  std::vector<std::vector<Node *>> ring_nodes(num_layers + 1,
                                              std::vector<Node *>(radial_nodes.size()));
  for (const auto i : make_range(num_layers + 1))
  {
    const Real frac = Real(i) / num_layers;
    for (const unsigned int q : make_range(4))
    {
      for (const auto jj : make_range(nseg))
      {
        const auto j = q * nseg + jj;

        Node * rad_node = radial_nodes[j];
        Node * sq_node;
        if (q == 0)
          sq_node = square_nodes[jj].front();
        else if (q == 1)
          sq_node = square_nodes.back()[jj];
        else if (q == 2)
          sq_node = square_nodes[nseg - jj].back();
        else
          sq_node = square_nodes.front()[nseg - jj];

        if (i == 0)
          ring_nodes[i][j] = sq_node;
        else if (i == num_layers)
          ring_nodes[i][j] = rad_node;
        else
          ring_nodes[i][j] = mesh.add_point(*sq_node * (1 - frac) + *rad_node * frac);
      }
    }
  }

  for (const auto i : make_range(num_layers))
    for (const auto j : make_range(radial_nodes.size()))
    {
      auto elem = mesh.add_elem(std::make_unique<Quad4>());
      const auto jp1 = j < radial_nodes.size() - 1 ? j + 1 : 0;
      FillBetweenPointVectorsTools::buildQuadElement(elem,
                                                     ring_nodes[i][j],
                                                     ring_nodes[i][jp1],
                                                     ring_nodes[i + 1][jp1],
                                                     ring_nodes[i + 1][j],
                                                     subdomain_id);
    }
}
}
