/*! \file    Bulk.cpp
 *  \brief   Bulk class definition
 *  \author  Shizhe Li
 *  \date    Oct/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>
#include <ctime>

// OpenCAEPoro header files
#include "Bulk.hpp"

/////////////////////////////////////////////////////////////////////
// General
/////////////////////////////////////////////////////////////////////

/// Read parameters from rs_param data structure.
void Bulk::InputParam(ParamReservoir& rs_param)
{
    OCP_FUNCNAME;

    // Common input  
    blackOil   = rs_param.blackOil;
    comps      = rs_param.comps;
    thermal    = rs_param.thermal;
    
    NTPVT      = rs_param.NTPVT;
    NTSFUN     = rs_param.NTSFUN;
    NTROCC     = rs_param.NTROOC;

    ScalePcow = rs_param.ScalePcow;

    if (rs_param.PBVD_T.data.size() > 0) EQUIL.PBVD.Setup(rs_param.PBVD_T.data[0]);

    if (blackOil) {
        // Isothermal blackoil model
        InputParamBLKOIL(rs_param);
    } else if (comps) {
        // Isothermal compositional model
        InputParamCOMPS(rs_param);
    } else if (thermal) {
        // thermal model
        InputParamTHERMAL(rs_param);
    }

    InputSatFunc(rs_param);
    InputRockFunc(rs_param);
}


void Bulk::InputParamBLKOIL(ParamReservoir& rs_param)
{
    oil = rs_param.oil;
    gas = rs_param.gas;
    water = rs_param.water;
    disGas = rs_param.disGas;

    EQUIL.Dref = rs_param.EQUIL[0];
    EQUIL.Pref = rs_param.EQUIL[1];

    if (water && !oil && !gas) {
        // water
        numPhase = 1;
        numCom = 1;
        SATmode = PHASE_W;
        PVTmode = PHASE_W;
    }
    else if (water && oil && !gas) {
        // water, dead oil
        numPhase = 2;
        numCom = 2;
        EQUIL.DOWC = rs_param.EQUIL[2];
        EQUIL.PcOW = rs_param.EQUIL[3];
        SATmode = PHASE_OW;
        PVTmode = PHASE_OW;
    }
    else if (water && oil && gas && !disGas) {
        // water, dead oil, dry gas
        numPhase = 3;
        numCom = 3;
        EQUIL.DOWC = rs_param.EQUIL[2];
        EQUIL.PcOW = rs_param.EQUIL[3];
        EQUIL.DGOC = rs_param.EQUIL[4];
        EQUIL.PcGO = rs_param.EQUIL[5];
        SATmode = PHASE_DOGW;
        PVTmode = PHASE_DOGW; // maybe it should be added later
    }
    else if (water && oil && gas && disGas) {
        // water, live oil, dry gas
        numPhase = 3;
        numCom = 3;

        EQUIL.DOWC = rs_param.EQUIL[2];
        EQUIL.PcOW = rs_param.EQUIL[3];
        EQUIL.DGOC = rs_param.EQUIL[4];
        EQUIL.PcGO = rs_param.EQUIL[5];
        PVTmode = PHASE_ODGW;

        if (rs_param.SOF3_T.data.size() > 0) {
            SATmode = PHASE_ODGW02;
        }
        else {
            SATmode = PHASE_ODGW01;
        }
    }
    numCom_1 = numCom - 1;
    rs_param.numPhase = numPhase;
    rs_param.numCom = numCom;

    // PVT mode
    switch (PVTmode) {
    case PHASE_W:
        OCP_ABORT("Wrong Type!");
        break;
    case PHASE_OW:
        for (USI i = 0; i < NTPVT; i++)
            flashCal.push_back(new BOMixture_OW(rs_param, i));
        break;
    case PHASE_DOGW:
        OCP_ABORT("Wrong Type!");
        break;
    case PHASE_ODGW:
        for (USI i = 0; i < NTPVT; i++)
            flashCal.push_back(new BOMixture_ODGW(rs_param, i));
        break;
    default:
        OCP_ABORT("Wrong Type!");
        break;
    }

    phase2Index.resize(3);
    switch (flashCal[0]->GetMixtureType())
    {
    case BLKOIL_W:
        phase2Index[WATER] = 0;
        break;
    case BLKOIL_OW:
        phase2Index[OIL] = 0;
        phase2Index[WATER] = 1;
        break;
    case BLKOIL_OG:
        phase2Index[OIL] = 0;
        phase2Index[GAS] = 1;
        break;
    case BLKOIL_DOGW:
    case BLKOIL_ODGW:
        phase2Index[OIL] = 0;
        phase2Index[GAS] = 1;
        phase2Index[WATER] = 2;
        break;
    default:
        OCP_ABORT("WRONG Mixture Type!");
    }

    InputRockFunc(rs_param);
    cout << "BLACKOIL model" << endl;
}


void Bulk::InputParamCOMPS(const ParamReservoir& rs_param)
{
    
    miscible = rs_param.comsParam.miscible;

    // Water exists and is excluded in EoS model NOW!
    oil = OCP_TRUE;
    gas = OCP_TRUE;
    water = OCP_TRUE;
    useEoS = OCP_TRUE;

    numPhase = rs_param.comsParam.numPhase + 1;
    numCom = rs_param.comsParam.numCom + 1;
    numCom_1 = numCom - 1;
    EQUIL.Dref = rs_param.EQUIL[0];
    EQUIL.Pref = rs_param.EQUIL[1];
    EQUIL.DOWC = rs_param.EQUIL[2];
    EQUIL.PcOW = rs_param.EQUIL[3];
    EQUIL.DGOC = rs_param.EQUIL[4];
    EQUIL.PcGO = rs_param.EQUIL[5];

    // Init Zi
    for (auto& v : rs_param.ZMFVD_T.data) {
        initZi_Tab.push_back(OCPTable(v));
    }

    // Init T
    // Use RTEMP
    RTemp = rs_param.rsTemp;    // ��F -> ��R
    vector<vector<OCP_DBL>> temp;
    temp.resize(2);
    // add depth
    temp[0].push_back(0);
    temp[0].push_back(1E8);
    // add temperature
    temp[1].push_back(RTemp);
    temp[1].push_back(RTemp);
    initT_Tab.push_back(OCPTable(temp));

    // Saturation mode
    if (rs_param.SOF3_T.data.size() > 0) {
        SATmode = PHASE_ODGW02;
    }
    else {
        SATmode = PHASE_ODGW01;
        if (miscible){ SATmode = PHASE_ODGW01_MISCIBLE; }
    }

    // PVT mode
    for (USI i = 0; i < NTPVT; i++)
        flashCal.push_back(new MixtureComp(rs_param, i));

    // phase index
    phase2Index.resize(3);
    phase2Index[OIL]   = 0;
    phase2Index[GAS]   = 1;
    phase2Index[WATER] = 2;

    skipStaAnalyTerm.SetUseSkip(OCP_TRUE);

    InputRockFunc(rs_param);
    cout << "COMPOSITIONAL model" << endl;
}


void Bulk::InputParamTHERMAL(const ParamReservoir& rs_param)
{
    oil = rs_param.oil;
    gas = rs_param.gas;
    water = rs_param.water;

    // Init T
    RTemp = rs_param.rsTemp;    // F -> R
    for (auto& v : rs_param.TEMPVD_T.data) {
        initT_Tab.push_back(OCPTable(v));
    }
    if (initT_Tab.size() == 0) {
        // Use RTEMP
        vector<vector<OCP_DBL>> temp;
        temp.resize(2);
        // add depth
        temp[0].push_back(0);       temp[0].push_back(1E8);
        // add temperature
        temp[1].push_back(RTemp);   temp[1].push_back(RTemp);
        initT_Tab.push_back(OCPTable(temp));
    }
    // thermal conductivity
    if (oil) { thconp.push_back(rs_param.thcono); }
    if (gas) { thconp.push_back(rs_param.thcong); }
    if (water) { thconp.push_back(rs_param.thconw); }


    // PVT mode
    for (USI i = 0; i < NTPVT; i++)
        flashCal.push_back(new MixtureThermal_K01(rs_param, i));
        

    InputRockFuncT(rs_param);
}

void Bulk::InputSatFunc(const ParamReservoir& rs_param)
{
    // Setup Saturation function
    satcm.resize(NTSFUN);
    switch (SATmode) {
    case PHASE_W:
        for (USI i = 0; i < NTSFUN; i++)
            flow.push_back(new FlowUnit_W(rs_param, i));
        break;
    case PHASE_OW:
        for (USI i = 0; i < NTSFUN; i++)
            flow.push_back(new FlowUnit_OW(rs_param, i));
        break;
    case PHASE_ODGW01:
        for (USI i = 0; i < NTSFUN; i++) {
            flow.push_back(new FlowUnit_ODGW01(rs_param, i));
            satcm[i] = flow[i]->GetScm();
        }
        break;
    case PHASE_ODGW01_MISCIBLE:
        for (USI i = 0; i < NTSFUN; i++) {
            flow.push_back(new FlowUnit_ODGW01_Miscible(rs_param, i));
            satcm[i] = flow[i]->GetScm();
        }
        break;
    case PHASE_ODGW02:
        for (USI i = 0; i < NTSFUN; i++)
            flow.push_back(new FlowUnit_ODGW02(rs_param, i));
        break;
    default:
        OCP_ABORT("Wrong Type!");
        break;
    }
}


void Bulk::InputRockFunc(const ParamReservoir& rs_param)
{
    for (USI i = 0; i < NTROCC; i++) {
        rock.push_back(new Rock_Linear(rs_param.rockSet[i]));
    }
}


void Bulk::InputRockFuncT(const ParamReservoir& rs_param)
{

    for (USI i = 0; i < NTROCC; i++) {
        if (rs_param.rockSet[i].type == "LINEAR") {
            rock.push_back(new RockT_Linear(rs_param.rockSet[i]));
        }
        else {
            rock.push_back(new RockT_Exp(rs_param.rockSet[i]));
        }
    }
}


/// Setup bulk information.
void Bulk::Setup(const Grid& myGrid)
{
    OCP_FUNCNAME;

    // Rock/Grid properties

    numBulk = myGrid.activeGridNum;
    depth.resize(numBulk, 0);
    ntg.resize(numBulk, 0);
    poroInit.resize(numBulk, 0);
    poro.resize(numBulk, 0);
    rockVntg.resize(numBulk, 0);
    rockVp.resize(numBulk, 0);
    rockKx.resize(numBulk, 0);
    rockKy.resize(numBulk, 0);
    rockKz.resize(numBulk, 0);
    SATNUM.resize(numBulk, 0);
    PVTNUM.resize(numBulk, 0);
    ROCKNUM.resize(numBulk, 0);

    if (myGrid.SwatInit.size() > 0) {
        SwatInitExist = OCP_TRUE;
        SwatInit.resize(numBulk);
    }
    if (ScalePcow) {
        ScaleValuePcow.resize(numBulk, 0);
    }

    for (OCP_USI bIdb = 0; bIdb < numBulk; bIdb++) {
        OCP_USI bIdg = myGrid.activeMap_B2G[bIdb];

        depth[bIdb] = myGrid.depth[bIdg];
        ntg[bIdb]   = myGrid.ntg[bIdg];

        poroInit[bIdb] = myGrid.poro[bIdg];

        rockVntg[bIdb] = myGrid.v[bIdg] * myGrid.ntg[bIdg];
        rockKx[bIdb] = myGrid.kx[bIdg];
        rockKy[bIdb] = myGrid.ky[bIdg];
        rockKz[bIdb] = myGrid.kz[bIdg];

        SATNUM[bIdb] = myGrid.SATNUM[bIdg];
        PVTNUM[bIdb] = myGrid.PVTNUM[bIdg];
        ROCKNUM[bIdb] = myGrid.ROCKNUM[bIdg];

        if (SwatInitExist) {
            SwatInit[bIdb] = myGrid.SwatInit[bIdg];
        }

        rockVp[bIdb] = rockVntg[bIdb] * poroInit[bIdb];
    }

    poro = poroInit;

    // physical variables
    Pb.resize(numBulk);
    T.resize(numBulk);
    P.resize(numBulk);
    Pj.resize(numBulk * numPhase);
    Pc.resize(numBulk * numPhase);
    phaseExist.resize(numBulk * numPhase);
    S.resize(numBulk * numPhase);
    rho.resize(numBulk * numPhase);
    xi.resize(numBulk * numPhase);
    xij.resize(numBulk * numPhase * numCom);
    Ni.resize(numBulk * numCom);
    mu.resize(numBulk * numPhase);
    kr.resize(numBulk * numPhase);
    vj.resize(numBulk * numPhase);
    vf.resize(numBulk);
    Nt.resize(numBulk);

    // Phase num
    phaseNum.resize(numBulk);
    lphaseNum.resize(numBulk);
    NRphaseNum.resize(numBulk);

    if (skipStaAnalyTerm.IfUseSkip()) {
        // accelerate phase equilibrium calculation
        skipStaAnalyTerm.Setup(numBulk, numCom_1);
        lskipStaAnalyTerm = skipStaAnalyTerm; // Initialization

        // error
        ePEC.resize(numBulk);
    }

    if (miscible) {
        surTen.resize(numBulk);
        Fk.resize(numBulk);
        Fp.resize(numBulk);
        lsurTen.resize(numBulk);
    }


    // error  
    eN.resize(numBulk);
    eV.resize(numBulk);

    CalSomeInfo(myGrid);

#if DEBUG
    CheckSetup();
#endif
}

void Bulk::CheckSetup() const
{
    CheckInitVpore();
    CheckVpore();
}

/// Check init pore volume.
void Bulk::CheckInitVpore() const
{
    for (OCP_USI n = 0; n < numBulk; n++) {
        if (rockVntg[n] < TINY) {
            OCP_ABORT("Bulk volume is too small: bulk[" + std::to_string(n) +
                      "] = " + std::to_string(rockVntg[n]));
        }
    }
}

// Check pore volume.
void Bulk::CheckVpore() const
{
    for (OCP_USI n = 0; n < numBulk; n++) {
        if (rockVp[n] < TINY) {
            OCP_ABORT("Bulk volume is too small: bulk[" + std::to_string(n) +
                      "] = " + std::to_string(rockVp[n]));
        }
    }
}


void Bulk::InitSjPc(const Grid& myGrid, const USI& tabrow)
{
    OCP_FUNCNAME;

    OCP_DBL Dref = EQUIL.Dref;
    OCP_DBL Pref = EQUIL.Pref;
    OCP_DBL DOWC = EQUIL.DOWC;
    OCP_DBL PcOW = EQUIL.PcOW;
    OCP_DBL DOGC = EQUIL.DGOC;
    OCP_DBL PcGO = EQUIL.PcGO;
    OCP_DBL Zmin = 1E8;
    OCP_DBL Zmax = 0;

    for (OCP_USI n = 0; n < numBulk; n++) {
        OCP_DBL temp1 = depth[n] - myGrid.Dz(n) / 2;
        OCP_DBL temp2 = depth[n] + myGrid.Dz(n) / 2;
        Zmin = Zmin < temp1 ? Zmin : temp1;
        Zmax = Zmax > temp2 ? Zmax : temp2;
    }
    OCP_DBL tabdz = (Zmax - Zmin) / (tabrow - 1);

    // creater table
    OCPTable         DepthP(tabrow, 4);
    vector<OCP_DBL>& Ztmp = DepthP.GetCol(0);
    vector<OCP_DBL>& Potmp = DepthP.GetCol(1);
    vector<OCP_DBL>& Pgtmp = DepthP.GetCol(2);
    vector<OCP_DBL>& Pwtmp = DepthP.GetCol(3);

    vector<OCP_DBL> tmpInitZi(numCom, 0);

    // cal Tab_Ztmp
    Ztmp[0] = Zmin;
    for (USI i = 1; i < tabrow; i++) {
        Ztmp[i] = Ztmp[i - 1] + tabdz;
    }

    OCP_DBL myTemp = RTemp;

    // find the RefId
    USI beginId = 0;
    if (Dref <= Ztmp[0]) {
        beginId = 0;
    }
    else if (Dref >= Ztmp[tabrow - 1]) {
        beginId = tabrow - 1;
    }
    else {
        beginId =
            distance(Ztmp.begin(), find_if(Ztmp.begin(), Ztmp.end(),
                [s = Dref](auto& t) { return t > s; }));
        beginId--;
    }

    // begin calculating oil pressure:
    OCP_DBL Pbb = Pref;
    OCP_DBL gammaOtmp, gammaWtmp, gammaGtmp;
    OCP_DBL Ptmp;
    USI     mynum = 10;
    OCP_DBL mydz = 0;
    OCP_DBL Poref, Pgref, Pwref;
    OCP_DBL Pbegin = 0;

    const OCP_BOOL initZi_flag = initZi_Tab.size() > 0 ? OCP_TRUE : OCP_FALSE;
    const OCP_BOOL initT_flag = initT_Tab.size() > 0 ? OCP_TRUE : OCP_FALSE;
    const OCP_BOOL PBVD_flag = EQUIL.PBVD.IsEmpty() ? OCP_FALSE : OCP_TRUE;

    if (Dref < DOGC) {
        // reference pressure is gas pressure
        Pgref = Pref;
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);
        if (PBVD_flag)
            Pbb = EQUIL.PBVD.Eval(0, Dref, 1);

        gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgref, Pbb, myTemp, &tmpInitZi[0], GAS);
        Pbegin = Pgref + gammaGtmp * (Ztmp[beginId] - Dref);
        Pgtmp[beginId] = Pbegin;

        // find the gas pressure
        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgtmp[id], Pbb, myTemp, &tmpInitZi[0], GAS);
            Pgtmp[id - 1] = Pgtmp[id] - gammaGtmp * (Ztmp[id] - Ztmp[id - 1]);
        }

        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgtmp[id], Pbb, myTemp, &tmpInitZi[0], GAS);
            Pgtmp[id + 1] = Pgtmp[id] + gammaGtmp * (Ztmp[id + 1] - Ztmp[id]);
        }

        // find the oil pressure in Dref by Pgref
        Poref = 0;
        Ptmp = Pgref;
        mydz = (DOGC - Dref) / mynum;
        OCP_DBL myz = Dref;

        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, myz, 1);
            
            gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], GAS);
            Ptmp += gammaGtmp * mydz;
            myz += mydz;
        }
        Ptmp -= PcGO;
        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, myz, 1);
           
            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], OIL);
            Ptmp -= gammaOtmp * mydz;
            myz -= mydz;
        }
        Poref = Ptmp;

        // find the oil pressure in tab
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);
        if (PBVD_flag)
            Pbb = EQUIL.PBVD.Eval(0, Dref, 1);

        gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Poref, Pbb, myTemp, &tmpInitZi[0], OIL);
        Pbegin = Poref + gammaOtmp * (Ztmp[beginId] - Dref);
        Potmp[beginId] = Pbegin;

        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Potmp[id], Pbb, myTemp, &tmpInitZi[0], OIL);
            Potmp[id - 1] = Potmp[id] - gammaOtmp * (Ztmp[id] - Ztmp[id - 1]);
        }

        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Potmp[id], Pbb, myTemp, &tmpInitZi[0], OIL);
            Potmp[id + 1] = Potmp[id] + gammaOtmp * (Ztmp[id + 1] - Ztmp[id]);
        }

        // find the water pressure in Dref by Poref
        Pwref = 0;
        Ptmp = Poref;
        mydz = (DOWC - Dref) / mynum;
        myz = Dref;

        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, myz, 1);
            
            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Poref, Pbb, myTemp, &tmpInitZi[0], OIL);
            Ptmp += gammaOtmp * mydz;
            myz += mydz;
        }
        Ptmp -= PcOW;
        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], WATER);
            Ptmp -= gammaWtmp * mydz;
            myz -= mydz;
        }
        Pwref = Ptmp;

        // find the water pressure in tab
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);

        gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwref, Pbb, myTemp, &tmpInitZi[0], WATER);
        Pbegin = Pwref + gammaWtmp * (Ztmp[beginId] - Dref);
        Pwtmp[beginId] = Pbegin;

        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwtmp[id], Pbb, myTemp, &tmpInitZi[0], WATER);
            Pwtmp[id - 1] = Pwtmp[id] - gammaWtmp * (Ztmp[id] - Ztmp[id - 1]);
        }

        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwtmp[id], Pbb, myTemp, &tmpInitZi[0], WATER);
            Pwtmp[id + 1] = Pwtmp[id] + gammaWtmp * (Ztmp[id + 1] - Ztmp[id]);
        }
    }
    else if (Dref > DOWC) {
        OCP_DBL myz;
        // reference pressure is water pressure
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);

        Pwref = Pref;
        gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwref, Pbb, myTemp, &tmpInitZi[0], WATER);
        Pbegin = Pwref + gammaWtmp * (Ztmp[beginId] - Dref);
        Pwtmp[beginId] = Pbegin;

        // find the water pressure
        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwtmp[id], Pbb, myTemp, &tmpInitZi[0], WATER);
            Pwtmp[id - 1] = Pwtmp[id] - gammaWtmp * (Ztmp[id] - Ztmp[id - 1]);
        }
        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwtmp[id], Pbb, myTemp, &tmpInitZi[0], WATER);
            Pwtmp[id + 1] = Pwtmp[id] + gammaWtmp * (Ztmp[id + 1] - Ztmp[id]);
        }

        // find the oil pressure in Dref by Pwref
        Poref = 0;
        Ptmp = Pwref;
        mydz = (DOWC - Dref) / mynum;
        myz = Dref;

        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], WATER);
            Ptmp += gammaWtmp * mydz;
            myz += mydz;
        }
        Ptmp += PcOW;

        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, myz, 1);
            
            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], OIL);
            Ptmp -= gammaOtmp * mydz;
            myz -= mydz;
        }
        Poref = Ptmp;

        // find the oil pressure in tab
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);
        if (PBVD_flag)
            Pbb = EQUIL.PBVD.Eval(0, Dref, 1);

        gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Poref, Pbb, myTemp, &tmpInitZi[0], OIL);
        Pbegin = Poref + gammaOtmp * (Ztmp[beginId] - Dref);
        Potmp[beginId] = Pbegin;

        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Potmp[id], Pbb, myTemp, &tmpInitZi[0], OIL);
            Potmp[id - 1] = Potmp[id] - gammaOtmp * (Ztmp[id] - Ztmp[id - 1]);
        }

        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Potmp[id], Pbb, myTemp, &tmpInitZi[0], OIL);
            Potmp[id + 1] = Potmp[id] + gammaOtmp * (Ztmp[id + 1] - Ztmp[id]);
        }

        if (gas) {
            // find the gas pressure in Dref by Poref
            Pgref = 0;
            Ptmp = Poref;
            mydz = (DOGC - Dref) / mynum;
            myz = Dref;

            for (USI i = 0; i < mynum; i++) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(myz, tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, myz, 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, myz, 1);

                gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], OIL);
                Ptmp += gammaOtmp * mydz;
                myz += mydz;
            }
            Ptmp += PcGO;
            for (USI i = 0; i < mynum; i++) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(myz, tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, myz, 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, myz, 1);

                gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], GAS);
                Ptmp -= gammaGtmp * mydz;
                myz -= mydz;
            }
            Pgref = Ptmp;

            // find the gas pressure in tab
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Dref, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Dref, 1);

            gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgref, Pbb, myTemp, &tmpInitZi[0], GAS);
            Pbegin = Pgref + gammaGtmp * (Ztmp[beginId] - Dref);
            Pgtmp[beginId] = Pbegin;

            for (USI id = beginId; id > 0; id--) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

                gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgtmp[id], Pbb, myTemp, &tmpInitZi[0], GAS);
                Pgtmp[id - 1] = Pgtmp[id] - gammaGtmp * (Ztmp[id] - Ztmp[id - 1]);
            }
            for (USI id = beginId; id < tabrow - 1; id++) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

                gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgtmp[id], Pbb, myTemp, &tmpInitZi[0], GAS);
                Pgtmp[id + 1] = Pgtmp[id] + gammaGtmp * (Ztmp[id + 1] - Ztmp[id]);
            }
        }
        
    }
    else {
        OCP_DBL myz;
        // reference pressure is oil pressure
        Poref = Pref;
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);
        if (PBVD_flag)
            Pbb = EQUIL.PBVD.Eval(0, Dref, 1);

        gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Poref, Pbb, myTemp, &tmpInitZi[0], OIL);
        Pbegin = Poref + gammaOtmp * (Ztmp[beginId] - Dref);
        Potmp[beginId] = Pbegin;

        // find the oil pressure
        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Potmp[id], Pbb, myTemp, &tmpInitZi[0], OIL);
            Potmp[id - 1] = Potmp[id] - gammaOtmp * (Ztmp[id] - Ztmp[id - 1]);
        }
        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Potmp[id], Pbb, myTemp, &tmpInitZi[0], OIL);
            Potmp[id + 1] = Potmp[id] + gammaOtmp * (Ztmp[id + 1] - Ztmp[id]);
        }

        if (gas) {
            // find the gas pressure in Dref by Poref
            Pgref = 0;
            Ptmp = Poref;
            mydz = (DOGC - Dref) / mynum;
            myz = Dref;

            for (USI i = 0; i < mynum; i++) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(myz, tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, myz, 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, myz, 1);

                gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], OIL);
                Ptmp += gammaOtmp * mydz;
                myz += mydz;
            }
            Ptmp += PcGO;
            for (USI i = 0; i < mynum; i++) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(myz, tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, myz, 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, myz, 1);

                gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], GAS);
                Ptmp -= gammaGtmp * mydz;
                myz -= mydz;
            }
            Pgref = Ptmp;

            // find the gas pressure in tab
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Dref, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, Dref, 1);

            gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgref, Pbb, myTemp, &tmpInitZi[0], GAS);
            Pbegin = Pgref + gammaGtmp * (Ztmp[beginId] - Dref);
            Pgtmp[beginId] = Pbegin;

            for (USI id = beginId; id > 0; id--) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

                gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgtmp[id], Pbb, myTemp, &tmpInitZi[0], GAS);
                Pgtmp[id - 1] = Pgtmp[id] - gammaGtmp * (Ztmp[id] - Ztmp[id - 1]);
            }

            for (USI id = beginId; id < tabrow - 1; id++) {
                if (initZi_flag)
                    initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
                if (initT_flag)
                    myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);
                if (PBVD_flag)
                    Pbb = EQUIL.PBVD.Eval(0, Ztmp[id], 1);

                gammaGtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pgtmp[id], Pbb, myTemp, &tmpInitZi[0], GAS);
                Pgtmp[id + 1] = Pgtmp[id] + gammaGtmp * (Ztmp[id + 1] - Ztmp[id]);
            }
        }
      
        // find the water pressure in Dref by Poref
        Pwref = 0;
        Ptmp = Poref;
        mydz = (DOWC - Dref) / mynum;
        myz = Dref;

        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);
            if (PBVD_flag)
                Pbb = EQUIL.PBVD.Eval(0, myz, 1);

            gammaOtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], OIL);
            Ptmp += gammaOtmp * mydz;
            myz += mydz;
        }
        Ptmp -= PcOW;
        for (USI i = 0; i < mynum; i++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(myz, tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, myz, 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Ptmp, Pbb, myTemp, &tmpInitZi[0], WATER);
            Ptmp -= gammaWtmp * mydz;
            myz -= mydz;
        }
        Pwref = Ptmp;

        // find the water pressure in tab
        if (initZi_flag)
            initZi_Tab[0].Eval_All0(Dref, tmpInitZi);
        if (initT_flag)
            myTemp = initT_Tab[0].Eval(0, Dref, 1);

        gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwref, Pbb, myTemp, &tmpInitZi[0], WATER);
        Pbegin = Pwref + gammaWtmp * (Ztmp[beginId] - Dref);
        Pwtmp[beginId] = Pbegin;

        for (USI id = beginId; id > 0; id--) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwtmp[id], Pbb, myTemp, &tmpInitZi[0], WATER);
            Pwtmp[id - 1] = Pwtmp[id] - gammaWtmp * (Ztmp[id] - Ztmp[id - 1]);
        }

        for (USI id = beginId; id < tabrow - 1; id++) {
            if (initZi_flag)
                initZi_Tab[0].Eval_All0(Ztmp[id], tmpInitZi);
            if (initT_flag)
                myTemp = initT_Tab[0].Eval(0, Ztmp[id], 1);

            gammaWtmp = GRAVITY_FACTOR * flashCal[0]->RhoPhase(Pwtmp[id], Pbb, myTemp, &tmpInitZi[0], WATER);
            Pwtmp[id + 1] = Pwtmp[id] + gammaWtmp * (Ztmp[id + 1] - Ztmp[id]);
        }
    }

    DepthP.Display();

    // calculate Pc from DepthP to calculate Sj
    std::vector<OCP_DBL> data(4, 0), cdata(4, 0);
    // if capillary between water and oil is considered
    vector<OCP_BOOL> FlagPcow(NTSFUN, OCP_TRUE);
    for (USI i = 0; i < NTSFUN; i++) {
        if (fabs(flow[i]->GetPcowBySw(0.0 - TINY)) < TINY &&
            fabs(flow[i]->GetPcowBySw(1.0 + TINY) < TINY)) {
            FlagPcow[i] = OCP_FALSE;
        }
    }

    for (OCP_USI n = 0; n < numBulk; n++) {
        if (initZi_flag) {
            initZi_Tab[0].Eval_All0(depth[n], tmpInitZi);
            for (USI i = 0; i < numCom_1; i++) {
                Ni[n * numCom + i] = tmpInitZi[i];
            }
        }
        if (initT_flag) {
            myTemp = initT_Tab[0].Eval(0, depth[n], 1);
            T[n] = myTemp;
        }
        
        DepthP.Eval_All(0, depth[n], data, cdata);
        OCP_DBL Po = data[1];
        OCP_DBL Pg = data[2];
        OCP_DBL Pw = data[3];
        OCP_DBL Pcgo = Pg - Po;
        OCP_DBL Pcow = Po - Pw;
        OCP_DBL Sw = flow[SATNUM[n]]->GetSwByPcow(Pcow);
        OCP_DBL Sg = 0;
        if (gas) {
            Sg = flow[SATNUM[n]]->GetSgByPcgo(Pcgo);
        }
        if (Sw + Sg > 1) {
            // should me modified
            OCP_DBL Pcgw = Pcow + Pcgo;
            Sw = flow[SATNUM[n]]->GetSwByPcgw(Pcgw);
            Sg = 1 - Sw;
        }

        if (1 - Sw < TINY) {
            // all water
            Po = Pw + flow[SATNUM[n]]->GetPcowBySw(1.0);
        }
        else if (1 - Sg < TINY) {
            // all gas
            Po = Pg - flow[SATNUM[n]]->GetPcgoBySg(1.0);
        }
        else if (1 - Sw - Sg < TINY) {
            // water and gas
            Po = Pg - flow[SATNUM[n]]->GetPcgoBySg(Sg);
        }
        P[n] = Po;

        if (depth[n] < DOGC) {
            Pbb = Po;
        }
        else if (PBVD_flag) {
            Pbb = EQUIL.PBVD.Eval(0, depth[n], 1);
        }
        Pb[n] = Pbb;

        // cal Sw
        OCP_DBL swco = flow[SATNUM[n]]->GetSwco();
        if (!FlagPcow[SATNUM[n]]) {
            S[n * numPhase + numPhase - 1] = swco;
            continue;
        }

        Sw = 0;
        Sg = 0;
        USI     ncut = 10;
        OCP_DBL avePcow = 0;

        for (USI k = 0; k < ncut; k++) {
            OCP_DBL tmpSw = 0;
            OCP_DBL tmpSg = 0;
            OCP_DBL dep = depth[n] + myGrid.Dz(n) / ncut * (k - (ncut - 1) / 2.0);
            DepthP.Eval_All(0, dep, data, cdata);
            Po = data[1];
            Pg = data[2];
            Pw = data[3];
            Pcow = Po - Pw;
            Pcgo = Pg - Po;
            avePcow += Pcow;
            tmpSw = flow[SATNUM[n]]->GetSwByPcow(Pcow);
            if (gas) {
                tmpSg = flow[SATNUM[n]]->GetSgByPcgo(Pcgo);
            }
            if (tmpSw + tmpSg > 1) {
                // should be modified
                OCP_DBL Pcgw = Pcow + Pcgo;
                tmpSw = flow[SATNUM[n]]->GetSwByPcgw(Pcgw);
                tmpSg = 1 - tmpSw;
            }
            Sw += tmpSw;
            // Sg += tmpSg;
        }
        Sw /= ncut;
        // Sg /= ncut;
        avePcow /= ncut;

        if (SwatInitExist) {
            if (ScalePcow) {
                ScaleValuePcow[n] = 1.0;
            }
            if (SwatInit[n] <= swco) {
                Sw = swco;
            }
            else {
                Sw = SwatInit[n];
                if (ScalePcow) {
                    if (avePcow > 0) {
                         OCP_DBL tmp = flow[SATNUM[n]]->GetPcowBySw(Sw);
                        if (tmp > 0) {
                            ScaleValuePcow[n] = avePcow / tmp;
                        }
                    }
                }
            }
        }
        S[n * numPhase + numPhase - 1] = Sw;
    }
}


/// Use initial saturation in blackoil model and initial Zi in compositional model.
/// It gives initial properties and some derivatives for IMPEC
/// It just gives Ni for FIM
void Bulk::InitFlash(const OCP_BOOL& flag)
{
    OCP_FUNCNAME;

    for (OCP_USI n = 0; n < numBulk; n++) {
        flashCal[PVTNUM[n]]->InitFlash(P[n], Pb[n], T[n], &S[n * numPhase], rockVp[n],
                                       Ni.data() + n * numCom);
        for (USI i = 0; i < numCom; i++) {
            Ni[n * numCom + i] = flashCal[PVTNUM[n]]->Ni[i];
        }
        if (flag) {
            PassFlashValue(n);
        }
    }

#ifdef DEBUG
    if (flag) {
        CheckSat();
    }
#endif // DEBUG
}

void Bulk::InitFlashDer()
{
    OCP_FUNCNAME;

    if (useEoS) {
        for (OCP_USI n = 0; n < numBulk; n++) {
            flashCal[PVTNUM[n]]->InitFlashDer(P[n], Pb[n], T[n], &S[n * numPhase],
                                              rockVp[n], Ni.data() + n * numCom);
            for (USI i = 0; i < numCom; i++) {
                Ni[n * numCom + i] = flashCal[PVTNUM[n]]->Ni[i];
            }
            PassFlashValueDeriv(n);
        }
    } else {
        InitFlash(OCP_FALSE);
        FlashDeriv();
    }
}

void Bulk::InitFlashDer_n()
{
    OCP_FUNCNAME;

    if (comps) {
        for (OCP_USI n = 0; n < numBulk; n++) {
            flashCal[PVTNUM[n]]->InitFlashDer_n(P[n], Pb[n], T[n], &S[n * numPhase],
                                                rockVp[n], Ni.data() + n * numCom);
            for (USI i = 0; i < numCom; i++) {
                Ni[n * numCom + i] = flashCal[PVTNUM[n]]->Ni[i];
            }
            PassFlashValueDeriv_n(n);
        }
    } else {
        OCP_ABORT("Not Completed in BLKOIL MODEL!");
    }
}

/// Use moles of component and pressure both in blackoil and compositional model.
void Bulk::Flash()
{
    OCP_FUNCNAME;

    USI ftype;
    for (OCP_USI n = 0; n < numBulk; n++) {

        ftype = CalFlashType(n, OCP_FALSE);

        flashCal[PVTNUM[n]]->Flash(P[n], T[n], &Ni[n * numCom], ftype, phaseNum[n],
            &xij[n * numPhase * numCom]);
        PassFlashValue(n);
    }

#ifdef DEBUG
    CheckSat();
#endif // DEBUG
}

/// Use moles of component and pressure both in blackoil and compositional model.
void Bulk::FlashDeriv()
{
    OCP_FUNCNAME;

    USI ftype;
    NRdSSP = 0;
    maxNRdSSP = 0;
    index_maxNRdSSP = 0;

    for (OCP_USI n = 0; n < numBulk; n++) {

        ftype = CalFlashType(n, OCP_TRUE);

        flashCal[PVTNUM[n]]->FlashDeriv(P[n], T[n], &Ni[n * numCom], ftype, phaseNum[n],
            &xij[n * numPhase * numCom]);
        PassFlashValueDeriv(n);
    }

#ifdef DEBUG
    CheckSat();
#endif // DEBUG
}

void Bulk::FlashDeriv_n()
{
    OCP_FUNCNAME;

    if (comps) {
        FlashDerivCOMP_n();
    } else {
        FlashDerivBLKOIL_n();
    }
}



void Bulk::FlashDerivBLKOIL_n() {}



void Bulk::FlashDerivCOMP_n()
{
    USI         ftype;
    vector<USI> flagB(numPhase, 0);
    NRdSSP          = 0;
    maxNRdSSP       = 0;
    index_maxNRdSSP = 0;

    for (OCP_USI n = 0; n < numBulk; n++) {

        ftype = CalFlashType(n, OCP_TRUE);

        for (USI j = 0; j < numPhase; j++) flagB[j] = phaseExist[n * numPhase + j];

        flashCal[PVTNUM[n]]->FlashDeriv_n(
            P[n], T[n], &Ni[n * numCom], &S[n * numPhase], &xij[n * numPhase * numCom],
            &nj[n * numPhase], ftype, &flagB[0], phaseNum[n]);

        PassFlashValueDeriv_n(n);
    }
}

USI Bulk::CalFlashType(const OCP_USI& n, const OCP_BOOL& fimbulk) const
{  
    if (skipStaAnalyTerm.IfUseSkip()) {

        if (skipStaAnalyTerm.IfSkip(P[n], T[n], Nt[n], &Ni[n * numCom], n, numCom_1)) {
            return 1;
        } 
        else if (phaseNum[n] >= 3 && fimbulk) {
            // if num of hydrocarbon phases is 2 at last NR step and predicted
            // saturations indicates that the stae will be kept, then skip phase
            // stability analysis, carry phase splitting directly
            OCP_USI bId = n * numPhase;
            for (USI j = 0; j < numPhase; j++) {
                if (dSNR[bId + j] + dSNRP[bId + j] < 1E-4) {
                    // phases change (predicted)
                    return 0;
                }
            }
            return 2;
        } 
        else {
            return 0;
        }
    }

    return 0;
}

void Bulk::PassFlashValue(const OCP_USI& n)
{
    OCP_FUNCNAME;

    OCP_USI bIdp   = n * numPhase;
    USI     pvtnum = PVTNUM[n];
    phaseNum[n] = 0;
    for (USI j = 0; j < numPhase; j++) {
        phaseExist[bIdp + j] = flashCal[pvtnum]->phaseExist[j];
        // Important! Saturation must be passed no matter if the phase exists. This is
        // because it will be used to calculate relative permeability and capillary
        // pressure at each time step. Make sure that all saturations are updated at
        // each step!
        S[bIdp + j] = flashCal[pvtnum]->S[j];
        if (phaseExist[bIdp + j]) { // j -> bId + j   fix bugs.
            phaseNum[n]++;
            rho[bIdp + j] = flashCal[pvtnum]->rho[j];
            xi[bIdp + j]  = flashCal[pvtnum]->xi[j];
            for (USI i = 0; i < numCom; i++) {
                xij[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->xij[j * numCom + i];
            }
            mu[bIdp + j] = flashCal[pvtnum]->mu[j];
            vj[bIdp + j] = flashCal[pvtnum]->v[j];
        }
    }
    Nt[n]        = flashCal[pvtnum]->Nt;
    vf[n]        = flashCal[pvtnum]->vf;
    vfp[n]       = flashCal[pvtnum]->vfp;
    OCP_USI bIdc = n * numCom;
    for (USI i = 0; i < numCom; i++) {
        vfi[bIdc + i] = flashCal[pvtnum]->vfi[i];
    }

    PassAdditionInfo(n, pvtnum);
}

void Bulk::PassFlashValueAIMc(const OCP_USI& n)
{
    // only var about volume needs, some flash var also
    OCP_FUNCNAME;

    USI     pvtnum = PVTNUM[n];

    Nt[n]        = flashCal[pvtnum]->Nt;
    vf[n]        = flashCal[pvtnum]->vf;
    vfp[n]       = flashCal[pvtnum]->vfp;
    OCP_USI bIdc = n * numCom;
    for (USI i = 0; i < numCom; i++) {
        vfi[bIdc + i] = flashCal[pvtnum]->vfi[i];
    }

    OCP_USI bIdp = n * numPhase;
    phaseNum[n] = 0;
    for (USI j = 0; j < numPhase; j++) {
        if (flashCal[pvtnum]->phaseExist[j]) {
            phaseNum[n]++;

            // IMPORTANT -- need for next Flash
            // But xij in nonlinear equations has been modified
            for (USI i = 0; i < numCom; i++) {
                xijNR[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->xij[j * numCom + i];
            }
        }
    }

    PassAdditionInfo(n, pvtnum);
}

void Bulk::PassFlashValueDeriv(const OCP_USI& n)
{
    OCP_FUNCNAME;

    OCP_USI bIdp   = n * numPhase;
    USI     pvtnum = PVTNUM[n];  
    USI     len    = 0;
    phaseNum[n] = 0;

    for (USI j = 0; j < numPhase; j++) {
        // Important! Saturation must be passed no matter if the phase exists. This is
        // because it will be used to calculate relative permeability and capillary
        // pressure at each time step. Make sure that all saturations are updated at
        // each step!
        S[bIdp + j]    = flashCal[pvtnum]->S[j];
        dSNR[bIdp + j] = S[bIdp + j] - dSNR[bIdp + j];
        if (phaseExist[bIdp + j]) {
            NRdSSP +=
                (dSNR[bIdp + j] - dSNRP[bIdp + j]) * (dSNR[bIdp + j] - dSNRP[bIdp + j]);
            if (fabs(maxNRdSSP) < fabs(dSNR[bIdp + j] - dSNRP[bIdp + j])) {
                maxNRdSSP       = dSNR[bIdp + j] - dSNRP[bIdp + j];
                index_maxNRdSSP = n;
            }
            // cout << n << "   " << scientific << setprecision(6) << dSNR[bIdp + j] <<
            // "   " << dSNRP[bIdp + j] << endl;
        }

        phaseExist[bIdp + j] = flashCal[pvtnum]->phaseExist[j];
        pSderExist[bIdp + j] = flashCal[pvtnum]->pSderExist[j];
        pVnumCom[bIdp + j]   = flashCal[pvtnum]->pVnumCom[j];
        if (pSderExist[bIdp + j]) len++;
        len += pVnumCom[bIdp + j];
        if (phaseExist[bIdp + j]) { // j -> bId + j fix bugs.
            phaseNum[n]++;
            nj[bIdp + j]  = flashCal[pvtnum]->nj[j];
            rho[bIdp + j] = flashCal[pvtnum]->rho[j];
            xi[bIdp + j]  = flashCal[pvtnum]->xi[j];
            mu[bIdp + j]  = flashCal[pvtnum]->mu[j];
            vj[bIdp + j]  = flashCal[pvtnum]->v[j];

            // Derivatives
            muP[bIdp + j]  = flashCal[pvtnum]->muP[j];
            xiP[bIdp + j]  = flashCal[pvtnum]->xiP[j];
            rhoP[bIdp + j] = flashCal[pvtnum]->rhoP[j];
            for (USI i = 0; i < numCom; i++) {
                xij[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->xij[j * numCom + i];
                mux[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->mux[j * numCom + i];
                xix[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->xix[j * numCom + i];
                rhox[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->rhox[j * numCom + i];
            }
        }
    }
    Nt[n]  = flashCal[pvtnum]->Nt;
    vf[n]  = flashCal[pvtnum]->vf;
    vfp[n] = flashCal[pvtnum]->vfp;

    OCP_USI bIdc = n * numCom;
    for (USI i = 0; i < numCom; i++) {
        vfi[bIdc + i] = flashCal[pvtnum]->vfi[i];
    }

#ifdef OCP_OLD_FIM
    Dcopy(maxLendSdP, &dSec_dPri[n * maxLendSdP], &flashCal[pvtnum]->dXsdXp[0]);
#else
    bRowSizedSdP[n] = len;
    len *= (numCom + 1);
    Dcopy(len, &dSec_dPri[n * maxLendSdP], &flashCal[pvtnum]->dXsdXp[0]);
#endif // OCP_OLD_FIM

    PassAdditionInfo(n, pvtnum);
}

void Bulk::PassFlashValueDeriv_n(const OCP_USI& n)
{
    OCP_FUNCNAME;

    OCP_USI bIdp   = n * numPhase;
    USI     pvtnum = PVTNUM[n];  
    USI     len    = 0;
    phaseNum[n] = 0;

    for (USI j = 0; j < numPhase; j++) {
        // Important! Saturation must be passed no matter if the phase exists. This is
        // because it will be used to calculate relative permeability and capillary
        // pressure at each time step. Make sure that all saturations are updated at
        // each step!
        S[bIdp + j]    = flashCal[pvtnum]->S[j];
        dSNR[bIdp + j] = S[bIdp + j] - dSNR[bIdp + j];
        if (phaseExist[bIdp + j]) {
            NRdSSP +=
                (dSNR[bIdp + j] - dSNRP[bIdp + j]) * (dSNR[bIdp + j] - dSNRP[bIdp + j]);
            if (fabs(maxNRdSSP) < fabs(dSNR[bIdp + j] - dSNRP[bIdp + j])) {
                maxNRdSSP       = dSNR[bIdp + j] - dSNRP[bIdp + j];
                index_maxNRdSSP = n;
            }
        }

        phaseExist[bIdp + j] = flashCal[pvtnum]->phaseExist[j];
        pSderExist[bIdp + j] = flashCal[pvtnum]->pSderExist[j];
        pVnumCom[bIdp + j]   = flashCal[pvtnum]->pVnumCom[j];
        if (pSderExist[bIdp + j]) len++;
        len += pVnumCom[bIdp + j];
        if (phaseExist[bIdp + j]) { // j -> bId + j fix bugs.
            phaseNum[n]++;
            nj[bIdp + j]  = flashCal[pvtnum]->nj[j];
            rho[bIdp + j] = flashCal[pvtnum]->rho[j];
            xi[bIdp + j]  = flashCal[pvtnum]->xi[j];
            mu[bIdp + j]  = flashCal[pvtnum]->mu[j];
            vj[bIdp + j]  = flashCal[pvtnum]->v[j];

            // Derivatives
            muP[bIdp + j]  = flashCal[pvtnum]->muP[j];
            xiP[bIdp + j]  = flashCal[pvtnum]->xiP[j];
            rhoP[bIdp + j] = flashCal[pvtnum]->rhoP[j];
            for (USI i = 0; i < numCom; i++) {
                xij[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->xij[j * numCom + i];
                mux[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->mux[j * numCom + i];
                xix[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->xix[j * numCom + i];
                rhox[bIdp * numCom + j * numCom + i] =
                    flashCal[pvtnum]->rhox[j * numCom + i];
            }
        }
    }
    Nt[n]  = flashCal[pvtnum]->Nt;
    vf[n]  = flashCal[pvtnum]->vf;
    vfp[n] = flashCal[pvtnum]->vfp;

    OCP_USI bIdc = n * numCom;
    for (USI i = 0; i < numCom; i++) {
        vfi[bIdc + i] = flashCal[pvtnum]->vfi[i];
    }

    resIndex[n + 1] = resIndex[n] + len;
    Dcopy(len, &res_n[0] + resIndex[n], &flashCal[pvtnum]->res[0]);
    len *= (numCom + 1);
    Dcopy(len, &dSec_dPri[n * maxLendSdP], &flashCal[pvtnum]->dXsdXp[0]);

    resPc[n] = flashCal[pvtnum]->resPc;

    PassAdditionInfo(n, pvtnum);
}

void Bulk::PassAdditionInfo(const OCP_USI& n, const USI& pvtnum)
{

    if (skipStaAnalyTerm.IfUseSkip()) {
        ePEC[n] = flashCal[pvtnum]->GetErrorPEC();

        SkipStaAnaly& skip = skipStaAnalyTerm;
        skip.flagSkip[n] = flashCal[pvtnum]->GetFlagSkip();
        if (skip.flagSkip[n]) {
            if (flashCal[pvtnum]->GetFtype() == 0) {
                // If recalculate the range then get it
                skip.minEigenSkip[n] = flashCal[pvtnum]->GetMinEigenSkip();
                for (USI j = 0; j < numPhase - 1; j++) {
                    if (flashCal[pvtnum]->phaseExist[j]) {
                        for (USI i = 0; i < numCom_1; i++) {
                            // now hydrocarbon phase = 1, so zi = existing xij
                            skip.ziSkip[n*numCom_1 + i] = flashCal[pvtnum]->xij[j * numCom + i];
                        }
                        break;
                    }
                }
                skip.PSkip[n] = P[n];
                skip.TSkip[n] = T[n];
            }
        }
    }
    if (miscible) {
        surTen[n] = flashCal[pvtnum]->GetSurTen();
    }
}



void Bulk::ResetFlash()
{
    OCP_FUNCNAME;

    phaseExist = lphaseExist;
    S          = lS;
    rho        = lrho;
    xi         = lxi;
    xij        = lxij;
    mu         = lmu;
    vj         = lvj;
    vf         = lvf;
    vfp        = lvfp;
    vfi        = lvfi;
}

/// Relative permeability and capillary pressure
void Bulk::CalKrPc()
{
    OCP_FUNCNAME;

    if (!miscible) {
        OCP_DBL tmp = 0;
        for (OCP_USI n = 0; n < numBulk; n++) {
            OCP_USI bId = n * numPhase;
            flow[SATNUM[n]]->CalKrPc(&S[bId], &kr[bId], &Pc[bId], 0, tmp, tmp);
            for (USI j = 0; j < numPhase; j++)
                Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];
        }
    } else {
        for (OCP_USI n = 0; n < numBulk; n++) {
            OCP_USI bId = n * numPhase;
            flow[SATNUM[n]]->CalKrPc(&S[bId], &kr[bId], &Pc[bId], surTen[n], Fk[n],
                                     Fp[n]);
            for (USI j = 0; j < numPhase; j++)
                Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];
        }
    }

    if (ScalePcow) {
        // correct
        USI Wid = phase2Index[WATER];
        for (USI n = 0; n < numBulk; n++) {
            Pc[n * numPhase + Wid] *= ScaleValuePcow[n];
            Pj[n * numPhase + Wid] = P[n] + Pc[n * numPhase + Wid];
        }
    }
}

void Bulk::CalKrPcDeriv()
{
    OCP_FUNCNAME;

    if (!miscible) {
        OCP_DBL tmp = 0;
        for (OCP_USI n = 0; n < numBulk; n++) {
            OCP_USI bId = n * numPhase;
            flow[SATNUM[n]]->CalKrPcDeriv(&S[bId], &kr[bId], &Pc[bId],
                                          &dKr_dS[bId * numPhase],
                                          &dPcj_dS[bId * numPhase], 0, tmp, tmp);
            for (USI j = 0; j < numPhase; j++) Pj[bId + j] = P[n] + Pc[bId + j];
        }
    } else {
        for (OCP_USI n = 0; n < numBulk; n++) {
            OCP_USI bId = n * numPhase;
            flow[SATNUM[n]]->CalKrPcDeriv(
                &S[bId], &kr[bId], &Pc[bId], &dKr_dS[bId * numPhase],
                &dPcj_dS[bId * numPhase], surTen[n], Fk[n], Fp[n]);
            for (USI j = 0; j < numPhase; j++)
                Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];
        }
    }

    if (ScalePcow) {
        // correct
        USI Wid = phase2Index[WATER];
        for (USI n = 0; n < numBulk; n++) {
            Pc[n * numPhase + Wid] *= ScaleValuePcow[n];
            Pj[n * numPhase + Wid] = P[n] + Pc[n * numPhase + Wid];
        }
    }
}

void Bulk::CalRock()
{
    OCP_FUNCNAME;

    for (OCP_USI n = 0; n < numBulk; n++) {
        rock[ROCKNUM[n]]->CalPoro(P[n], poroInit[n], poro[n], poroP[n]);
        rockVp[n] = rockVntg[n] * poro[n];
    }
}

OCP_DBL Bulk::CalFPR() const
{
    OCP_FUNCNAME;

    OCP_DBL ptmp = 0;
    OCP_DBL vtmp = 0;
    OCP_DBL tmp  = 0;

    if (numPhase == 3) {
        for (OCP_USI n = 0; n < numBulk; n++) {
            tmp = rockVp[n] * (1 - S[n * numPhase + 2]);
            ptmp += P[n] * tmp;
            vtmp += tmp;
        }
    } else if (numPhase < 3) {
        for (OCP_USI n = 0; n < numBulk; n++) {
            tmp = rockVp[n] * (S[n * numPhase]);
            ptmp += P[n] * tmp;
            vtmp += tmp;
        }
    } else {
        OCP_ABORT("Number of phases is out of range!");
    }
    return ptmp / vtmp;
}

void Bulk::CalMaxChange()
{
    OCP_FUNCNAME;

    dPmax       = 0;
    dNmax       = 0;
    dSmax       = 0;
    dVmax       = 0;
    OCP_DBL tmp = 0;
    OCP_USI id;
    OCP_USI ndPmax, ndNmax, ndSmax, ndVmax;

    for (OCP_USI n = 0; n < numBulk; n++) {

        // dP
        tmp = fabs(P[n] - lP[n]);
        if (dPmax < tmp) {
            dPmax  = tmp;
            ndPmax = n;
        }

        // dS
        for (USI j = 0; j < numPhase; j++) {
            id  = n * numPhase + j;
            tmp = fabs(S[id] - lS[id]);
            if (dSmax < tmp) {
                dSmax  = tmp;
                ndSmax = n;
            }
        }

        // dN
        for (USI i = 0; i < numCom; i++) {
            id = n * numCom + i;

            tmp = fabs(max(Ni[id], lNi[id]));
            if (tmp > TINY) {
                tmp = fabs(Ni[id] - lNi[id]) / tmp;
                if (dNmax < tmp) {
                    dNmax  = tmp;
                    ndNmax = n;
                }
            }
        }

        tmp = fabs(vf[n] - rockVp[n]) / rockVp[n];
        if (dVmax < tmp) {
            dVmax  = tmp;
            ndVmax = n;
        }
    }

    if (0) {
        cout << scientific << setprecision(6);
        cout << setw(15) << dPmax << setw(15) << dNmax << setw(15) << dSmax << setw(15)
             << dVmax << endl;
        cout << setw(15) << ndPmax << setw(15) << ndNmax << setw(15) << ndSmax
             << setw(15) << ndVmax << endl;

        cout << ndSmax << endl;
        cout << "old   " << lP[ndSmax] << endl;
        tmp = Dnorm1(numCom_1, &lNi[ndSmax * numCom]);
        for (USI i = 0; i < numCom_1; i++) {
            cout << lNi[ndSmax * numCom + i] << "   ";
        }
        cout << endl;
        for (USI i = 0; i < numCom_1; i++) {
            cout << lNi[ndSmax * numCom + i] / tmp << "   ";
        }
        cout << endl;
        for (USI j = 0; j < numPhase - 1; j++) {
            cout << lS[ndSmax * numPhase + j] << "   ";
            for (USI i = 0; i < numCom_1; i++) {
                cout << lxij[(ndSmax * numPhase + j) * numCom + i] << "   ";
            }
            cout << endl;
        }
        cout << "new   " << P[ndSmax] << endl;
        tmp = Dnorm1(numCom_1, &Ni[ndSmax * numCom]);
        for (USI i = 0; i < numCom_1; i++) {
            cout << Ni[ndSmax * numCom + i] << "   ";
        }
        cout << endl;
        for (USI i = 0; i < numCom_1; i++) {
            cout << Ni[ndSmax * numCom + i] / tmp << "   ";
        }
        cout << endl;
        for (USI j = 0; j < numPhase - 1; j++) {
            cout << S[ndSmax * numPhase + j] << "   ";
            for (USI i = 0; i < numCom_1; i++) {
                cout << xij[(ndSmax * numPhase + j) * numCom + i] << "   ";
            }
            cout << endl;
        }
    }
}

/// Return OCP_TRUE if no negative pressure and OCP_FALSE otherwise.
OCP_BOOL Bulk::CheckP() const
{
    OCP_FUNCNAME;

    for (OCP_USI n = 0; n < numBulk; n++) {
        if (P[n] < 0) {
            std::ostringstream PStringSci;
            PStringSci << std::scientific << P[n];
            OCP_WARNING("Negative pressure: P[" + std::to_string(n) +
                        "] = " + PStringSci.str());
            cout << "P = " << P[n] << endl;
            return OCP_FALSE;
        }
    }

    return OCP_TRUE;
}

/// Return OCP_TRUE if no negative Ni and OCP_FALSE otherwise.
OCP_BOOL Bulk::CheckNi()
{
    OCP_FUNCNAME;

    OCP_USI len = numBulk * numCom;
    for (OCP_USI n = 0; n < len; n++) {
        if (Ni[n] < 0) {
            OCP_USI bId = n / numCom;
            if (Ni[n] > -1E-3 * Nt[bId] && OCP_FALSE) {
                Ni[n] = 1E-8 * Nt[bId];
                // Ni[n] = 0;
            } else {
                USI                cId = n - bId * numCom;
                std::ostringstream NiStringSci;
                NiStringSci << std::scientific << Ni[n];
                OCP_WARNING("Negative Ni: Ni[" + std::to_string(cId) + "] in Bulk[" +
                            std::to_string(bId) + "] = " + NiStringSci.str() + ",  " +
                            "dNi = " + std::to_string(dNNR[n]));
#ifdef DEBUG
                for (USI i = 0; i < numCom; i++) {
                    cout << Ni[bId * numCom + i] << "   ";
                }
                cout << endl;
                for (USI j = 0; j < numPhase - 1; j++) {
                    cout << setprecision(9);
                    if (phaseExist[bId * numPhase + j]) {
                        cout << j << "   ";
                        for (USI i = 0; i < numCom_1; i++) {
                            cout << xij[bId * numPhase * numCom + j * numCom + i]
                                 << "   ";
                        }
                        cout << endl;
                    }
                }
#endif // DEBUG
                return OCP_FALSE;
            }
        }
    }
    return OCP_TRUE;
}

/// Return OCP_TRUE if all Ve < Vlim and OCP_FALSE otherwise.
OCP_BOOL Bulk::CheckVe(const OCP_DBL& Vlim) const
{
    OCP_FUNCNAME;

    // OCP_BOOL tmpflag = OCP_TRUE;

    OCP_DBL dVe = 0.0;
    for (OCP_USI n = 0; n < numBulk; n++) {
        dVe = fabs(vf[n] - rockVp[n]) / rockVp[n];
        if (dVe > Vlim) {
            cout << "Volume error at Bulk[" << n << "] = " << setprecision(6) << dVe
                 << " is too big!" << endl;
            // OutputInfo(n);
            return OCP_FALSE;
            // tmpflag = OCP_FALSE;
        }
    }
    // OutputInfo(39);
    // if (!tmpflag) return OCP_FALSE;
    return OCP_TRUE;
}

void Bulk::CheckDiff()
{
    OCP_FUNCNAME;

    OCP_DBL tmp;
    OCP_USI id;
    for (OCP_USI n = 0; n < numBulk; n++) {
        for (USI j = 0; j < numPhase; j++) {
            id  = n * numPhase + j;
            tmp = fabs(phaseExist[id] - lphaseExist[id]);
            if (tmp >= 1E-10) {
                cout << "Difference in phaseExist\t" << tmp << "\n";
            }
            if (lphaseExist[id] || phaseExist[id]) {
                tmp = fabs(S[id] - lS[id]);
                if (tmp >= 1E-10) {
                    cout << scientific << setprecision(16);
                    cout << "Bulk[" << n << "]" << endl;
                    cout << "Saturation" << endl;
                    for (USI j = 0; j < numPhase; j++) {
                        cout << S[n * numPhase + j] << "   " << lS[n * numPhase + j]
                             << endl;
                    }
                    cout << "Pressure" << endl;
                    cout << P[n] << "   " << lP[n] << endl;
                    cout << "Ni" << endl;
                    for (USI i = 0; i < numCom; i++) {
                        cout << Ni[n * numCom + i] << "   " << lNi[n * numCom + i]
                             << endl;
                    }

                    const SkipStaAnaly& skip = skipStaAnalyTerm;
                    const SkipStaAnaly& lskip = lskipStaAnalyTerm;
                    cout << "PhaseNum" << endl;
                    cout << phaseNum[n] << "   " << lphaseNum[n] << endl;
                    cout << "minEigenSkip" << endl;
                    cout << skip.minEigenSkip[n] << "   " << lskip.minEigenSkip[n] << endl;
                    cout << "flagSkip" << endl;
                    cout << skip.flagSkip[n] << "   " << lskip.flagSkip[n] << endl;
                    cout << "PSkip" << endl;
                    cout << skip.PSkip[n] << "   " << lskip.PSkip[n] << endl;
                    cout << "TSkip" << endl;
                    cout << skip.TSkip[n] << "   " << lskip.TSkip[n] << endl;
                    cout << "ziSkip" << endl;
                    for (USI i = 0; i < numCom_1; i++) {
                        cout << skip.ziSkip[n*numCom_1 + i] << "   "
                             << lskip.ziSkip[n * numCom_1 + i] << endl;
                    }

                    cout << "Difference in S\t" << tmp << "  " << phaseExist[id]
                         << "\n";
                }
                tmp = fabs(xi[id] - lxi[id]);
                if (tmp >= 1E-10) {
                    cout << "Difference in Xi\t" << tmp << "  " << phaseExist[id]
                         << "\n";
                }
                tmp = fabs(rho[id] - lrho[id]);
                if (tmp >= 1E-10) {
                    cout << "Difference in rho\t" << tmp << "  " << phaseExist[id]
                         << "\n";
                }
            }
        }
    }
}

void Bulk::CheckSat() const
{
    OCP_FUNCNAME;

    OCP_DBL tmp;
    for (OCP_USI n = 0; n < numBulk; n++) {
        tmp = 0.0;
        for (USI j = 0; j < numPhase; j++) {
            if (phaseExist[n * numPhase + j]) {
                if (S[n * numPhase + j] < 0) {
                    OCP_ABORT("Negative volume!");
                }
                tmp += S[n * numPhase + j];
            }
        }
        if (fabs(tmp - 1) > TINY) {
            OCP_ABORT("Saturation is greater than 1!");
        }
    }
}

USI Bulk::GetMixMode() const
{
    OCP_FUNCNAME;

    return useEoS;

}

void Bulk::CalSomeInfo(const Grid& myGrid) const
{
    // test
    OCP_DBL   depthMax = 0;
    OCP_USI   ndepa    = 0;
    OCP_DBL   depthMin = 1E8;
    OCP_USI   ndepi    = 0;
    OCP_DBL   dxMax    = 0;
    OCP_USI   nxa      = 0;
    OCP_DBL   dxMin    = 1E8;
    OCP_USI   nxi      = 0;
    OCP_DBL   dyMax    = 0;
    OCP_USI   nya      = 0;
    OCP_DBL   dyMin    = 1E8;
    OCP_USI   nyi      = 0;
    OCP_DBL   dzMax    = 0;
    OCP_USI   nza      = 0;
    OCP_DBL   dzMin    = 1E8;
    OCP_USI   nzi      = 0;
    OCP_DBL   RVMax    = 0;
    OCP_USI   nRVa     = 0;
    OCP_DBL   RVMin    = 1E8;
    OCP_USI   nRVi     = 0;
    OCP_DBL   RVPMax   = 0;
    OCP_USI   nRVPa    = 0;
    OCP_DBL   RVPMin   = 1E8;
    OCP_USI   nRVPi    = 0;
    OCP_DBL   PerxMax  = 0;
    OCP_USI   nPerxa   = 0;
    OCP_DBL   PerxMin  = 1E8;
    OCP_USI   nPerxi   = 0;
    USI       I, J, K;
    const USI sp = myGrid.GetNumDigitIJK();
    for (OCP_USI n = 0; n < numBulk; n++) {
        // if (!activeMap_G2B[nn].IsAct())
        //     continue;
        // OCP_USI n = activeMap_G2B[nn].GetId();
        if (depthMax < depth[n]) {
            depthMax = depth[n];
            ndepa    = n;
        }
        if (depthMin > depth[n]) {
            depthMin = depth[n];
            ndepi    = n;
        }
        if (dxMax < myGrid.Dx(n)) {
            dxMax = myGrid.Dx(n);
            nxa   = n;
        }
        if (dxMin > myGrid.Dx(n)) {
            dxMin = myGrid.Dx(n);
            nxi   = n;
        }
        if (dyMax < myGrid.Dy(n)) {
            dyMax = myGrid.Dy(n);
            nya   = n;
        }
        if (dyMin > myGrid.Dy(n)) {
            dyMin = myGrid.Dy(n);
            nyi   = n;
        }
        if (dzMax < myGrid.Dz(n)) {
            dzMax = myGrid.Dz(n);
            nza   = n;
        }
        if (dzMin > myGrid.Dz(n)) {
            dzMin = myGrid.Dz(n);
            nzi   = n;
        }
        OCP_DBL tmp = myGrid.v[myGrid.activeMap_B2G[n]];
        if (RVMax < tmp) {
            RVMax = tmp;
            nRVa  = n;
        }
        if (RVMin > tmp) {
            RVMin = tmp;
            nRVi  = n;
        }
        tmp = rockVp[n];
        if (RVPMax < tmp) {
            RVPMax = tmp;
            nRVPa  = n;
        }
        if (RVPMin > tmp) {
            RVPMin = tmp;
            nRVPi  = n;
        }
        tmp = rockKx[n];
        if (PerxMax < tmp) {
            PerxMax = tmp;
            nPerxa  = n;
        }
        if (PerxMin > tmp && fabs(tmp) > 1E-8) {
            PerxMin = tmp;
            nPerxi  = n;
        }
    }

    cout << "---------------------" << endl
         << "BULK" << endl
         << "---------------------" << endl;
    myGrid.GetIJKGrid(I, J, K, ndepa);
    cout << "  Depthmax = " << depthMax << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, ndepi);
    cout << "  Depthmin = " << depthMin << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nxa);
    cout << "  DXmax    = " << dxMax << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nxi);
    cout << "  DXmin    = " << dxMin << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nya);
    cout << "  DYmax    = " << dyMax << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nyi);
    cout << "  DYmin    = " << dyMin << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nza);
    cout << "  DZmax    = " << dzMax << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nzi);
    cout << "  DZmin    = " << dzMin << " feet "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nRVa);
    cout << "  RVmax    = " << RVMax / CONV1 << " rb "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nRVi);
    cout << "  RVmin    = " << RVMin / CONV1 << " rb "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nRVPa);
    cout << "  RVmax    = " << RVPMax / CONV1 << " rb "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nRVPi);
    cout << "  RVmin    = " << RVPMin / CONV1 << " rb "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nPerxa);
    cout << "  Perxmax  = " << PerxMax << "   "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
    myGrid.GetIJKGrid(I, J, K, nPerxi);
    cout << "  Perxmin  = " << scientific << PerxMin << "   "
         << GetIJKformat(to_string(I), to_string(J), to_string(K), sp) << endl;
}

/////////////////////////////////////////////////////////////////////
// IMPEC
/////////////////////////////////////////////////////////////////////

void Bulk::AllocateAuxIMPEC()
{
    OCP_FUNCNAME;

    // IMPEC var
    poroP.resize(numBulk, 0);
    vfi.resize(numBulk * numCom);
    vfp.resize(numBulk);
    cfl.resize(numBulk * numPhase);

    // Last Step
    lP.resize(numBulk);
    lPj.resize(numBulk * numPhase);
    lPc.resize(numBulk * numPhase);
    lphaseExist.resize(numBulk * numPhase);
    lS.resize(numBulk * numPhase);
    lrho.resize(numBulk * numPhase);
    lxi.resize(numBulk * numPhase);
    lxij.resize(numBulk * numPhase * numCom);
    lNi.resize(numBulk * numCom);
    lmu.resize(numBulk * numPhase);
    lkr.resize(numBulk * numPhase);
    lvj.resize(numBulk * numPhase);
    lvf.resize(numBulk);
    lNt.resize(numBulk);
    lvfi.resize(numBulk * numCom);
    lvfp.resize(numBulk);
    lrockVp.resize(numBulk);
    lporo.resize(numBulk);
    lporoP.resize(numBulk);
}

void Bulk::GetSolIMPEC(const vector<OCP_DBL>& u)
{
    OCP_FUNCNAME;

    for (OCP_USI n = 0; n < numBulk; n++) {
        P[n] = u[n];
        for (USI j = 0; j < numPhase; j++) {
            OCP_USI id = n * numPhase + j;
            if (phaseExist[id]) Pj[id] = P[n] + Pc[id];
        }
    }
}

OCP_DBL Bulk::CalCFL() const
{
    OCP_FUNCNAME;

    OCP_DBL       tmp = 0;
    const OCP_USI len = numBulk * numPhase;
    for (OCP_USI n = 0; n < len; n++) {
        if (phaseExist[n]) {
            cfl[n] /= vj[n];
#ifdef DEBUG
            if (!isfinite(cfl[n])) {
                OCP_ABORT("cfl is nan!");
            }
#endif // DEBUG
            if (tmp < cfl[n]) tmp = cfl[n];
        }
    }
    return tmp;
}

void Bulk::UpdateLastStepIMPEC()
{
    OCP_FUNCNAME;
    lphaseNum     = phaseNum;
    lskipStaAnalyTerm = skipStaAnalyTerm;

    lP          = P;
    lPj         = Pj;
    lPc         = Pc;
    lphaseExist = phaseExist;
    lS          = S;
    lrho        = rho;
    lxi         = xi;
    lxij        = xij;
    lNi         = Ni;
    lmu         = mu;
    lkr         = kr;
    lvj         = vj;
    lvf         = vf;
    lvfi        = vfi;
    lvfp        = vfp;
    lporo       = poro;
    lporoP      = poroP; 
    lrockVp     = rockVp;
    lNt         = Nt;

    if (miscible) {
        lsurTen = surTen;
    }
}

/////////////////////////////////////////////////////////////////////
// FIM
/////////////////////////////////////////////////////////////////////

void Bulk::AllocateAuxFIM()
{
    OCP_FUNCNAME;

    // FIM var
    poroP.resize(numBulk);
    nj.resize(numBulk * numPhase);
    vfi.resize(numBulk * numCom);
    vfp.resize(numBulk);
    muP.resize(numBulk * numPhase);
    xiP.resize(numBulk * numPhase);
    rhoP.resize(numBulk * numPhase);
    mux.resize(numBulk * numCom * numPhase);
    xix.resize(numBulk * numCom * numPhase);
    rhox.resize(numBulk * numCom * numPhase);
    maxLendSdP = (numCom + 1) * (numCom + 1) * numPhase;
    dSec_dPri.resize(numBulk * maxLendSdP);
    bRowSizedSdP.resize(numBulk);   
    pSderExist.resize(numBulk * numPhase);
    pVnumCom.resize(numBulk * numPhase);
    dKr_dS.resize(numBulk * numPhase * numPhase);
    dPcj_dS.resize(numBulk * numPhase * numPhase);

    // Last Step
    lP.resize(numBulk);
    lPj.resize(numBulk * numPhase);
    lPc.resize(numBulk * numPhase);
    lphaseExist.resize(numBulk * numPhase);
    lS.resize(numBulk * numPhase);
    lnj.resize(numBulk * numPhase);
    lrho.resize(numBulk * numPhase);
    lxi.resize(numBulk * numPhase);
    lxij.resize(numBulk * numPhase * numCom);
    lNi.resize(numBulk * numCom);
    lmu.resize(numBulk * numPhase);
    lkr.resize(numBulk * numPhase);
    lvj.resize(numBulk * numPhase);
    lvf.resize(numBulk);
    lNt.resize(numBulk);
    lvfi.resize(numBulk * numCom);
    lvfp.resize(numBulk);
    lrockVp.resize(numBulk);
    lporo.resize(numBulk);
    lporoP.resize(numBulk);
    lmuP.resize(numBulk * numPhase);
    lxiP.resize(numBulk * numPhase);
    lrhoP.resize(numBulk * numPhase);
    lmux.resize(numBulk * numCom * numPhase);
    lxix.resize(numBulk * numCom * numPhase);
    lrhox.resize(numBulk * numCom * numPhase);
    ldSec_dPri.resize(numBulk * maxLendSdP);
    lbRowSizedSdP.resize(numBulk);   
    lpSderExist.resize(numBulk * numPhase);
    lpVnumCom.resize(numBulk * numPhase);
    ldKr_dS.resize(numBulk * numPhase * numPhase);
    ldPcj_dS.resize(numBulk * numPhase * numPhase);

    dSNR.resize(numBulk * numPhase);
    dSNRP.resize(numBulk * numPhase);
    dNNR.resize(numBulk * numCom);
    dPNR.resize(numBulk);

    NRstep.resize(numBulk);
}


void Bulk::AllocateAuxFIMn()
{
    AllocateAuxFIM();

    res_n.resize((numPhase + numPhase * numCom) * numBulk);
    resPc.resize(numBulk);
    resIndex.resize(numBulk + 1, 0);

    lres_n.resize((numPhase + numPhase * numCom) * numBulk);
    lresPc.resize(numBulk);
    lresIndex.resize(numBulk + 1, 0);
}


void Bulk::GetSolFIM(const vector<OCP_DBL>& u,
                     const OCP_DBL&         dPmaxlim,
                     const OCP_DBL&         dSmaxlim)
{
    OCP_FUNCNAME;

    OCP_DBL         dP;
    USI             row = numPhase * (numCom + 1);
    USI             col = numCom + 1;
    OCP_USI         n_np_j;
    vector<OCP_DBL> dtmp(row, 0);
    OCP_DBL         chopmin = 1;
    OCP_DBL         choptmp = 0;

    dSNR       = S;
    NRphaseNum = phaseNum;
    NRdSmaxP   = 0;
    NRdPmax    = 0;
    NRdNmax    = 0;

    for (OCP_USI n = 0; n < numBulk; n++) {
        // const vector<OCP_DBL>& scm = satcm[SATNUM[n]];

        chopmin = 1;
        // compute the chop
        fill(dtmp.begin(), dtmp.end(), 0.0);

#ifdef OCP_OLD_FIM
        DaAxpby(numPhase, col, 1, &dSec_dPri[n * maxLendSdP], u.data() + n * col, 1,
                dtmp.data());
        const OCP_BOOL newFIM = OCP_FALSE;
#else
        DaAxpby(bRowSizedSdP[n], col, 1, &dSec_dPri[n * maxLendSdP], u.data() + n * col,
                1, dtmp.data());
        const OCP_BOOL newFIM = OCP_TRUE;
#endif // OCP_OLD_FIM

        USI js = 0;
        for (USI j = 0; j < numPhase; j++) {
            if (!pSderExist[n * numPhase + j] && newFIM) {
                continue;
            }
            n_np_j = n * numPhase + j;

            choptmp = 1;
            if (fabs(dtmp[js]) > dSmaxlim) {
                choptmp = dSmaxlim / fabs(dtmp[js]);
            } else if (S[n_np_j] + dtmp[js] < 0.0) {
                choptmp = 0.9 * S[n_np_j] / fabs(dtmp[js]);
            }

            // if (fabs(S[n_np_j] - scm[j]) > TINY &&
            //     (S[n_np_j] - scm[j]) / (choptmp * dtmp[js]) < 0)
            //     choptmp *= min(1.0, -((S[n_np_j] - scm[j]) / (choptmp * dtmp[js])));

            chopmin = min(chopmin, choptmp);
            js++;
        }

        // dS
        js = 0;
        for (USI j = 0; j < numPhase; j++) {
            if (!pSderExist[n * numPhase + j] && newFIM) {
                dSNRP[n * numPhase + j] = 0;
                continue;
            }
            dSNRP[n * numPhase + j] = chopmin * dtmp[js];
            if (fabs(NRdSmaxP) < fabs(dSNRP[n * numPhase + j]))
                NRdSmaxP = dSNRP[n * numPhase + j];
            js++;
        }

        // dxij   ---- Compositional model only
        if (useEoS) {
            if (phaseNum[n] >= 3) {
                // num of Hydroncarbon phase >= 2
                OCP_USI  bId     = 0;
                for (USI j = 0; j < 2; j++) {
                    bId = n * numPhase * numCom + j * numCom;
                    for (USI i = 0; i < numCom_1; i++) {
                        xij[bId + i] += chopmin * dtmp[js];
                        js++;
                    }
                }
            }
        }

        // dP
        dP = u[n * col];
        // choptmp = dPmaxlim / fabs(dP);
        // chopmin = min(chopmin, choptmp);
        if (fabs(NRdPmax) < fabs(dP)) NRdPmax = dP;
        P[n] += dP; // seems better
        dPNR[n] = dP;

        // dNi
        NRstep[n] = chopmin;
        for (USI i = 0; i < numCom; i++) {
            dNNR[n * numCom + i] = u[n * col + 1 + i] * chopmin;
            if (fabs(NRdNmax) < fabs(dNNR[n * numCom + i]) / Nt[n])
                NRdNmax = dNNR[n * numCom + i] / Nt[n];

            Ni[n * numCom + i] += dNNR[n * numCom + i];
        }
        // cout << scientific << setprecision(6) << dP << "   " << n << endl;
    }
}

void Bulk::GetSolFIM_n(const vector<OCP_DBL>& u,
                       const OCP_DBL&         dPmaxlim,
                       const OCP_DBL&         dSmaxlim)
{
    // For saturations changes:
    // 1. maximum changes must be less than dSmaxlim,
    // 2. if phases become mobile/immobile, then set it to crtical point,
    // 3. After saturations are determined, then scale the nij to conserve Volume
    // equations.

    OCP_FUNCNAME;

    dSNR       = S;
    NRphaseNum = phaseNum;
    fill(dSNRP.begin(), dSNRP.end(), 0.0);

    NRdSmaxP = 0;
    NRdPmax  = 0;
    NRdNmax  = 0;

    USI row  = numPhase * (numCom + 1);
    USI ncol = numCom + 1;
    USI n_np_j;

    vector<OCP_DBL> dtmp(row, 0);
    vector<OCP_DBL> tmpNij(numPhase * numCom, 0);

    OCP_DBL dSmax;
    OCP_DBL dP;
    OCP_DBL chop = 1;

    // vector<OCP_DBL> tmpxi(numPhase, 0);
    // vector<OCP_DBL> tmpVf(numPhase, 0);

    for (OCP_USI n = 0; n < numBulk; n++) {
        const vector<OCP_DBL>& scm = satcm[SATNUM[n]];

        dP      = u[n * ncol];
        dPNR[n] = dP;
        P[n] += dP; // seems better
        if (fabs(NRdPmax) < fabs(dP)) NRdPmax = dP;

        // rockVp[n] = rockVntg[n] * (1 + rockC1 * (P[n] - rockPref));
        // cout << scientific << setprecision(6) << dP << "   " << n << endl;

        dSmax = 0;
        chop  = 1;

        const USI cNp = phaseNum[n];
        const USI len = resIndex[n + 1] - resIndex[n];
        Dcopy(len, &dtmp[0], &res_n[resIndex[n]]);
        DaAxpby(len, ncol, 1.0, &dSec_dPri[n * maxLendSdP], u.data() + n * ncol, 1.0,
                dtmp.data());

        // Calculate dSmax
        USI js = 0;
        for (USI j = 0; j < numPhase; j++) {
            n_np_j = n * numPhase + j;
            if (pSderExist[n_np_j]) {
                dSmax = max(fabs(dtmp[js]), dSmax);
                js++;
            }
        }

        // Calculate chop
        if (dSmax > dSmaxlim) {
            chop  = min(chop, dSmaxlim / dSmax);
            dSmax = dSmaxlim;
        }
        js = 0;
        for (USI j = 0; j < numPhase; j++) {
            n_np_j = n * numPhase + j;
            if (pSderExist[n_np_j]) {
                if (fabs(S[n_np_j] - scm[j]) > TINY &&
                    (S[n_np_j] - scm[j]) / (chop * dtmp[js]) < 0)
                    chop *= min(1.0, -((S[n_np_j] - scm[j]) / (chop * dtmp[js])));
                if (S[n_np_j] + chop * dtmp[js] < 0)
                    chop *= min(1.0, -S[n_np_j] / (chop * dtmp[js]));
                js++;
            }
        }

        // Calculate S, phaseExist, xij, nj
        fill(tmpNij.begin(), tmpNij.end(), 0.0);
        // fill(Ni.begin() + n * numCom, Ni.begin() + n * numCom + numCom, 0.0);

        js     = 0;
        USI jx = cNp;
        for (USI j = 0; j < numPhase; j++) {
            n_np_j = n * numPhase + j;
            if (pSderExist[n_np_j]) {
                dSNRP[n_np_j] = chop * dtmp[js];
                if (fabs(NRdSmaxP) < fabs(dSNRP[n_np_j])) NRdSmaxP = dSNRP[n_np_j];
                js++;

                // S[n_np_j] += chop * dtmp[js];
                // if (S[n_np_j] < TINY) {
                //     pSderExist[n_np_j] = OCP_FALSE;
                // }
                // js++;
                Daxpy(numCom, nj[n_np_j], &xij[n_np_j * numCom], &tmpNij[j * numCom]);
                for (USI i = 0; i < pVnumCom[j]; i++) {
                    tmpNij[j * numCom + i] += chop * dtmp[jx + i];
                }
                jx += pVnumCom[j];
                nj[n_np_j] = Dnorm1(numCom, &tmpNij[j * numCom]);
                for (USI i = 0; i < numCom; i++) {
                    xij[n_np_j * numCom + i] = tmpNij[j * numCom + i] / nj[n_np_j];
                    // Ni[n * numCom + i] += tmpNij[j * numCom + i];
                }
            }
        }

        // Vf correction
        /*        OCP_DBL dVmin = 100 * rockVp[n];
                USI index = 0;
                for (USI j = 0; j < numPhase; j++) {
                    if (phaseExist[n * numPhase + j]) {
                        tmpxi[j] = flashCal[PVTNUM[n]]->XiPhase(P[n], T, &xij[(n *
           numPhase + j) * numCom]); tmpVf[j] = nj[n * numPhase + j] / (S[n * numPhase +
           j] * tmpxi[j]); if (fabs(tmpVf[j] - rockVp[n]) < dVmin) { dVmin =
           fabs(tmpVf[j] - rockVp[n]); index = j;
                        }
                    }
                } */
        // for (USI j = 0; j < numPhase; j++) {
        //     if (phaseExist[n * numPhase + j]) {
        //         nj[n * numPhase + j] *= (tmpVf[index] / tmpVf[j]);
        //         for (USI i = 0; i < numCom; i++) {
        //             Ni[n * numCom + i] += nj[n * numPhase + j] * xij[(n * numPhase +
        //             j) * numCom + i];
        //         }
        //     }
        // }

        NRstep[n] = chop;
        for (USI i = 0; i < numCom; i++) {
            dNNR[n * numCom + i] = u[n * ncol + 1 + i] * chop;
            if (fabs(NRdNmax) < fabs(dNNR[n * numCom + i]) / Nt[n])
                NRdNmax = dNNR[n * numCom + i] / Nt[n];
            Ni[n * numCom + i] += dNNR[n * numCom + i];
        }
    }
}

