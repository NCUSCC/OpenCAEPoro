/*! \file    OCPOutput.cpp
 *  \brief   OCPOutput class definition
 *  \author  Shizhe Li
 *  \date    Oct/01/2021
 *
 *-----------------------------------------------------------------------------------
 *  Copyright (C) 2021--present by the OpenCAEPoro team. All rights reserved.
 *  Released under the terms of the GNU Lesser General Public License 3.0 or later.
 *-----------------------------------------------------------------------------------
 */

#include "OCPOutput.hpp"

void Summary::InputParam(const OutputSummary& summary_param)
{
    FPR  = summary_param.FPR;
    FOPR = summary_param.FOPR;
    FOPT = summary_param.FOPT;
    FGPR = summary_param.FGPR;
    FGPt = summary_param.FGPt;
    FWPR = summary_param.FWPR;
    FWPT = summary_param.FWPT;
    FGIR = summary_param.FGIR;
    FGIT = summary_param.FGIT;
    FWIR = summary_param.FWIR;
    FWIT = summary_param.FWIT;

    WOPR = summary_param.WOPR;
    WOPT = summary_param.WOPT;
    WGPR = summary_param.WGPR;
    WGPT = summary_param.WGPT;
    WWPR = summary_param.WWPR;
    WWPT = summary_param.WWPT;
    WGIR = summary_param.WGIR;
    WGIT = summary_param.WGIT;
    WWIR = summary_param.WWIR;
    WWIT = summary_param.WWIT;

    WBHP = summary_param.WBHP;
    DG   = summary_param.DG;

    BPR  = summary_param.BPR;
    SOIL = summary_param.SOIL;
    SGAS = summary_param.SGAS;
    SWAT = summary_param.SWAT;

    // cout << "Summary::InputParam" << endl;
}

