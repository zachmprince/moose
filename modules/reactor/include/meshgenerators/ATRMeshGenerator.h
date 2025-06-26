//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "MeshGenerator.h"

class ATRMeshGenerator : public MeshGenerator
{
public:
  static InputParameters validParams();

  ATRMeshGenerator(const InputParameters & parameters);

  virtual std::unique_ptr<MeshBase> generate() override;

protected:
  class BaseFuelElement
  {
  public:
    BaseFuelElement(unsigned int num_azi_segments);

    dof_id_type numRadii() const { return _radii.size(); }
    dof_id_type numThetas() const { return _thetas.cols(); }
    dof_id_type numPoints() const { return numRadii() * numThetas(); }
    unsigned int numSidePlateSegments() const { return _num_side_plate_segments; }

    Point getPoint(dof_id_type i, dof_id_type j, Point center, Real angle) const;
    Point getPoint(dof_id_type k, Point center, Real angle) const
    {
      return getPoint(k / numThetas(), k % numThetas(), center, angle);
    }

    /// 0: fuel, 1: clad, 2: water
    unsigned int getSubdomain(dof_id_type i, dof_id_type j) const;

  private:
    /// Number of fuel plates
    static constexpr unsigned int num_plates = 19;

    /// Number of side plate segments
    unsigned int _num_side_plate_segments;
    /// Number of fuel segments for each plate
    std::array<unsigned int, num_plates> _num_fuel_segments;

    /// Radii of points (4 for each plate)
    Eigen::Matrix<Real, num_plates, 4> _radii;
    /// Azimuthal locations
    Eigen::Matrix<Real, num_plates, Eigen::Dynamic> _thetas;
  };

  class FuelElement
  {
  public:
    FuelElement(const BaseFuelElement & base, Point center, FuelElement * previous = nullptr);

    Point center() const { return _center; }
    Real angle() const { return _angle; }
    bool flipped() const { return _flipped; }

    dof_id_type numRadii() const { return _base.numRadii(); }
    dof_id_type numThetas() const;

    Node * getNode(dof_id_type i, dof_id_type j) const;
    /// side = 0: inner, 1: right, 2: outer, 3: left (considers flipped)
    std::vector<Node *> getSideNodes(unsigned int side) const;

    void generate(ReplicatedMesh & mesh);
    static void generateSidePlate(ReplicatedMesh & mesh,
                                  FuelElement & left,
                                  FuelElement & right,
                                  unsigned int num_segments);
    void addSidePlateNodes(const std::vector<Node *> & new_nodes, bool is_right);

  private:
    const BaseFuelElement & _base;
    const Point _center;
    FuelElement * _previous;

    Real _angle = 0.0;
    Real _flipped = false;

    std::vector<Node *> _nodes;
    std::vector<Node *> _left_side_plate_nodes;
    std::vector<Node *> _right_side_plate_nodes;
  };

  class FluxTrap
  {
  public:
    FluxTrap(Point center, const std::vector<std::unique_ptr<FuelElement>> & fuel_elements);

    virtual void generate(ReplicatedMesh & mesh) = 0;

    dof_id_type numRimNodes() const
    {
      return _fuel_element_nodes.size() + _nonfuel_rim_angles.size();
    }

  protected:
    std::vector<Node *> getRimNodes(ReplicatedMesh & mesh);
    Real getAngle(dof_id_type j) const;

    const Point _center;
    std::vector<Node *> _fuel_element_nodes;
    Real _radius;

  private:
    std::vector<Real> _nonfuel_rim_angles;
    std::vector<Node *> _nonfuel_rim_nodes;
  };

  /// Northwest trap
  class FluxTrap1 : public FluxTrap
  {
  public:
    FluxTrap1(Point center, const std::vector<std::unique_ptr<FuelElement>> & fuel_elements)
      : FluxTrap(center, fuel_elements)
    {
    }

    virtual void generate(ReplicatedMesh & mesh) override;

  private:
    static constexpr std::array<Real, 7> layer_radii = {
        7.46125, 6.82625, 6.26364, 5.84454, 5.74294, 5.08, 4.49834};
  };

  void generateFuel(ReplicatedMesh & mesh);
  void generateFluxTraps(ReplicatedMesh & mesh);

  static const Point northwest_trap;
  static const Point north_trap;
  static const Point northeast_trap;
  static const Point east_trap;
  static const Point southeast_trap;
  static const Point south_trap;
  static const Point southwest_trap;
  static const Point west_trap;
  static const Point center_trap;

private:
  const unsigned int _num_azimuthal_segments = 40;
  BaseFuelElement _base_fuel_element;
  std::vector<std::unique_ptr<FuelElement>> _fuel_elements;
  std::vector<std::unique_ptr<FluxTrap>> _flux_traps;
};

namespace ATRMeshing
{
Real computeAngle(const Point & pt, const Point & center);

Point computePoint(Real angle, Real radius, const Point & center);

void meshDiskWithQuad(ReplicatedMesh & mesh,
                      std::vector<Node *> radial_nodes,
                      unsigned int num_layers,
                      Real radius,
                      Point center,
                      SubdomainID subdomain_id);
}