void Bulk::CalRelResFIM(OCPRes& resFIM) const
{
    OCP_FUNCNAME;

    OCP_DBL tmp;

    const USI len = numCom + 1;
    for (OCP_USI n = 0; n < numBulk; n++) {

        eV[n] = 0;
        for (USI i = 0; i < len; i++) {
            tmp = fabs(resFIM.res[n * len + i] / rockVp[n]);
            if (resFIM.maxRelRes_v < tmp) {
                resFIM.maxRelRes_v = tmp;
                resFIM.maxId_v     = n;
            }
            eV[n] += tmp * tmp;
        }
        eV[n] = sqrt(eV[n]);

        eN[n] = 0;
        for (USI i = 1; i < len; i++) {
            tmp = fabs(resFIM.res[n * len + i] / Nt[n]);
            if (resFIM.maxRelRes_mol < tmp) {
                resFIM.maxRelRes_mol = tmp;
                resFIM.maxId_mol     = n;
            }
            eN[n] += tmp * tmp;
        }
        eN[n] = sqrt(eN[n]);
    }
}

void Bulk::ShowRes(const vector<OCP_DBL>& res) const
{
    OCP_USI bId;

    const USI len = numCom + 1;
    for (OCP_USI n = 0; n < numBulk; n++) {
        bId = n * len;
        cout << endl;
        cout << "Bulk[" << setw(3) << n << "]   ";
        cout << "Vp  " << rockVp[n] << "   ";
        cout << "Nt  " << Nt[n] << "   ";
        cout << "NRstep  " << NRstep[n] << "   ";
        cout << "P   " << P[n] << "   ";
        cout << dPNR[n] << "   " << dPNR[n] / P[n] << "   ";
        cout << (P[n] - lP[n]) / P[n] << "   ";
        cout << "PhaseNum   " << phaseNum[n] << "   ";
        cout << endl;
        for (USI i = 0; i < len; i++) {

            cout << "[" << setw(2) << i << "]"
                 << "   ";
            cout << -res[bId + i] << "   ";
            cout << fabs(res[bId + i] / rockVp[n]) << "   ";
            if (i > 0) {
                cout << fabs(res[bId + i] / Nt[n]) << "   ";
                cout << Ni[n * numCom + i - 1] << "   " << lNi[n * numCom + i - 1]
                     << "   ";
                cout << dNNR[n * numCom + i - 1] << "   "
                     << dNNR[n * numCom + i - 1] /
                            (Ni[n * numCom + i - 1] - dNNR[n * numCom + i - 1])
                     << "   ";
                cout << (Ni[n * numCom + i - 1] - lNi[n * numCom + i - 1]) /
                            lNi[n * numCom + i - 1]
                     << "   ";
                // cout << vfi[n * numCom + i] << "   ";
            } else {
                cout << rockVp[n] - vf[n] << "   ";
            }

            cout << endl;
        }
    }
}