void Summary::Setup(const Reservoir& reservoir, const OCP_DBL& totalTime)
{
    Sumdata.push_back(SumPair("TIME", "  ", "DAY"));
    Sumdata.push_back(SumPair("NRiter", "  ", "  "));
    Sumdata.push_back(SumPair("LSiter", "  ", "  "));
    if (FPR) Sumdata.push_back(SumPair("FPR", "  ", "PSIA"));
    if (FOPR) Sumdata.push_back(SumPair("FOPR", "  ", "STB/DAY"));
    if (FOPT) Sumdata.push_back(SumPair("FOPT", "  ", "STB"));
    if (FGPR) Sumdata.push_back(SumPair("FGPR", "  ", "MSCF/DAY"));
    if (FGPt) Sumdata.push_back(SumPair("FGPT", "  ", "MSCF"));
    if (FWPR) Sumdata.push_back(SumPair("FWPR", "  ", "STB/DAY"));
    if (FWPT) Sumdata.push_back(SumPair("FWPT", "  ", "STB"));
    if (FGIR) Sumdata.push_back(SumPair("FGIR", "  ", "MSCF/DAY"));
    if (FGIT) Sumdata.push_back(SumPair("FGIT", "  ", "MSCF"));
    if (FWIR) Sumdata.push_back(SumPair("FWIR", "  ", "STB/DAY"));
    if (FWIT) Sumdata.push_back(SumPair("FWIT", "  ", "STB"));

    const USI sp = reservoir.grid.GetNumDigitIJK();

    USI    num;
    USI    wellnum = reservoir.allWells.GetWellNum();
    string wellname;

    if (WOPR.activity) {
        if (WOPR.obj[0] == "All") {
            WOPR.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WOPR.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WOPR", wellname, "STB/DAY"));
                WOPR.index.push_back(w);
            }
        } else {
            num = WOPR.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WOPR", WOPR.obj[w], "STB/DAY"));
                WOPR.index.push_back(reservoir.allWells.GetIndex(WOPR.obj[w]));
            }
        }
    }

    if (WOPT.activity) {
        if (WOPT.obj[0] == "All") {
            WOPT.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WOPT.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WOPT", wellname, "STB"));
                WOPT.index.push_back(w);
            }
        } else {
            num = WOPT.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WOPT", WOPT.obj[w], "STB"));
                WOPT.index.push_back(reservoir.allWells.GetIndex(WOPT.obj[w]));
            }
        }
    }

    if (WGPR.activity) {
        if (WGPR.obj[0] == "All") {
            WGPR.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WGPR.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WGPR", wellname, "MSCF/DAY"));
                WGPR.index.push_back(w);
            }
        } else {
            num = WGPR.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WGPR", WGPR.obj[w], "MSCF/DAY"));
                WGPR.index.push_back(reservoir.allWells.GetIndex(WGPR.obj[w]));
            }
        }
    }

    if (WGPT.activity) {
        if (WGPT.obj[0] == "All") {
            WGPT.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WGPT.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WGPT", wellname, "MSCF"));
                WGPT.index.push_back(w);
            }
        } else {
            num = WGPT.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WGPT", WGPT.obj[w], "MSCF"));
                WGPT.index.push_back(reservoir.allWells.GetIndex(WGPT.obj[w]));
            }
        }
    }

    if (WWPR.activity) {
        if (WWPR.obj[0] == "All") {
            WWPR.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WWPR.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WWPR", wellname, "STB/DAY"));
                WWPR.index.push_back(w);
            }
        } else {
            num = WWPR.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WWPR", WWPR.obj[w], "STB/DAY"));
                WWPR.index.push_back(reservoir.allWells.GetIndex(WWPR.obj[w]));
            }
        }
    }

    if (WWPT.activity) {
        if (WWPT.obj[0] == "All") {
            WWPT.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WWPT.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WWPT", wellname, "STB"));
                WWPT.index.push_back(w);
            }
        } else {
            num = WWPT.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WWPT", WWPT.obj[w], "STB"));
                WWPT.index.push_back(reservoir.allWells.GetIndex(WWPT.obj[w]));
            }
        }
    }

    if (WGIR.activity) {
        if (WGIR.obj[0] == "All") {
            WGIR.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WGIR.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WGIR", wellname, "MSCF/DAY"));
                WGIR.index.push_back(w);
            }
        } else {
            num = WGIR.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WGIR", WGIR.obj[w], "MSCF/DAY"));
                WGIR.index.push_back(reservoir.allWells.GetIndex(WGIR.obj[w]));
            }
        }
    }

    if (WGIT.activity) {
        if (WGIT.obj[0] == "All") {
            WGIT.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WGIT.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WGIT", wellname, "MSCF"));
                WGIT.index.push_back(w);
            }
        } else {
            num = WGIT.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WGIT", WGIT.obj[w], "MSCF"));
                WGIT.index.push_back(reservoir.allWells.GetIndex(WGIT.obj[w]));
            }
        }
    }

    if (WWIR.activity) {
        if (WWIR.obj[0] == "All") {
            WWIR.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WWIR.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WWIR", wellname, "STB/DAY"));
                WWIR.index.push_back(w);
            }
        } else {
            num = WWIR.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WWIR", WWIR.obj[w], "STB/DAY"));
                WWIR.index.push_back(reservoir.allWells.GetIndex(WWIR.obj[w]));
            }
        }
    }

    if (WWIT.activity) {
        if (WWIT.obj[0] == "All") {
            WWIT.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WWIT.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WWIT", wellname, "STB"));
                WWIT.index.push_back(w);
            }
        } else {
            num = WWIT.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WWIT", WWIT.obj[w], "STB"));
                WWIT.index.push_back(reservoir.allWells.GetIndex(WWIT.obj[w]));
            }
        }
    }

    if (WBHP.activity) {
        if (WBHP.obj[0] == "All") {
            WBHP.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                wellname = reservoir.allWells.GetWellName(w);
                WBHP.obj.push_back(wellname);
                Sumdata.push_back(SumPair("WBHP", wellname, "PSIA"));
                WBHP.index.push_back(w);
            }
        } else {
            num = WBHP.obj.size();
            for (USI w = 0; w < num; w++) {
                Sumdata.push_back(SumPair("WBHP", WBHP.obj[w], "PSIA"));
                WBHP.index.push_back(reservoir.allWells.GetIndex(WBHP.obj[w]));
            }
        }
    }

    if (DG.activity) {
        if (DG.obj[0] == "All") {
            DG.obj.clear();
            for (USI w = 0; w < wellnum; w++) {
                string wellname = reservoir.allWells.GetWellName(w);
                DG.obj.push_back(wellname);
                USI perfnum = reservoir.allWells.GetWellPerfNum(w);
                for (USI p = 0; p < perfnum; p++) {
                    Sumdata.push_back(
                        SumPair("DG", wellname + " Perf" + to_string(p), "PSIA"));
                    DG.index.push_back(w);
                }
            }
        } else {
            num = DG.obj.size();
            for (USI w = 0; w < num; w++) {
                USI wId     = reservoir.allWells.GetIndex(DG.obj[w]);
                USI perfnum = reservoir.allWells.GetWellPerfNum(wId);
                for (USI p = 0; p < perfnum; p++) {
                    Sumdata.push_back(
                        SumPair("DG", DG.obj[w] + " P" + to_string(p), "PSIA"));
                    DG.index.push_back(wId);
                }
            }
        }
    }

    if (BPR.activity) {
        num = BPR.obj.size();
        for (USI i = 0; i < num; i++) {
            string temp = GetIJKformat(to_string(BPR.obj[i].I), to_string(BPR.obj[i].J),
                                       to_string(BPR.obj[i].K), sp);
            Sumdata.push_back(SumPair("BPR", temp, "PSIA"));
            USI I = BPR.obj[i].I - 1;
            USI J = BPR.obj[i].J - 1;
            USI K = BPR.obj[i].K - 1;
            BPR.index.push_back(reservoir.grid.GetActIndex(I, J, K));
        }
    }

    if (SOIL.activity) {
        num = SOIL.obj.size();
        for (USI i = 0; i < num; i++) {
            string temp =
                GetIJKformat(to_string(SOIL.obj[i].I), to_string(SOIL.obj[i].J),
                             to_string(SOIL.obj[i].K), sp);
            Sumdata.push_back(SumPair("SOIL", temp, "   "));
            USI I = SOIL.obj[i].I - 1;
            USI J = SOIL.obj[i].J - 1;
            USI K = SOIL.obj[i].K - 1;
            SOIL.index.push_back(reservoir.grid.GetActIndex(I, J, K));
        }
    }

    if (SGAS.activity) {
        num = SGAS.obj.size();
        for (USI i = 0; i < num; i++) {
            string temp =
                GetIJKformat(to_string(SGAS.obj[i].I), to_string(SGAS.obj[i].J),
                             to_string(SGAS.obj[i].K), sp);
            Sumdata.push_back(SumPair("SGAS", temp, "   "));
            USI I = SGAS.obj[i].I - 1;
            USI J = SGAS.obj[i].J - 1;
            USI K = SGAS.obj[i].K - 1;
            SGAS.index.push_back(reservoir.grid.GetActIndex(I, J, K));
        }
    }

    if (SWAT.activity) {
        num = SWAT.obj.size();
        for (USI i = 0; i < num; i++) {
            string temp =
                GetIJKformat(to_string(SWAT.obj[i].I), to_string(SWAT.obj[i].J),
                             to_string(SWAT.obj[i].K), sp);
            Sumdata.push_back(SumPair("SWAT", temp, "   "));
            USI I = SWAT.obj[i].I - 1;
            USI J = SWAT.obj[i].J - 1;
            USI K = SWAT.obj[i].K - 1;
            SWAT.index.push_back(reservoir.grid.GetActIndex(I, J, K));
        }
    }

    // Allocate memory
    USI rs = totalTime / 0.1;
    USI cs = Sumdata.size();
    for (USI i = 0; i < cs; i++) {
        Sumdata[i].val.reserve(rs);
    }

    // cout << "Summary::Setup" << endl;
}

