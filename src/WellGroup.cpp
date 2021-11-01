/*! \file    WellGroup.cpp
 *  \brief   WellGroup class definition
 *  \author  Shizhe Li
 *  \date    Oct/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#include "WellGroup.hpp"

void WellGroup::InputParam(const ParamWell& paramWell)
{
    numWell = paramWell.well.size();
    wellGroup.resize(numWell);
    USI         t = paramWell.criticalTime.size();
    vector<USI> wellCriticalTime;
    for (USI w = 0; w < numWell; w++) {
        wellGroup[w].name  = paramWell.well[w].name;
        wellGroup[w].depth = paramWell.well[w].depth;
        wellGroup[w].I     = paramWell.well[w].I - 1;
        wellGroup[w].J     = paramWell.well[w].J - 1;

        wellGroup[w].InputPerfo(paramWell.well[w]);

        // opt
        wellGroup[w].optSet.resize(t);
        USI n = paramWell.well[w].optParam.size();
        wellCriticalTime.clear();
        wellCriticalTime.resize(n + 1);
        for (USI i = 0; i < n; i++) {
            wellCriticalTime[i] = paramWell.well[w].optParam[i].d;
        }
        wellCriticalTime.back() = t;
        for (USI i = 0; i < n; i++) {
            for (USI d = wellCriticalTime[i]; d < wellCriticalTime[i + 1]; d++) {
                wellGroup[w].optSet[d] = WellOpt(paramWell.well[w].optParam[i].opt);
            }
        }
    }

    cout << "WellGroup::InputParam" << endl;
}

void WellGroup::Setup(const Grid& myGrid, const Bulk& myBulk)
{
    SetupWell(myGrid, myBulk);
    SetupMixture(myBulk);
}

void WellGroup::SetupWell(const Grid& myGrid, const Bulk& myBulk)
{
    for (USI w = 0; w < numWell; w++) {
        wellGroup[w].Setup(myGrid, myBulk);
    }
}

void WellGroup::SetupMixture(const Bulk& myBulk)
{

    flashCal = myBulk.GetMixture();
    cout << "WellGroup::setupMixture" << endl;
}

void WellGroup::Init(const Bulk& myBulk)
{
    for (USI w = 0; w < numWell; w++) {
        wellGroup[w].Init(myBulk);
    }
}

void WellGroup::ApplyControl(const USI& i)
{
    for (USI w = 0; w < numWell; w++) {
        wellGroup[w].opt = wellGroup[w].optSet[i];
    }
}

OCP_DBL WellGroup::CalCFL(const Bulk& myBulk, const OCP_DBL& dt) const
{
    OCP_DBL cflw = 0;
    OCP_DBL tmp  = 0;
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            tmp = wellGroup[w].CalCFL(myBulk, dt);
            if (cflw < tmp) cflw = tmp;
        }
    }
    return cflw;
}

void WellGroup::CalCFL01(const Bulk& myBulk, const OCP_DBL& dt) const
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].CalCFL01(myBulk, dt);
        }
    }
}

void WellGroup::CalFlux(const Bulk& myBulk)
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].CalFlux(myBulk);
        }
    }
}

void WellGroup::CalTrans(const Bulk& myBulk)
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].CalTrans(myBulk);
        }
    }
}

void WellGroup::CaldG(const Bulk& myBulk)
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].CaldG(myBulk);
        }
    }
}

void WellGroup::MassConserve(Bulk& myBulk, OCP_DBL dt)
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].MassConserve(myBulk, dt);
        }
    }
}

void WellGroup::PrepareWell(const Bulk& myBulk)
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {

            wellGroup[w].CalTrans(myBulk);
            wellGroup[w].CalFlux(myBulk, true);
            wellGroup[w].CaldG(myBulk);
            // test
            // wellGroup[w].SmoothdG();
            wellGroup[w].CheckOptMode(myBulk);
        }
    }
}


void WellGroup::AllocateMat(LinearSolver& mySolver, const USI& bulknum) const
{
    USI maxNum= GetMaxWellPerNum() + 1;
    for (USI w = 0; w < numWell; w++) {
        wellGroup[w].AllocateMat(mySolver);
        mySolver.RowCapPlus(bulknum + w, maxNum);
    }
}


void WellGroup::AssemblaMat_WB_IMPEC(LinearSolver& mySolver, const Bulk& myBulk,
                                     const OCP_DBL& dt) const
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {

            switch (wellGroup[w].WellType()) {
                case INJ:
                    wellGroup[w].AssembleMat_INJ_IMPEC(myBulk, mySolver, dt);
                    break;
                case PROD:
                    wellGroup[w].AssembleMat_PROD_BLK_IMPEC(myBulk, mySolver, dt);
                    break;
                default:
                    OCP_ABORT("Wrong well type");
            }
        }
    }
}

void WellGroup::AssemblaMat_WB_FIM(LinearSolver& mySolver, const Bulk& myBulk,
    const OCP_DBL& dt) const
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {

            switch (wellGroup[w].WellType()) {
            case INJ:
                wellGroup[w].AssembleMat_INJ_FIM(myBulk, mySolver, dt);
                break;
            case PROD:
                wellGroup[w].AssembleMat_PROD_BLK_FIM(myBulk, mySolver, dt);
                break;
            default:
                OCP_ABORT("Wrong well type");
            }
        }
    }
}

void WellGroup::GetSol_IMPEC(const vector<OCP_DBL>& u, const OCP_USI& bId)
{
    USI wId = 0;
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].BHP = u[bId + wId];
            wellGroup[w].UpdatePerfP();
            wId++;
        }
    }
}

void WellGroup::GetSol_FIM(const vector<OCP_DBL>& u, const OCP_USI& bId, const USI& len)
{
    USI wId = 0;
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].BHP += u[(bId + wId)*len];
            wellGroup[w].UpdatePerfP();
            wId++;
        }
    }
}


void WellGroup::CalResFIM(ResFIM& resFIM, const Bulk& myBulk, const OCP_DBL& dt) const
{
    USI wId = 0;
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {
            wellGroup[w].CalResFIM(resFIM, myBulk, dt, wId);
            wId++;
        }
    }
}


void WellGroup::CalIPRT(const Bulk& myBulk, OCP_DBL dt)
{
    FGIR = 0;
    FWIR = 0;
    FOPR = 0;
    FGPR = 0;
    FWPR = 0;
    for (USI w = 0; w < numWell; w++) {
        wellGroup[w].WGIR = 0;
        wellGroup[w].WWIR = 0;
        wellGroup[w].WOPR = 0;
        wellGroup[w].WGPR = 0;
        wellGroup[w].WWPR = 0;

        if (wellGroup[w].WellState()) {
            if (wellGroup[w].WellType() == PROD) {
                wellGroup[w].CalProdQi_Blk(myBulk, dt);
            } else {
                wellGroup[w].CalInjQi_Blk(myBulk, dt);
            }
        }
        FGIR += wellGroup[w].WGIR;
        FWIR += wellGroup[w].WWIR;
        FOPR += wellGroup[w].WOPR;
        FGPR += wellGroup[w].WGPR;
        FWPR += wellGroup[w].WWPR;
    }
    FGIT += FGIR * dt;
    FWIT += FWIR * dt;
    FOPT += FOPR * dt;
    FGPt += FGPR * dt;
    FWPT += FWPR * dt;
}

OCP_INT WellGroup::CheckP(const Bulk& myBulk)
{
    // 0 : All correct
    // 1   : negative P, cut the timestep and resolve
    // 2.1 : change well mode to BHP, resolve
    // 2.2 : crossflow happens, then close corresponding perf, resolve
    bool flag2 = false;
    bool flag3 = false;

    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].WellState()) {

            OCP_INT flag = wellGroup[w].CheckP(myBulk);
#ifdef _DEBUG
            // wellGroup[w].ShowPerfStatus();
#endif // _DEBUG
            switch (flag) {
                case 1:
                    return 1;
                case 2:
                    flag2 = true;
                    break;
                case 3:
                    flag3 = true;
                    break;
                default:
                    break;
            }
        }
    }

    if (flag2 || flag3) return 2;

    return 0;
}


USI WellGroup::GetMaxWellPerNum() const
{
    USI m = 0;
    for (USI w = 0; w < numWell; w++) {
        m = max(m, wellGroup[w].numPerf);
    }
    return m;
}

// return the index of Specified well name
USI WellGroup::GetIndex(const string& name) const
{
    for (USI w = 0; w < numWell; w++) {
        if (wellGroup[w].name == name) {
            return w;
        }
    }
    OCP_ABORT("Well name not found!");
}

/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Oct/01/2021      Create file                          */
/*  Chensong Zhang      Oct/15/2021      Format file                          */
/*----------------------------------------------------------------------------*/