void Bulk::ResetFIM()
{
    OCP_FUNCNAME;

    phaseNum     = lphaseNum;
    skipStaAnalyTerm = lskipStaAnalyTerm;

    P            = lP;
    Pj           = lPj;
    Pc           = lPc;
    phaseExist   = lphaseExist;
    S            = lS;
    nj           = lnj;
    rho          = lrho;
    xi           = lxi;
    xij          = lxij;
    Ni           = lNi;
    mu           = lmu;
    kr           = lkr;
    vj           = lvj;
    vf           = lvf;
    Nt           = lNt;
    vfi          = lvfi;
    vfp          = lvfp;
    poro         = lporo;
    poroP        = lporoP;
    rockVp       = lrockVp;
    muP          = lmuP;
    xiP          = lxiP;
    rhoP         = lrhoP;
    mux          = lmux;
    xix          = lxix;
    rhox         = lrhox;
    dSec_dPri    = ldSec_dPri;
    bRowSizedSdP = lbRowSizedSdP;  
    pSderExist   = lpSderExist;
    pVnumCom     = lpVnumCom;
    dKr_dS       = ldKr_dS;
    dPcj_dS      = ldPcj_dS;
    if (miscible) {
        surTen = lsurTen;
    }
}

void Bulk::ResetFIMn()
{
    ResetFIM();
    res_n = lres_n;
    resPc = lresPc;
    resIndex = lresIndex;
}