void Summary::SetVal(const Reservoir& rs, const OCPControl& ctrl)
{
    USI n = 0;

    // TIME
    Sumdata[n++].val.push_back(ctrl.GetCurTime());
    // NRiter
    Sumdata[n++].val.push_back(ctrl.GetNRiterT());
    // LSiter
    Sumdata[n++].val.push_back(ctrl.GetLSiterT());

    // FPR
    if (FPR) Sumdata[n++].val.push_back(rs.bulk.CalFPR());
    if (FOPR) Sumdata[n++].val.push_back(rs.allWells.GetFOPR());
    if (FOPT) Sumdata[n++].val.push_back(rs.allWells.GetFOPT());
    if (FGPR) Sumdata[n++].val.push_back(rs.allWells.GetFGPR());
    if (FGPt) Sumdata[n++].val.push_back(rs.allWells.GetFGPT());
    if (FWPR) Sumdata[n++].val.push_back(rs.allWells.GetFWPR());
    if (FWPT) Sumdata[n++].val.push_back(rs.allWells.GetFWPT());
    if (FGIR) Sumdata[n++].val.push_back(rs.allWells.GetFGIR());
    if (FGIT) Sumdata[n++].val.push_back(rs.allWells.GetFGIT());
    if (FWIR) Sumdata[n++].val.push_back(rs.allWells.GetFWIR());
    if (FWIT) Sumdata[n++].val.push_back(rs.allWells.GetFWIT());

    USI len = 0;
    // WOPR
    len = WOPR.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWOPR(WOPR.index[w]));

    // WOPT
    len = WOPT.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWOPT(WOPT.index[w]));

    // WGPR
    len = WGPR.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWGPR(WGPR.index[w]));

    // WGPT
    len = WGPT.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWGPT(WGPT.index[w]));

    // WWPR
    len = WWPR.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWWPR(WWPR.index[w]));

    // WWPT
    len = WWPT.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWWPT(WWPT.index[w]));

    // WGIR
    len = WGIR.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWGIR(WGIR.index[w]));

    // WGIT
    len = WGIT.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWGIT(WGIT.index[w]));

    // WWIR
    len = WWIR.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWWIR(WWIR.index[w]));

    // WWIT
    len = WWIT.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWWIT(WWIT.index[w]));

    // WBHP
    len = WBHP.index.size();
    for (USI w = 0; w < len; w++)
        Sumdata[n++].val.push_back(rs.allWells.GetWBHP(WBHP.index[w]));

    // DG
    len = DG.obj.size();
    for (USI w = 0; w < len; w++) {
        USI numperf = rs.allWells.GetWellPerfNum(DG.index[w]);
        for (USI p = 0; p < numperf; p++) {
            Sumdata[n++].val.push_back(rs.allWells.GetWellDg(DG.index[w], p));
        }
    }

    // BPR
    len = BPR.index.size();
    for (USI i = 0; i < len; i++)
        Sumdata[n++].val.push_back(rs.bulk.GetP(BPR.index[i]));

    // SOIL
    len = SOIL.index.size();
    for (USI i = 0; i < len; i++)
        Sumdata[n++].val.push_back(rs.bulk.GetSOIL(SOIL.index[i]));

    // SGAS
    len = SGAS.index.size();
    for (USI i = 0; i < len; i++)
        Sumdata[n++].val.push_back(rs.bulk.GetSGAS(SGAS.index[i]));

    // SWAT
    len = SWAT.index.size();
    for (USI i = 0; i < len; i++)
        Sumdata[n++].val.push_back(rs.bulk.GetSWAT(SWAT.index[i]));
}

