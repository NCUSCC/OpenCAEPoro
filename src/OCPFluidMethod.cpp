/*! \file    OCPFluidMethod.cpp
 *  \brief   Definition of solution methods for fluid part in OpenCAEPoro
 *  \author  Shizhe Li
 *  \date    Nov/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#include "OCPFluidMethod.hpp"

////////////////////////////////////////////
// OCP_IMPEC
////////////////////////////////////////////

void OCP_IMPEC::Setup(Reservoir& rs, LinearSystem& myLS, const OCPControl& ctrl)
{
    // Allocate Memory of auxiliary variables for IMPEC 
    rs.AllocateAuxIMPEC();
    // Allocate Memory of Matrix for IMPEC
    rs.AllocateMatIMPEC(myLS);
}

void OCP_IMPEC::Prepare(Reservoir& rs, OCP_DBL& dt)
{
    rs.PrepareWell();
    OCP_DBL cfl = rs.CalCFL01IMPEC(dt);
    if (cfl > 1) dt /= (cfl + 1);
}

void OCP_IMPEC::SolveLinearSystem(LinearSystem& myLS, Reservoir& rs, OCPControl& ctrl)
{
#ifdef _DEBUG
    myLS.CheckEquation();
#endif // DEBUG

    myLS.AssembleMatLinearSolver();

#ifdef DEBUG
    myLS.OutputLinearSystem("testA.out", "testb.out");
#endif // _DEBUG

    GetWallTime Timer;
    Timer.Start();
    int status = myLS.Solve();
    if (status < 0) {
        status = myLS.GetNumIters();
    }

    ctrl.UpdateTimeLS(Timer.Stop() / 1000);
    ctrl.UpdateIterLS(status);
    ctrl.UpdateIterNR();

#ifdef DEBUG
    myLS.OutputSolution("testx.out");
#endif // DEBUG

    rs.GetSolutionIMPEC(myLS.GetSolution());
    myLS.ClearData();
}

bool OCP_IMPEC::UpdateProperty(Reservoir& rs, OCPControl& ctrl)
{
    OCP_DBL& dt = ctrl.current_dt;

    // first check : Pressure check
    OCP_INT flagCheck = rs.CheckP();
    switch (flagCheck) {
        case 1:
            cout << "well change" << endl;
            dt /= 2;
            rs.ResetVal00IMPEC();
            return false;
        case 2:
            cout << "well change" << endl;
            dt /= 1;
            rs.ResetVal00IMPEC();
            // rs.ResetWellIMPEC();
            return false;
        default:
            break;
    }

    rs.CalFLuxIMPEC();

    // second check : CFL check
    OCP_DBL cfl = rs.CalCFL01IMPEC(dt);
    if (cfl > 1) {
        dt /= 2;
        rs.ResetVal01IMPEC();
        cout << "CFL is too big" << endl;
        return false;
    }

    rs.MassConseveIMPEC(dt);

    // third check: Ni check
    if (!rs.CheckNi()) {
        dt /= 2;
        rs.ResetVal02IMPEC();
        cout << "Negative Ni occurs\n";
        return false;
    }

    rs.CalVpore();
    rs.CalFlashIMPEC();

    // fouth check: Volume error check
    if (!rs.CheckVe(0.01)) {
        // cout << ctrl.GetCurTime() << "Days" << "=======" << endl;
        dt /= 2;
        rs.ResetVal03IMPEC();
        return false;
    }

    rs.CalKrPc();
    rs.CalConnFluxIMPEC();
    // rs.allWells.ShowWellStatus(rs.bulk);

    return true;
}

bool OCP_IMPEC::UpdateProperty01(Reservoir& rs, OCPControl& ctrl)
{
    OCP_DBL& dt = ctrl.current_dt;

    // first check : Pressure check
    OCP_INT flagCheck = rs.CheckP();
    switch (flagCheck) {
    case 1:
        cout << "well change" << endl;
        dt /= 2;
        rs.ResetVal00IMPEC();
        return false;
    case 2:
        cout << "well change" << endl;
        dt /= 1;
        rs.ResetVal00IMPEC();
        // rs.ResetWellIMPEC();
        return false;
    default:
        break;
    }

    rs.CalFLuxIMPEC();

    // second check : CFL check
    OCP_DBL cfl = rs.CalCFL01IMPEC(dt);
    if (cfl > 1) {
        dt /= 2;
        rs.ResetVal01IMPEC();
        cout << "CFL is too big" << endl;
        return false;
    }

    rs.MassConseveIMPEC(dt);

    // third check: Ni check
    if (!rs.CheckNi()) {
        dt /= 2;
        rs.ResetVal02IMPEC();
        cout << "Negative Ni occurs\n";
        return false;
    }

    rs.CalVpore();
    rs.CalFlashIMPEC();
    rs.CalKrPc();
    rs.CalConnFluxIMPEC();

    return true;
}

bool OCP_IMPEC::FinishNR01(Reservoir& rs, OCPControl& ctrl)
{
    OCP_DBL& dt = ctrl.current_dt;
    // fouth check: Volume error check
    if (!rs.CheckVe(0.01)) {
        // continue NR 
        rs.bulk.ResetNi();
        rs.allWells.CalTrans(rs.bulk);
        rs.allWells.CalFlux(rs.bulk);
        rs.allWells.CalProdWeight(rs.bulk);
        rs.allWells.CaldG(rs.bulk);
        return false;
    }
    return true;
}

void OCP_IMPEC::FinishStep(Reservoir& rs, OCPControl& ctrl)
{
    rs.CalIPRT(ctrl.GetCurDt());
    rs.CalMaxChange();
    rs.UpdateLastStepIMPEC();
    ctrl.CalNextTstepIMPEC(rs);
    ctrl.UpdateIters();
}

////////////////////////////////////////////
// OCP_FIM
////////////////////////////////////////////

void OCP_FIM::Setup(Reservoir& rs, LinearSystem& myLS, const OCPControl& ctrl)
{
    // Allocate Bulk and BulkConn Memory
    rs.AllocateAuxFIM();
    // Allocate memory for internal matrix structure
    rs.AllocateMatFIM(myLS);
    // Allocate memory for resiual of FIM
    OCP_USI num = (rs.GetBulkNum() + rs.GetWellNum()) * (rs.GetComNum() + 1);
    resFIM.res.resize(num);
}

void OCP_FIM::Prepare(Reservoir& rs, OCP_DBL& dt)
{
    rs.PrepareWell();
    rs.CalWellFlux();
    rs.CalResFIM(resFIM, dt);
    resFIM.maxRelRes0_v = resFIM.maxRelRes_v;
}

void OCP_FIM::AssembleMat(LinearSystem& myLS, const Reservoir& rs,
                          const OCP_DBL& dt) const
{
    rs.AssembleMatFIM(myLS, dt);
    myLS.AssembleRhs(resFIM.res);
}

void OCP_FIM::SolveLinearSystem(LinearSystem& myLS, Reservoir& rs, OCPControl& ctrl)
{
#ifdef _DEBUG
    myLS.CheckEquation();
#endif // DEBUG

    myLS.AssembleMatLinearSolver();

    GetWallTime Timer;
    Timer.Start();
    int status = myLS.Solve();
    if (status < 0) {
        status = myLS.GetNumIters();
    }
    // cout << "LS step = " << status << endl;

#ifdef _DEBUG
    // myLS.OutputLinearSystem("testA.out", "testb.out");
    // myLS.OutputSolution("testx.out");
    myLS.CheckSolution();
#endif // DEBUG

    ctrl.UpdateTimeLS(Timer.Stop() / 1000);
    ctrl.UpdateIterLS(status);
    ctrl.UpdateIterNR();

    rs.GetSolutionFIM(myLS.GetSolution(), ctrl.ctrlNR.NRdPmax, ctrl.ctrlNR.NRdSmax);
    // rs.GetSolution01FIM(myLS.GetSolution());
    // rs.PrintSolFIM(ctrl.workDir + "testPNi.out");
    myLS.ClearData();
}

bool OCP_FIM::UpdateProperty(Reservoir& rs, OCPControl& ctrl)
{
    OCP_DBL& dt = ctrl.current_dt;

    // Second check: Ni check and bulk Pressure check
    if (!rs.CheckNi() || rs.CheckP(true, false) != 0) {
        dt *= ctrl.ctrlTime.cutFacNR;
        rs.ResetFIM(false);
        rs.CalResFIM(resFIM, dt);
        resFIM.maxRelRes0_v = resFIM.maxRelRes_v;
        cout << "Cut time stepsize and repeat!\n";
        return false;
    }

    // Update reservoir properties
    rs.CalFlashDerivFIM();
    rs.CalKrPcDerivFIM();
    rs.CalVpore();
    rs.CalWellTrans();
    rs.CalWellFlux();
    rs.CalResFIM(resFIM, dt);
    return true;
}

bool OCP_FIM::FinishNR(Reservoir& rs, OCPControl& ctrl)
{
    OCP_DBL NRdPmax = rs.GetNRdPmax();
    OCP_DBL NRdSmax = rs.GetNRdSmax();

    //#ifdef _DEBUG
    // cout << "### DEBUG: Residuals = " << scientific << resFIM.maxRelRes0_v << "  "
    //    << resFIM.maxRelRes_v << "  " << resFIM.maxRelRes_mol << "  " << NRdSmax
    //    << "  " << NRdPmax << endl;
    // cout << "bk[0]: " << rs.bulk.GetSOIL(0) << "   " << rs.bulk.GetSGAS(0) << endl;
    //#endif

    if (ctrl.iterNR > ctrl.ctrlNR.maxNRiter) {
        ctrl.current_dt *= ctrl.ctrlTime.cutFacNR;
        rs.ResetFIM(false);
        rs.CalResFIM(resFIM, ctrl.current_dt);
        resFIM.maxRelRes0_v = resFIM.maxRelRes_v;
        ctrl.ResetIterNRLS();
        cout << "### WARNING: NR not fully converged! Cut time stepsize and repeat!\n";
        return false;
    }

    if (resFIM.maxRelRes_v <= resFIM.maxRelRes0_v * ctrl.ctrlNR.NRtol ||
        resFIM.maxRelRes_v <= ctrl.ctrlNR.NRtol ||
        resFIM.maxRelRes_mol <= ctrl.ctrlNR.NRtol ||
        (NRdPmax <= ctrl.ctrlNR.NRdPmin && NRdSmax <= ctrl.ctrlNR.NRdSmin)) {

        OCP_INT flagCheck = rs.CheckP(false, true);
#if DEBUG
        if (flagCheck > 0) {
            cout << ">> Switch well constraint: Case " << flagCheck << endl;
        }
#endif

        switch (flagCheck) {
            case 1:
                ctrl.current_dt *= ctrl.ctrlTime.cutFacNR;
                rs.ResetFIM(true);
                rs.CalResFIM(resFIM, ctrl.current_dt);
                resFIM.maxRelRes0_v = resFIM.maxRelRes_v;
                ctrl.ResetIterNRLS();
                return false;
            case 2:
                ctrl.current_dt /= 1;
                rs.ResetFIM(true);
                rs.CalResFIM(resFIM, ctrl.current_dt);
                resFIM.maxRelRes0_v = resFIM.maxRelRes_v;
                ctrl.ResetIterNRLS();
                return false;
            default:
                break;
        }
        return true;
    } else {
        return false;
    }
}

void OCP_FIM::FinishStep(Reservoir& rs, OCPControl& ctrl)
{
    rs.CalIPRT(ctrl.GetCurDt());
    rs.CalMaxChange();
    rs.UpdateLastStepFIM();
    ctrl.CalNextTstepFIM(rs);
    ctrl.UpdateIters();
}

////////////////////////////////////////////
// OCP_AIMt
////////////////////////////////////////////

void OCP_AIMt::Setup(Reservoir& rs, LinearSystem& myLS, LinearSystem& myAuxLS, const OCPControl& ctrl)
{
    // Allocate Memory of auxiliary variables for AIMt 
    rs.AllocateAuxAIMt();
    // Allocate Memory of Matrix for IMPEC
    rs.AllocateMatIMPEC(myLS);
    // Allocate memory for internal matrix structure for local FIM
    rs.AllocateMatAIMt(myAuxLS);
    // Allocate memory for resiual of FIM
    OCP_USI num = (rs.GetMaxFIMBulk() + rs.GetWellNum()) * (rs.GetComNum() + 1);
    resFIM.res.resize(num);
}

void OCP_AIMt::Prepare(Reservoir& rs, OCP_DBL& dt)
{
    rs.PrepareWell();
    OCP_DBL cfl = rs.CalCFL01IMPEC(dt);
    if (cfl > 1) dt /= (cfl + 1);

    // setup WellbulkId
    rs.SetupWellBulk();
}


bool OCP_AIMt::UpdateProperty(Reservoir& rs, OCPControl& ctrl, LinearSystem& myAuxLS)
{
    OCP_DBL& dt = ctrl.current_dt;

    rs.CalFLuxIMPEC();
    rs.CalCFL01IMPEC(dt);
    rs.MassConseveIMPEC(dt);
    rs.CalVpore();
    rs.CalFlashIMPEC();

    // Perform FIM in local grid
    // Init
    rs.SetupFIMBulk();
    rs.CalFlashDerivAIMt();
    rs.CalKrPcDerivAIMt();
    rs.ResetAIMt();
    rs.AssembleMatAIMt(myAuxLS, dt);


    // Pressure check
    OCP_INT flagCheck = rs.CheckP();
    switch (flagCheck) {
    case 1:
        cout << "well change" << endl;
        dt /= 2;
        rs.ResetVal00IMPEC();
        return false;
    case 2:
        cout << "well change" << endl;
        dt /= 1;
        rs.ResetVal00IMPEC();
        // rs.ResetWellIMPEC();
        return false;
    default:
        break;
    }

    rs.CalKrPc();
    rs.CalConnFluxIMPEC();
}




/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Nov/01/2021      Create file                          */
/*  Chensong Zhang      Jan/08/2022      Update output                        */
/*----------------------------------------------------------------------------*/