void Bulk::UpdateLastStepFIM()
{
    OCP_FUNCNAME;
    lphaseNum     = phaseNum;
    lskipStaAnalyTerm = skipStaAnalyTerm;

    lP            = P;
    lPj           = Pj;
    lPc           = Pc;
    lphaseExist   = phaseExist;
    lS            = S;
    lnj           = nj;
    lrho          = rho;
    lxi           = xi;
    lxij          = xij;
    lNi           = Ni;
    lmu           = mu;
    lkr           = kr;
    lvj           = vj;
    lvf           = vf;
    lNt           = Nt;
    lvfi          = vfi;
    lvfp          = vfp;
    lporo         = poro;
    lporoP        = poroP; 
    lrockVp       = rockVp;
    lmuP          = muP;
    lxiP          = xiP;
    lrhoP         = rhoP;
    lmux          = mux;
    lxix          = xix;
    lrhox         = rhox;
    ldSec_dPri    = dSec_dPri;
    lbRowSizedSdP = bRowSizedSdP;
    lpSderExist   = pSderExist;
    lpVnumCom     = pVnumCom;
    ldKr_dS       = dKr_dS;
    ldPcj_dS      = dPcj_dS;

    if (miscible) {
        lsurTen = surTen;
    }
}


void Bulk::UpdateLastStepFIMn()
{
    UpdateLastStepFIM();
    lres_n = res_n;
    lresPc = resPc;
    lresIndex = resIndex;
}