/// Write output information in the dir/SUMMARY.out file.
void Summary::PrintInfo(const string& dir) const
{
    string   FileOut = dir + "SUMMARY.out";
    ofstream outF(FileOut);
    if (!outF.is_open()) {
        OCP_ABORT("Can not open " + FileOut);
    }

    USI ns  = 12;
    USI col = 10;
    USI row = 0;
    USI num = Sumdata.size();
    USI len = Sumdata[0].val.size();
    USI id  = 0;
    USI ID  = 1;

    while (id != num) {

        outF << "Row " << ++row << "\n";

        // Item
        // Time
        outF << "\t" << setw(ns) << Sumdata[0].Item;

        id = ID;
        for (USI i = 1; i < col; i++) {
            outF << "\t" << setw(ns) << Sumdata[id++].Item;
            if (id == num) break;
        }
        outF << "\n";

        // Unit
        // Time
        outF << "\t" << setw(ns) << Sumdata[0].Unit;

        id = ID;
        for (USI i = 1; i < col; i++) {
            outF << "\t" << setw(ns) << Sumdata[id++].Unit;
            if (id == num) break;
        }
        outF << "\n";

        // Obj
        // Time
        outF << "\t" << setw(ns) << Sumdata[0].Obj;

        id = ID;
        for (USI i = 1; i < col; i++) {
            outF << "\t" << setw(ns) << Sumdata[id++].Obj;
            if (id == num) break;
        }
        outF << "\n";

        // Data
        for (USI l = 0; l < len; l++) {

            // Time
            outF << "\t" << setw(ns) << Sumdata[0].val[l];

            id = ID;
            for (USI i = 1; i < col; i++) {
                outF << "\t" << setw(ns) << Sumdata[id++].val[l];
                if (id == num) break;
            }
            outF << "\n";
        }

        ID += (col - 1);

        outF << "\n";
    }

    outF.close();
}

void CriticalInfo::Setup(const OCP_DBL& totalTime)
{
    // Allocate memory
    USI rc = totalTime / 0.1;
    time.reserve(rc);
    dt.reserve(rc);
    dPmax.reserve(rc);
    dVmax.reserve(rc);
    dSmax.reserve(rc);
    dNmax.reserve(rc);
    cfl.reserve(rc);
}

void CriticalInfo::SetVal(const Reservoir& reservoir, const OCPControl& ctrl)
{
    time.push_back(ctrl.GetCurTime());
    dt.push_back(ctrl.GetLastDt());
    dPmax.push_back(reservoir.bulk.GetdPmax());
    dVmax.push_back(reservoir.bulk.GetdVmax());
    dSmax.push_back(reservoir.bulk.GetdSmax());
    dNmax.push_back(reservoir.bulk.GetdNmax());
    cfl.push_back(reservoir.cfl);
}

