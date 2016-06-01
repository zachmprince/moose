/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

#ifndef DT2mod_H_
#define DT2mod_H_

// MOOSE includes
#include "DT2.h"

// Forward declarations
class DT2mod;

namespace libMesh
{
template <typename T> class NumericVector;
}


template<>
InputParameters validParams<DT2mod>();

/**
 * An adaptive timestepper that compares the solution obtained from a
 * single step of size dt with two steps of size dt/2 and adjusts the
 * next timestep accordingly.
 */
class DT2mod : public DT2
{
public:
  DT2mod(const InputParameters & parameters);

  virtual void step();
  virtual void postStep();
  virtual unsigned int getDT2Step(){return _DT2_step;}

protected:
  virtual Real computeFailedDT();
  virtual Real computeDT();

  unsigned int _DT2_step;

  Real _e_max_orig;
  Real _time_old_orig;
  bool _at_sync_time;
};


#endif /* DT2mod_H_ */