OCP_DBL Bulk::CalNRdSmax(OCP_USI& index)
{
    NRdSmax     = 0;
    OCP_USI len = numBulk * numPhase;
    for (USI n = 0; n < len; n++) {
        if (fabs(NRdSmax) < fabs(dSNR[n])) {
            NRdSmax = dSNR[n];
            index   = n;
        }
    }
    index /= numPhase;
    return NRdSmax;
}

OCP_DBL Bulk::GetNRdPmax() { return NRdPmax; }

OCP_DBL Bulk::GetNRdSmaxP() { return NRdSmaxP; }

OCP_DBL Bulk::GetNRdNmax() { return NRdNmax; }

void Bulk::CorrectNi(const vector<OCP_DBL>& res)
{
    for (OCP_USI n = 0; n < numBulk; n++) {
        for (USI i = 0; i < numCom; i++) {
            Ni[n * numCom + i] += res[n * (numCom + 1) + i];
            if (Ni[n * numCom + i] < 0) {
                Ni[n * numCom + i] = 1E-8 * Nt[n];
            }
        }
    }
}

void Bulk::OutputInfo(const OCP_USI& n) const
{
    OCP_USI bIdC  = n * numCom;
    OCP_USI bIdP  = n * numPhase;
    OCP_USI bIdPC = bIdP * numCom;

    cout << "------------------------------" << endl;
    cout << "Bulk[" << n << "]" << endl;
    cout << fixed << setprecision(3);
    for (USI i = 0; i < numCom; i++) {
        cout << Ni[bIdC + i] << "   ";
    }
    cout << endl << P[n] << "   " << T[n];
    cout << endl;

    cout << fixed << setprecision(8);
    if (phaseExist[bIdP + 0]) {
        for (USI i = 0; i < numCom; i++) {
            cout << xij[bIdPC + i] << "   ";
        }
    } else {
        for (USI i = 0; i < numCom; i++) {
            cout << 0.000000 << "   ";
        }
    }
    cout << phaseExist[bIdP + 0] << "   ";
    cout << xi[bIdP + 0] << "   ";
    cout << S[bIdP + 0] << "   ";
    cout << endl;

    if (phaseExist[bIdP + 1]) {
        for (USI i = 0; i < numCom; i++) {
            cout << xij[bIdPC + numCom + i] << "   ";
        }
    } else {
        for (USI i = 0; i < numCom; i++) {
            cout << 0.000000 << "   ";
        }
    }

    cout << phaseExist[bIdP + 1] << "   ";
    cout << xi[bIdP + 1] << "   ";
    cout << S[bIdP + 1] << "   ";
    cout << endl;

    cout << fixed << setprecision(3);

    cout << vf[n] << "   " << rockVp[n] << "   ";
    cout << fixed << setprecision(12);
    cout << fabs(vf[n] - rockVp[n]) / rockVp[n] << endl;
    cout << fixed << setprecision(3);
    cout << vj[bIdP] << "   " << vj[bIdP + 1] << "   " << vj[bIdP + 2] << endl;
    cout << "------------------------------" << endl;
}