void CriticalInfo::PrintFastReview(const string& dir) const
{
    string   FileOut = dir + "FastReview.out";
    ofstream outF(FileOut);
    if (!outF.is_open()) {
        OCP_ABORT("Can not open " + FileOut);
    }

    // Item
    outF << "\t" << setw(10) << "Time";
    outF << "\t" << setw(10) << "dt";
    outF << "\t" << setw(10) << "dPmax";
    outF << "\t" << setw(10) << "dVmax";
    outF << "\t" << setw(10) << "dSmax";
    outF << "\t" << setw(10) << "dNmax";
    outF << "\t" << setw(10) << "CFL\n";
    // Unit
    outF << "\t" << setw(10) << "Days";
    outF << "\t" << setw(10) << "Days";
    outF << "\t" << setw(10) << "Psia";
    outF << "\t" << setw(10) << "    ";
    outF << "\t" << setw(10) << "    ";
    outF << "\t" << setw(10) << "    ";
    outF << "\t" << setw(10) << "    \n\n";

    USI n = time.size();
    for (USI i = 0; i < n; i++) {
        outF << "\t" << setw(10) << time[i];
        outF << "\t" << setw(10) << dt[i];
        outF << "\t" << setw(10) << dPmax[i];
        outF << "\t" << setw(10) << dVmax[i];
        outF << "\t" << setw(10) << dSmax[i];
        outF << "\t" << setw(10) << dNmax[i];
        outF << "\t" << setw(10) << cfl[i];
        outF << "\n";
    }

    outF.close();
}


void BasicGridProperty::SetBasicGridProperty(const BasicGridPropertyParam& param)
{
    PRE = param.PRE;
    PGAS = param.PGAS;
    PWAT = param.PWAT;
    SOIL = param.SOIL;
    SGAS = param.SGAS;
    SWAT = param.SWAT;
    DENO = param.DENO;
    DENG = param.DENG;
    DENW = param.DENW;
    KRO = param.KRO;
    KRG = param.KRG;
    KRW = param.KRW;
    BOIL = param.BOIL;
    BGAS = param.BGAS;
    BWAT = param.BWAT;
    VOIL = param.VOIL;
    VGAS = param.VGAS;
    VWAT = param.VWAT;
    XMF = param.XMF;
    YMF = param.YMF;
    PCW = param.PCW;
}


void Out4RPT::InputParam(const OutputRPTParam& RPTparam)
{
    useRPT = RPTparam.useRPT;
    if (!useRPT) return;

    bgp.SetBasicGridProperty(RPTparam.bgp);
}


void Out4RPT::Setup(const string& dir, const Reservoir& rs)
{
    if (!useRPT) return;

    string   FileOut = dir + "RPT.out";
    ofstream outF(FileOut);
    if (!outF.is_open()) {
        OCP_ABORT("Can not open " + FileOut);
    }
    outF.close();

    nx = rs.grid.nx;
    ny = rs.grid.ny;
    numGrid = rs.grid.numGrid;
    IJKspace = rs.grid.numDigutIJK;
}

