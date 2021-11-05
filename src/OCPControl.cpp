/*! \file    OCPControl.cpp
 *  \brief   OCPControl class definition
 *  \author  Shizhe Li
 *  \date    Oct/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#include "OCPControl.hpp"

ControlTime::ControlTime(const vector<OCP_DBL>& src)
{
    timeInit       = src[0];
    timeMax        = src[1];
    timeMin        = src[2];
    maxIncreFac    = src[3];
    minChopFac     = src[4];
    cutFacNR       = src[5];
}

ControlPreTime::ControlPreTime(const vector<OCP_DBL>& src)
{
    dPlim = src[0];
    dSlim = src[1];
    dNlim = src[2];
    dVlim = src[3];
}

ControlNR::ControlNR(const vector<OCP_DBL>& src)
{
    maxNRiter   = src[0];
    NRtol       = src[1];
    NRdPmax     = src[2];
    NRdSmax     = src[3];
    NRdPmin     = src[4];
    NRdSmin     = src[5];
    Verrmax     = src[6];
}

void OCPControl::InputParam(const ParamControl& CtrlParam)
{
    workDir = CtrlParam.dir;
    if (CtrlParam.method == "IMPEC") {
        method = IMPEC;
    } else if (CtrlParam.method == "FIM") {
        method = FIM;
    } else {
        OCP_ABORT("Wrong method specified!");
    }
    lsFile = CtrlParam.linearSolve;
    criticalTime = CtrlParam.criticalTime;

    USI t = CtrlParam.criticalTime.size();
    ctrlTimeSet.resize(t);
    ctrlPreTimeSet.resize(t);
    ctrlNRSet.resize(t);

    USI         n = CtrlParam.tuning_T.size();
    vector<USI> ctrlCriticalTime(n + 1);
    for (USI i = 0; i < n; i++) {
        ctrlCriticalTime[i] = CtrlParam.tuning_T[i].d;
    }
    ctrlCriticalTime.back() = t;
    for (USI i = 0; i < n; i++) {
        for (USI d = ctrlCriticalTime[i]; d < ctrlCriticalTime[i + 1]; d++) {
            ctrlTimeSet[d]  = ControlTime(CtrlParam.tuning_T[i].Tuning[0]);
            ctrlPreTimeSet[d] = ControlPreTime(CtrlParam.tuning_T[i].Tuning[1]);
            ctrlNRSet[d]  = ControlNR(CtrlParam.tuning_T[i].Tuning[2]);
        }
    }

    cout << "OCPControl::input" << endl;
}

void OCPControl::ApplyControl(const USI& i)
{
    ctrlTime  = ctrlTimeSet[i];
    ctrlPreTime = ctrlPreTimeSet[i];
    ctrlNR  = ctrlNRSet[i];
    end_time  = criticalTime[i + 1];
}

void OCPControl::InitTime(const USI& i)
{
    OCP_DBL dt = criticalTime[i + 1] - current_time;
    if (dt < 0) OCP_ABORT("Negative time stepsize!");
    current_dt = min(dt, ctrlTime.timeInit);
}

void OCPControl::CalNextTstepIMPEC(const Reservoir& reservoir)
{
    last_dt = current_dt;
    current_time += current_dt;

    OCP_DBL c1, c2, c3, c4, c;
    c1 = c2 = c3 = c4 = 10;

    OCP_DBL dPmax = reservoir.bulk.GetdPmax();
    OCP_DBL dNmax = reservoir.bulk.GetdNmax();
    OCP_DBL dSmax = reservoir.bulk.GetdSmax();
    OCP_DBL dVmax = reservoir.bulk.GetdVmax();

    if (dPmax > TINY)   c1 = ctrlPreTime.dPlim / dPmax;
    if (dSmax > TINY)   c2 = ctrlPreTime.dSlim / dSmax;
    if (dNmax > TINY)   c3 = ctrlPreTime.dNlim / dNmax;
    if (dVmax > TINY)   c4 = ctrlPreTime.dVlim / dVmax;

    c = min(min(c1, c2), min(c3, c4));
    c = max(ctrlTime.minChopFac, c);
    c = min(ctrlTime.maxIncreFac, c);

    current_dt *= c;

    if (current_dt > ctrlTime.timeMax) current_dt = ctrlTime.timeMax;
    if (current_dt < ctrlTime.timeMin) current_dt = ctrlTime.timeMin;

    OCP_DBL dt = end_time - current_time;
    if (current_dt > dt) current_dt = dt;
}

void OCPControl::CalNextTstepFIM(const Reservoir& reservoir)
{
    last_dt = current_dt;
    current_time += current_dt;

    OCP_DBL c1, c2, c;
    c1 = c2 = 10;

    OCP_DBL dPmax = reservoir.bulk.GetdPmax();
    OCP_DBL dSmax = reservoir.bulk.GetdSmax();

    if (dPmax > TINY)   c1 = ctrlPreTime.dPlim / dPmax;
    if (dSmax > TINY)   c2 = ctrlPreTime.dSlim / dSmax;

    OCP_DBL c3 = 1.5;

    if (iterNR < 3) {
        c3 = 2;
    }
    else if (iterNR > 8) {
        c3 = 0.5;
    }

    c = min(min(c1, c2), c3);
    c = max(ctrlTime.minChopFac, c);
    c = min(ctrlTime.maxIncreFac, c);

    current_dt *= c;
    if (current_dt > ctrlTime.timeMax) current_dt = ctrlTime.timeMax;
    if (current_dt < ctrlTime.timeMin) current_dt = ctrlTime.timeMin;

    OCP_DBL dt = end_time - current_time;
    if (current_dt > dt) current_dt = dt;
}



void OCPControl::UpdateIters()
{
    numTstep += 1;
    iterNR_total += iterNR;
}

/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Oct/01/2021      Create file                          */
/*  Chensong Zhang      Oct/15/2021      Format file                          */
/*----------------------------------------------------------------------------*/