/////////////////////////////////////////////////////////////////////
// For AIMc
/////////////////////////////////////////////////////////////////////

void Bulk::ShowFIMBulk(const OCP_BOOL& flag) const
{

    if (flag) {
        USI iter = 0;
        for (USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfFIMbulk(n)) {
                // FIM bulk
                cout << setw(6) << n << "   ";
                iter++;
            }
            
            if ((iter + 1) % 10 == 0) {
                cout << endl;
                iter = 0;
            }
        }
        cout << endl;
    }
}

void Bulk::AllocateAuxAIMc()
{
    xijNR.resize(numBulk * numPhase * numCom);
    cfl.resize(numBulk * numPhase);
    bulkTypeAIM.Init(numBulk);
}

void Bulk::FlashAIMc()
{
    USI ftype;
    for (OCP_USI n = 0; n < numBulk; n++) {
        if (bulkTypeAIM.IfIMPECbulk(n)) {
            // IMPEC bulk
            ftype = CalFlashType(n, OCP_FALSE);

            flashCal[PVTNUM[n]]->Flash(P[n], T[n], &Ni[n * numCom], ftype, phaseNum[n],
                &xijNR[n * numPhase * numCom]);
            PassFlashValueAIMc(n);
        }       
    }
}


void Bulk::FlashAIMc01()
{
    USI ftype;
    for (OCP_USI n = 0; n < numBulk; n++) {
        if (bulkTypeAIM.IfIMPECbulk(n)) {
            // IMPEC bulk
            ftype = CalFlashType(n, OCP_FALSE);

            flashCal[PVTNUM[n]]->Flash(P[n], T[n], &Ni[n * numCom], ftype, phaseNum[n],
                &xij[n * numPhase * numCom]);
            PassFlashValue(n);
        }       
    }
}