void Out4RPT::PrintRPT(const string&    dir,
                           const Reservoir& rs,
                           const OCP_DBL&   days) const
{

    if (!useRPT) return;

    string   FileOut = dir + "RPT.out";
    ofstream outRPT;
    outRPT.open(FileOut, ios::app);
    if (!outRPT.is_open()) {
        OCP_ABORT("Can not open " + FileOut);
    }

    const USI     np     = rs.bulk.numPhase;
    const USI     nc     = rs.bulk.numCom;
    const USI     OIndex = rs.bulk.phase2Index[OIL];
    const USI     GIndex = rs.bulk.phase2Index[GAS];
    const USI     WIndex = rs.bulk.phase2Index[WATER];
    const vector<GB_Pair>& g2bp = rs.grid.activeMap_G2B;

    outRPT << OCP_SEP02(50) << "\n";

    // Well Info
    USI numWell = rs.allWells.GetWellNum();
    outRPT << "Well Information"
        << "                    ";
    outRPT << fixed << setprecision(3) << days << "  DAYS" << "\n";
    // INJ
    for (USI w = 0; w < numWell; w++) {
        if (rs.allWells.wells[w].opt.type == INJ) {
            outRPT << "-------------------------------------" << "\n";
            outRPT << rs.allWells.wells[w].name << "   " << w << "   "
                << rs.allWells.wells[w].depth << " (feet)     ";
            outRPT << rs.allWells.wells[w].I << "   " << rs.allWells.wells[w].J << "\n";

            if (rs.allWells.wells[w].opt.state == OPEN) {
                outRPT << "OPEN\t" << rs.allWells.wells[w].WGIR << " (MSCF/DAY)\t"
                    << rs.allWells.wells[w].WWIR << " (STB/DAY)" << "\n";
            }
            else {
                outRPT << "SHUTIN" << "\n";
            }
            // perf
            for (USI p = 0; p < rs.allWells.wells[w].numPerf; p++) {
                outRPT << "perf" << p << "   " << rs.allWells.wells[w].perf[p].I << "   "
                    << rs.allWells.wells[w].perf[p].J << "   "
                    << rs.allWells.wells[w].perf[p].K << "   "
                    << rs.allWells.wells[w].perf[p].depth << "   ";
                if (rs.allWells.wells[w].perf[p].state == OPEN) {
                    outRPT << "OPEN";
                }
                else {
                    outRPT << "SHUTIN";
                }
                outRPT << "   " << rs.allWells.wells[w].perf[p].location << "\n";
            }
        }
    }
    // PROD
    for (USI w = 0; w < numWell; w++) {
        if (rs.allWells.wells[w].opt.type == PROD) {
            outRPT << "-------------------------------------" << "\n";
            outRPT << rs.allWells.wells[w].name << "   " << w << "   "
                << rs.allWells.wells[w].depth << " (feet)     ";
            outRPT << rs.allWells.wells[w].I << "   " << rs.allWells.wells[w].J << "\n";

            if (rs.allWells.wells[w].opt.state == OPEN) {
                outRPT << "OPEN\t" << rs.allWells.wells[w].WOPR << " (STB/DAY)\t"
                    << rs.allWells.wells[w].WGPR << " (MSCF/DAY)\t"
                    << rs.allWells.wells[w].WWPR << " (STB/DAY)" << "\n";
            }
            else {
                outRPT << "SHUTIN" << "\n";
            }
            // perf
            for (USI p = 0; p < rs.allWells.wells[w].numPerf; p++) {
                outRPT << "perf" << p << "   " << rs.allWells.wells[w].perf[p].I << "   "
                    << rs.allWells.wells[w].perf[p].J << "   "
                    << rs.allWells.wells[w].perf[p].K << "   "
                    << rs.allWells.wells[w].perf[p].depth << "   ";
                if (rs.allWells.wells[w].perf[p].state == OPEN) {
                    outRPT << "OPEN";
                }
                else {
                    outRPT << "SHUTIN";
                }
                outRPT << "   " << rs.allWells.wells[w].perf[p].location << "\n";
            }
        }
    }

    outRPT << "\n\n";


    static OCP_BOOL flag = OCP_FALSE;
    // Print once
    if (flag) {
        PrintRPT_Scalar(outRPT, "DX : feet", days, &rs.grid.dx[0], 1, g2bp, OCP_FALSE);
        PrintRPT_Scalar(outRPT, "DY : feet", days, &rs.grid.dy[0], 1, g2bp, OCP_FALSE);
        PrintRPT_Scalar(outRPT, "DZ : feet", days, &rs.grid.dz[0], 1, g2bp, OCP_FALSE);
        PrintRPT_Scalar(outRPT, "Depth : feet", days, &rs.grid.depth[0], 1, g2bp, OCP_FALSE);
        PrintRPT_Scalar(outRPT, "PERMX : MDarcy", days, &rs.grid.kx[0], 1, g2bp, OCP_FALSE);
        PrintRPT_Scalar(outRPT, "PERMY : MDarcy", days, &rs.grid.ky[0], 1, g2bp, OCP_FALSE);
        PrintRPT_Scalar(outRPT, "PERMZ : MDarcy", days, &rs.grid.kz[0], 1, g2bp, OCP_FALSE);
        flag = OCP_FALSE;
    }
    

    // PRESSURE
    if (bgp.PRE) {
        PrintRPT_Scalar(outRPT, "PRESSURE : psia", days, &rs.bulk.P[0], 1, g2bp, OCP_TRUE);
    }

    // DENSITY of OIL
    if (bgp.DENO && rs.bulk.oil) {
        PrintRPT_Scalar(outRPT, "DENO : lb/ft3", days, &rs.bulk.rho[OIndex], np, g2bp, OCP_TRUE);
    }
    outRPT << endl;

    // DENSITY of GAS
    if (bgp.DENG && rs.bulk.gas) {
        PrintRPT_Scalar(outRPT, "DENG : lb/ft3", days, &rs.bulk.rho[GIndex], np, g2bp, OCP_TRUE);
    }

    // DENSITY of WATER
    if (bgp.DENW && rs.bulk.water) {
        PrintRPT_Scalar(outRPT, "DENW : lb/ft3", days, &rs.bulk.rho[WIndex], np, g2bp, OCP_TRUE);
    }

    // SATURATION of OIL
    if (bgp.SOIL && rs.bulk.oil) {
        PrintRPT_Scalar(outRPT, "SOIL         ", days, &rs.bulk.S[OIndex], np, g2bp, OCP_TRUE);
    }

    // SATURATION of GAS
    if (bgp.SGAS && rs.bulk.gas) {
        PrintRPT_Scalar(outRPT, "SGAS         ", days, &rs.bulk.S[GIndex], np, g2bp, OCP_TRUE);
    }

    // SATURATION of WATER
    if (bgp.SWAT && rs.bulk.water) {
        PrintRPT_Scalar(outRPT, "SWAT         ", days, &rs.bulk.S[WIndex], np, g2bp, OCP_TRUE);
    }

    // Relative Permeability of OIL
    if (bgp.KRO && rs.bulk.oil) {
        PrintRPT_Scalar(outRPT, "KRO          ", days, &rs.bulk.kr[OIndex], np, g2bp, OCP_TRUE);
    }

    // Relative Permeability of GAS
    if (bgp.KRG && rs.bulk.gas) {
        PrintRPT_Scalar(outRPT, "KRG          ", days, &rs.bulk.kr[GIndex], np, g2bp, OCP_TRUE);
    }

    // Relative Permeability of WATER
    if (bgp.KRW && rs.bulk.water) {
        PrintRPT_Scalar(outRPT, "KRW          ", days, &rs.bulk.kr[WIndex], np, g2bp, OCP_TRUE);
    }

    // Molar Density of OIL
    if (bgp.BOIL && rs.bulk.oil && rs.bulk.comps) {
        PrintRPT_Scalar(outRPT, "BOIL : lb-M/rb", days, &rs.bulk.xi[OIndex], np, g2bp, OCP_TRUE);
    }

    // Molar Density of GAS
    if (bgp.BGAS && rs.bulk.gas && rs.bulk.comps) {
        PrintRPT_Scalar(outRPT, "BGAS : lb-M/rb", days, &rs.bulk.xi[GIndex], np, g2bp, OCP_TRUE);
    }

    // Molar Density of WATER
    if (bgp.BWAT && rs.bulk.water) {
        PrintRPT_Scalar(outRPT, "BWAT : lb-M/rb", days, &rs.bulk.xi[WIndex], np, g2bp, OCP_TRUE, (CONV1 * 19.437216));
    }

    // Viscosity of OIL
    if (bgp.VOIL && rs.bulk.oil) {
        PrintRPT_Scalar(outRPT, "VOIL : lb-M/rb", days, &rs.bulk.mu[OIndex], np, g2bp, OCP_TRUE);
    }

    // Viscosity of GAS
    if (bgp.VGAS && rs.bulk.gas) {
        PrintRPT_Scalar(outRPT, "VGAS : lb-M/rb", days, &rs.bulk.mu[GIndex], np, g2bp, OCP_TRUE);
    }

    // Viscosity of WATER
    if (bgp.VWAT && rs.bulk.water) {
        PrintRPT_Scalar(outRPT, "VWAT : lb-M/rb", days, &rs.bulk.mu[WIndex], np, g2bp, OCP_TRUE);
    }

    // liquid component mole fractions.
    if (bgp.XMF && rs.bulk.comps) {
        for (USI i = 0; i < nc - 1; i++) {
            PrintRPT_Scalar(outRPT, "XMF : Oil  " + to_string(i + 1) + "th Component", days, &rs.bulk.xij[OIndex * nc + i], np * nc, g2bp, OCP_TRUE);
        }
    }

    // gas component mole fractions.
    if (bgp.YMF && rs.bulk.comps) {
        for (USI i = 0; i < nc - 1; i++) {
            PrintRPT_Scalar(outRPT, "YMF : Gas  " + to_string(i + 1) + "th Component", days, &rs.bulk.xij[GIndex * nc + i], np * nc, g2bp, OCP_TRUE);
        }
    }

    // surface tension
    if (rs.bulk.miscible & OCP_FALSE) {
        PrintRPT_Scalar(outRPT, "STEN        ", days, &rs.bulk.surTen[0], 1, g2bp, OCP_TRUE);
    }

    // Fk
    if (rs.bulk.miscible & OCP_FALSE) {
        PrintRPT_Scalar(outRPT, "FMISC       ", days, &rs.bulk.Fk[0], 1, g2bp, OCP_TRUE);
    }

    // Fp
    if (rs.bulk.miscible & OCP_FALSE) {
        PrintRPT_Scalar(outRPT, "FPC         ", days, &rs.bulk.Fp[0], 1, g2bp, OCP_TRUE);
    }

    // Po - Pw
    if (bgp.PCW) {
        PrintRPT_Scalar(outRPT, "PCW : psia  ", days, &rs.bulk.Pc[WIndex], np, g2bp, OCP_TRUE);
    }

    outRPT.close();
}


