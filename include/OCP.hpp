/*! \file    OCP.hpp
 *  \brief   Main header file for OpenCAEPoro simulator
 *  \author  Shizhe Li
 *  \date    Oct/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#ifndef __OCP_HEADER__
#define __OCP_HEADER__

// OpenCAEPoro header files
#include "OCPControl.hpp"
#include "OCPMethod.hpp"
#include "OCPOutput.hpp"
#include "ParamRead.hpp"
#include "Reservoir.hpp"
#include "UtilTiming.hpp"

#define OCPVersion "0.1.0"

/// Top level data structure in the OpenCAEPoro simulator
class OpenCAEPoro
{
public:
    /// Output OpenCAEPoro version information.
    void PrintVersion()
    {
        std::cout << "-------------------------\n"
                  << "|   OpenCAEPoro " << OCPVersion << "   |\n"
                  << "-------------------------\n"
                  << std::endl;
    };

    /// Read input parameters to an internal structure.
    void InputParam(ParamRead& param);

    /// Setup reservoir based on an internal structure.
    void SetupReservoir(ParamRead& param);

    /// Setup linear solution method.
    void SetupSolver();

    /// Initialize or get intitial status of reserovir.
    void InitReservoir();

    /// Run dynamic simulation.
    void RunSimulation();

    /// Output necessary information for post-processing.
    void OutputResults();

private:
    /// The core properties of a reservoir.
    Reservoir reservoir;

    /// The IMplicit Pressure Explicit Saturation method class.
    OCP_IMPES impes;

    /// The Fully Implicit method class.
    OCP_FIM fim;

    /// Control class manages the params of method and time step.
    OCP_Control control;

    /// Output class outputs the results you are interested in.
    OCP_Output output;
};

#endif /* end if __OCP_HEADER__ */

/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Oct/01/2021      Create file                          */
/*  Chensong Zhang      Oct/15/2021      Format file                          */
/*----------------------------------------------------------------------------*/