void Bulk::FlashDerivAIMc()
{
    USI ftype;
    for (OCP_USI n = 0; n < numBulk; n++) {
        if (bulkTypeAIM.IfFIMbulk(n)) {
            // FIM bulk
            ftype = CalFlashType(n, OCP_TRUE);

            flashCal[PVTNUM[n]]->FlashDeriv(P[n], T[n], &Ni[n * numCom], ftype, phaseNum[n],
                &xij[n * numPhase * numCom]);
            PassFlashValueDeriv(n);
        }
    }
}


void Bulk::CalKrPcAIMc()
{
    OCP_FUNCNAME;

    if (!miscible) {
        OCP_DBL tmp = 0;
        for (OCP_USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfIMPECbulk(n)) {
                // IMPEC bulk
                OCP_USI bId = n * numPhase;
                flow[SATNUM[n]]->CalKrPc(&S[bId], &kr[bId], &Pc[bId], 0, tmp, tmp);
                for (USI j = 0; j < numPhase; j++)
                    Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];               
            }
            
        }
    } else {
        for (OCP_USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfIMPECbulk(n)) {
                // IMPEC bulk
                OCP_USI bId = n * numPhase;
                flow[SATNUM[n]]->CalKrPc(&S[bId], &kr[bId], &Pc[bId], surTen[n], Fk[n],
                    Fp[n]);
                for (USI j = 0; j < numPhase; j++)
                    Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];
            }           
        }
    }

    if (ScalePcow) {
        // correct
        const USI Wid = phase2Index[WATER];
        for (USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfIMPECbulk(n)) {
                // IMPEC bulk
                Pc[n * numPhase + Wid] *= ScaleValuePcow[n];
                Pj[n * numPhase + Wid] = P[n] + Pc[n * numPhase + Wid];
            }           
        }
    }
}