void Out4RPT::GetIJKGrid(USI& i, USI& j, USI& k, const OCP_USI& n) const
{
    // i,j,k begin from 1
    // n must be the index of grids instead bulks
    k = n / (nx * ny) + 1;
    j = (n - (k - 1) * nx * ny) / nx + 1;
    i = n - (k - 1) * nx * ny - (j - 1) * nx + 1;
}


void Out4VTK::InputParam(const OutputVTKParam& VTKParam)
{
    useVTK = VTKParam.useVTK;
    if (!useVTK) return;

    bgp.SetBasicGridProperty(VTKParam.bgp);
}


void Out4VTK::Setup(const string& dir, const Reservoir& rs, const USI& ndates)
{
    if (!useVTK) return;

    string file = dir + "grid" + to_string(index) + ".vtk";
    string newfile;
    string title = "test";

    out4vtk.Init(file, title, VTK_ASCII, VTK_UNSTRUCTURED_GRID, rs.grid.polyhedronGrid.size(), rs.allWells.polyhedronWell.size());
    out4vtk.OutputPOINTS(file, rs.grid.polyhedronGrid, rs.allWells.polyhedronWell, VTK_FLOAT);
    out4vtk.OutputCELLS(file, rs.grid.polyhedronGrid, rs.allWells.polyhedronWell);
    out4vtk.OutputCELL_TYPES(file, rs.grid.polyhedronGrid, rs.allWells.polyhedronWell);
    out4vtk.BeginCellData();
    // output dead grid, live grid, well
    vector<USI> tmpW(rs.allWells.numWell, 2);
    out4vtk.OutputCELL_DATA_SCALARS(file, "CellType", VTK_UNSIGNED_INT, &rs.grid.activityFlag[0],
        1, rs.grid.activeMap_G2B, OCP_FALSE, tmpW);

    for (USI i = 1; i < ndates; i++) {
        index++;
        newfile = dir + "grid" + to_string(index) + ".vtk";;
        ifstream source(file, ios::binary);
        ofstream dest(newfile, ios::binary);
        dest << source.rdbuf();
        source.close();
        dest.close();
    }
    index = 0;
}


