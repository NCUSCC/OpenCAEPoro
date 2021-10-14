#include "OCP_Method.hpp"


void OCP_IMPES::SetupParam(const string& dir, const string& file)
{
	solver.SetupParam(dir, file);
}

void OCP_IMPES::AllocateMat(const Reservoir& rs)
{
	solver.AllocateMem(rs.bulk.GetBulkNum() + rs.wellgroup.GetWellNum());
	rs.conn.AllocateMat(solver);
	rs.wellgroup.AllocateMat(solver);
	solver.AllocateColValMem();
}

void OCP_IMPES::run(Reservoir& rs, OCP_Control& ctrl, OCP_Output& output)
{

	USI numdates = ctrl.getNumDates();
	for (USI d = 0; d < numdates - 1; d++) {
		rs.wellgroup.ApplyControl(d);
		ctrl.ApplyControl(d);
		ctrl.InitTime(d);
		while (ctrl.criticalTime[d + 1] - ctrl.Current_time > TINY) {

			goOneStep(rs, ctrl);
			output.SetVal(rs, ctrl);

		}
	}
}

void OCP_IMPES::goOneStep(Reservoir& rs, OCP_Control& ctrl)
{
	OCP_DBL ve = 0.01;
	OCP_DBL cfl = 1;
	int	   flagCheck = 0;
	double& dt = ctrl.Current_dt;

	rs.wellgroup.PrepareWell(rs.bulk);

	cfl = rs.CalCFL(dt);
	if (cfl > 1)
		dt /= (cfl + 1);

	while (true)
	{
		SolveP(rs, ctrl, dt);

		// first check : Pressure check
		flagCheck = rs.CheckP();
		if (flagCheck == 1) {
			dt /= 2;
			continue;
		}
		else if (flagCheck == 2) {
			continue;
		}

		rs.conn.CalFlux(rs.bulk);
		rs.wellgroup.CalFlux(rs.bulk);

		// second check : cfl check
		cfl = rs.CalCFL(dt);
		if (cfl > 1) {
			dt /= 2;
			rs.ResetVal01();
			continue;
		}

		rs.conn.MassConserve(rs.bulk, dt);
		rs.wellgroup.MassConserve(rs.bulk, dt);

		// third check: Ni check
		if (!rs.CheckNi()) {
			dt /= 2;
			rs.ResetVal01();
			continue;
		}

		rs.bulk.FlashNi();
		rs.bulk.CalVporo();

		// fouth check: Volume error check
		if (!rs.CheckVe(ve)) {
			dt /= 2;
			rs.ResetVal02();
			continue;
		}

		rs.bulk.CalKrPc();
		rs.conn.CalFlux(rs.bulk);

		break;
	}


	rs.wellgroup.CalIPRT(rs.bulk, dt);
	ctrl.Tstep += 1;
	ctrl.NR_iter = 1;
	ctrl.NR_iter_total += 1;

	rs.bulk.CalMaxChange();
	ctrl.setNextTstep(rs);
	rs.bulk.SetLastStep();
	rs.wellgroup.SetLastStep();
}


void OCP_IMPES::SolveP(Reservoir& rs, OCP_Control& ctrl, const OCP_DBL& dt)
{
	rs.AssembleMat(solver, dt);

#ifdef _DEBUG
	solver.CheckVal();
#endif // _DEBUG

#ifdef __SOLVER_FASP__

	solver.AssembleMat_Fasp();
	GetWallTime Timer;
	Timer.Start();
	int status = solver.FaspSolve();
	ctrl.LS_time += Timer.Stop() / 1000;

#ifdef _DEBUG
	// solver.PrintfMatCSR("testA.dat", "testb.dat");
	// solver.PrintfSolution("testx.dat");
#endif // _DEBUG


	solver.Free_Fasp();

	ctrl.LS_iter = status;
	ctrl.LS_iter_total += status;

#endif // __SOLVER_FASP__

	rs.GetSolution_IMPES(solver.GetSolution());
	solver.ClearData();
}


/*----------------------------------------------------------------------------*/
/*  Brief Change History of This File                                         */
/*----------------------------------------------------------------------------*/
/*  Author              Date             Actions                              */
/*----------------------------------------------------------------------------*/
/*  Shizhe Li           Oct/01/2021      Create file                          */
/*----------------------------------------------------------------------------*/