void Bulk::CalKrPcDerivAIMc()
{
    OCP_FUNCNAME;

    if (!miscible) {
        OCP_DBL tmp = 0;
        for (OCP_USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfFIMbulk(n)) {
                // FIM bulk
                OCP_USI bId = n * numPhase;
                flow[SATNUM[n]]->CalKrPcDeriv(&S[bId], &kr[bId], &Pc[bId],
                    &dKr_dS[bId * numPhase],
                    &dPcj_dS[bId * numPhase], 0, tmp, tmp);
                for (USI j = 0; j < numPhase; j++) Pj[bId + j] = P[n] + Pc[bId + j];
            }
        }
    } else {
        for (OCP_USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfFIMbulk(n)) {
                // FIM bulk
                OCP_USI bId = n * numPhase;
                flow[SATNUM[n]]->CalKrPcDeriv(
                    &S[bId], &kr[bId], &Pc[bId], &dKr_dS[bId * numPhase],
                    &dPcj_dS[bId * numPhase], surTen[n], Fk[n], Fp[n]);
                for (USI j = 0; j < numPhase; j++)
                    Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];
            }
        }
    }

    if (ScalePcow) {
        // correct
        const USI Wid = phase2Index[WATER];
        for (OCP_USI n = 0; n < numBulk; n++) {
            if (bulkTypeAIM.IfFIMbulk(n)) {
                // FIM bulk
                Pc[n * numPhase + Wid] *= ScaleValuePcow[n];
                Pj[n * numPhase + Wid] = P[n] + Pc[n * numPhase + Wid];
            }
        }
    }
}

void Bulk::GetSolAIMc(const vector<OCP_DBL>& u,
                      const OCP_DBL&         dPmaxlim,
                      const OCP_DBL&         dSmaxlim)
{
    dSNR     = S;
    NRdSmaxP = 0;
    NRdPmax  = 0;
    OCP_DBL         dP;
    USI             row = numPhase * (numCom + 1);
    USI             col = numCom + 1;
    OCP_USI         n_np_j;
    vector<OCP_DBL> dtmp(row, 0);
    OCP_DBL         chopmin = 1;
    OCP_DBL         choptmp = 0;

    for (OCP_USI n = 0; n < numBulk; n++) {
        if (bulkTypeAIM.IfIMPECbulk(n)) {
            // IMPEC Bulk
            // Pressure
            dP      = u[n * col];
            NRdPmax = max(NRdPmax, fabs(dP));
            P[n] += dP; // seems better
            dPNR[n]   = dP;
            NRstep[n] = 1;
            // Ni
            for (USI i = 0; i < numCom; i++) {
                dNNR[n * numCom + i] = u[n * col + 1 + i];
                Ni[n * numCom + i] += dNNR[n * numCom + i];
            }
            continue;
        }

        chopmin = 1;
        // compute the chop
        fill(dtmp.begin(), dtmp.end(), 0.0);
        DaAxpby(bRowSizedSdP[n], col, 1, &dSec_dPri[n * maxLendSdP], u.data() + n * col,
                1, dtmp.data());

        USI js = 0;
        for (USI j = 0; j < numPhase; j++) {
            if (!pSderExist[n * numPhase + j]) {
                continue;
            }
            n_np_j = n * numPhase + j;

            choptmp = 1;
            if (fabs(dtmp[js]) > dSmaxlim) {
                choptmp = dSmaxlim / fabs(dtmp[js]);
            } else if (S[n_np_j] + dtmp[js] < 0.0) {
                choptmp = 0.9 * S[n_np_j] / fabs(dtmp[js]);
            }

            // if (fabs(S[n_np_j] - scm[j]) > TINY &&
            //     (S[n_np_j] - scm[j]) / (choptmp * dtmp[js]) < 0)
            //     choptmp *= min(1.0, -((S[n_np_j] - scm[j]) / (choptmp * dtmp[js])));

            chopmin = min(chopmin, choptmp);
            js++;
        }

        // dS
        js = 0;
        for (USI j = 0; j < numPhase; j++) {
            if (!pSderExist[n * numPhase + j]) {
                dSNRP[n * numPhase + j] = 0;
                continue;
            }
            dSNRP[n * numPhase + j] = chopmin * dtmp[js];
            if (fabs(NRdSmaxP) < fabs(dSNRP[n * numPhase + j]))
                NRdSmaxP = dSNRP[n * numPhase + j];
            js++;
        }

        // dxij   ---- Compositional model only
        if (useEoS) {
            if (phaseNum[n] >= 3) {
                OCP_USI  bId     = 0;
                for (USI j = 0; j < 2; j++) {
                    bId = n * numPhase * numCom + j * numCom;
                    for (USI i = 0; i < numCom_1; i++) {
                        xij[bId + i] += chopmin * dtmp[js];
                        js++;
                    }
                }
            }
        }

        // dP
        dP = u[n * col];
        if (fabs(NRdPmax) < fabs(dP)) NRdPmax = dP;
        P[n] += dP; // seems better
        dPNR[n] = dP;

        // dNi
        NRstep[n] = chopmin;
        for (USI i = 0; i < numCom; i++) {
            dNNR[n * numCom + i] = u[n * col + 1 + i] * chopmin;
            if (fabs(NRdNmax) < fabs(dNNR[n * numCom + i]) / Nt[n])
                NRdNmax = dNNR[n * numCom + i] / Nt[n];

            Ni[n * numCom + i] += dNNR[n * numCom + i];
        }
    }
}

void Bulk::UpdatePj()
{
    for (OCP_USI n = 0; n < numBulk; n++) {
        for (USI j = 0; j < numPhase; j++) {
            Pj[n * numPhase + j] = P[n] + Pc[n * numPhase + j];
        }
    }
}

/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Oct/01/2021      Create file                          */
/*  Chensong Zhang      Oct/15/2021      Format file                          */
/*  Chensong Zhang      Jan/09/2022      Update Doxygen                       */
/*----------------------------------------------------------------------------*/