void Out4VTK::PrintVTK(const string& dir, const Reservoir& rs, const OCP_DBL& days) const
{
    if (!useVTK) return;

    string file = dir + "grid" + to_string(index) + ".vtk";
    // Calulcate Well val for output
    rs.allWells.SetWellVal();

    const USI     np = rs.bulk.numPhase;
    const USI     OIndex = rs.bulk.phase2Index[OIL];
    const USI     GIndex = rs.bulk.phase2Index[GAS];
    const USI     WIndex = rs.bulk.phase2Index[WATER];
    const vector<GB_Pair>& g2bp = rs.grid.activeMap_G2B;
    const vector<OCP_DBL>& well = rs.allWells.wellVal;

    // output
    if (bgp.PRE)
        out4vtk.OutputCELL_DATA_SCALARS(file, "PRESSURE", VTK_FLOAT, &rs.bulk.P[0], 1, g2bp, OCP_TRUE, well);
    if (bgp.SOIL)
        out4vtk.OutputCELL_DATA_SCALARS(file, "SOIL", VTK_FLOAT, &rs.bulk.S[OIndex], np, g2bp, OCP_TRUE, well);
    if (bgp.SGAS)
        out4vtk.OutputCELL_DATA_SCALARS(file, "SGAS", VTK_FLOAT, &rs.bulk.S[GIndex], np, g2bp, OCP_TRUE, well);
    if (bgp.SWAT)
        out4vtk.OutputCELL_DATA_SCALARS(file, "SWAT", VTK_FLOAT, &rs.bulk.S[WIndex], np, g2bp, OCP_TRUE, well);

    index++;
}


void OCPOutput::InputParam(const ParamOutput& paramOutput)
{
    summary.InputParam(paramOutput.summary);
    out4RPT.InputParam(paramOutput.outRPTParam);
    out4VTK.InputParam(paramOutput.outVTKParam);
}

void OCPOutput::Setup(const Reservoir& reservoir, const OCPControl& ctrl)
{
    workDir = ctrl.workDir;
    summary.Setup(reservoir, ctrl.criticalTime.back());
    crtInfo.Setup(ctrl.criticalTime.back());
    out4RPT.Setup(workDir, reservoir);
    out4VTK.Setup(workDir, reservoir, ctrl.criticalTime.size());
}

void OCPOutput::SetVal(const Reservoir& reservoir, const OCPControl& ctrl)
{
    summary.SetVal(reservoir, ctrl);
    crtInfo.SetVal(reservoir, ctrl);
}

void OCPOutput::PrintInfo() const
{
    summary.PrintInfo(workDir);
    crtInfo.PrintFastReview(workDir);
}

void OCPOutput::PrintInfoSched(const Reservoir&  rs,
                               const OCPControl& ctrl,
                               const OCP_DBL&    time) const
{
    OCP_DBL days = ctrl.current_time;
    cout << "Timestep " << setw(6) << left << ctrl.numTstep << ": " << fixed << setw(10)
         << setprecision(3) << right << days << " Days"
         << "    Wall time: " << time / 1000 << " Sec" << endl;

    GetWallTime timer;
    timer.Start();

    out4RPT.PrintRPT(workDir, rs, days);
    out4VTK.PrintVTK(workDir, rs, days);

    outputTime += timer.Stop() / 1000;
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