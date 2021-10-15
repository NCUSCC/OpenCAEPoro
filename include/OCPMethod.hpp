/*! \file    OCPMethod.hpp
 *  \brief   Solution methods in OpenCAEPoro
 *  \author  Shizhe Li
 *  \date    Oct/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#ifndef __OCP_METHOD_HEADER__
#define __OCP_METHOD_HEADER__

// OpenCAEPoro header files
#include "OCPControl.hpp"
#include "OCPOutput.hpp"
#include "Reservoir.hpp"
#include "LinearSolver.hpp"
#include "UtilTiming.hpp"

/// OCP_IMPES is IMPES(implict pressure explict saturation) method in our
/// simulator(OpenCAEPoro), which consists of the functions in Reservoir.
class OCP_IMPES
{
public:
    void SetupParam(const string& dir, const string& file);
    void AllocateMat(const Reservoir& rs);
    void Run(Reservoir& rs, OCP_Control& ctrl, OCP_Output& output);
    void GoOneStep(Reservoir& rs, OCP_Control& ctrl);
    void SolveP(Reservoir& rs, OCP_Control& ctrl, const OCP_DBL& dt);

private:
    Solver<OCP_DBL> solver;
};

/// (to do) OCP_FIM is FIM(fully implict method) method in our simulator(OpenCAEPoro),
/// which consists of the functions in Reservoir.
class OCP_FIM
{
public:
private:
    Solver<vector<OCP_DBL>> solver;
};

#endif /* end if __OCP_METHOD_HEADER__ */

/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Oct/01/2021      Create file                          */
/*  Chensong Zhang      Oct/15/2021      Format file                          */
/*----------------------------------------------------------------------------*/