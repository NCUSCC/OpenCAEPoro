#pragma once
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
#include "MAT.hxx"
#include "fasp.h"

extern "C" {
#include "fasp_functs.h"
}



template <typename T>
class Solver
{
	friend class OpenCAEPoro;
	friend class Connection_BB;
	friend class Well;
public:
	void allocate(int dimMax);
	void allocateColVal();

	void initSolver(string& dir, string& file);
	// FASP
	void init_param_Fasp();
	void read_param_Fasp();
	void assemble_Fasp();
	void free_Fasp();
	void showMat_CSR(string fileA, string fileb);



	void clearData();

	std::vector<T>& getSol() { return u; }

private:
	int									MaxDim;
	int									Dim;
	// for Mat assemble
	std::vector<int>					RowCapacity;
	std::vector<std::vector<int>>		ColId;
	std::vector<std::vector<T>>			Val;
	std::vector<int>					DiagPtr;
	std::vector<T>						DiagVal;  // just for assembling: intermediate variable

	std::vector<T>						b;
	std::vector<T>						u;

	// for FASP solver
	string								SolveDir;
	string								SolveFile;
	dCSRmat								A_Fasp;
	dvector								b_Fasp;
	dvector								x_Fasp;

	input_param							inparam;  // parameters from input files
	ITS_param							itparam;  // parameters for itsolver    

	AMG_param							amgparam; // parameters for AMG
	ILU_param							iluparam; // parameters for ILU
	SWZ_param							swzparam; // parameters for Schwarz method 
};


template <typename T>
void Solver<T>::allocate(int dimMax)
{
	MaxDim = dimMax;
	RowCapacity.resize(MaxDim, 0);
	ColId.resize(MaxDim);
	Val.resize(MaxDim);
	DiagPtr.resize(MaxDim, 0);
	DiagVal.resize(MaxDim, 0);
	b.resize(MaxDim, 0);
	u.resize(MaxDim, 0);
}

template <typename T>
void Solver<T>::allocateColVal()
{
	for (int n = 0; n < MaxDim; n++) {
		ColId[n].reserve(RowCapacity[n]);
		Val[n].reserve(RowCapacity[n]);
	}
}


template <typename T>
void Solver<T>::clearData()
{
	for (int i = 0; i < MaxDim; i++) {
		ColId[i].clear();
		Val[i].clear();
	}
	DiagPtr.resize(MaxDim, -1);
	DiagVal.resize(MaxDim, 0);
	b.resize(MaxDim, 0);
}


template <typename T>
void Solver<T>::assemble_Fasp()
{
	// b & x
	b_Fasp.row = Dim;
	b_Fasp.val = b.data();
	x_Fasp.row = Dim;
	x_Fasp.val = u.data();
	// A
	int nnz = 0;
	for (int i = 0; i < Dim; i++) {
		nnz += ColId[i].size();
	}
	A_Fasp = fasp_dcsr_create(Dim, Dim, nnz);
	// IA
	
	int count = 0;
	for (int i = 1; i < Dim + 1; i++) {
		int nnz_Row = ColId[i - 1].size();
		A_Fasp.IA[i] = A_Fasp.IA[i - 1] + nnz_Row;

		for (int j = 0; j < nnz_Row; j++) {
			A_Fasp.JA[count]	= ColId[i - 1][j];
			A_Fasp.val[count]	= Val[i - 1][j];
			count++;
		}
	}
}

template <typename T>
void Solver<T>::free_Fasp()
{
	fasp_dcsr_free(&A_Fasp);
	fasp_dvec_free(&b_Fasp);
	fasp_dvec_free(&x_Fasp);
}

template <typename T>
void Solver<T>::showMat_CSR(string fileA, string fileb)
{
	string FileA = SolveDir + fileA;
	string Fileb = SolveDir + fileb;
	// csr format
	ofstream outA(FileA);
	if (!outA.is_open())
	    cout << "Can not open " << FileA << endl;
	outA << A_Fasp.row << endl;
	int nnz0 = A_Fasp.nnz;
	for (int i = 0; i < A_Fasp.row + 1; i++)
	    outA << A_Fasp.IA[i]+1 << endl;
	for (int i = 0; i < nnz0; i++)
	    outA << A_Fasp.JA[i]+1 << endl;
	for (int i = 0; i < nnz0; i++)
	    outA << A_Fasp.val[i] << endl;
	outA.close();
	// out rhs
	ofstream outb(Fileb);
	if (!outb.is_open())
	    cout << "Can not open " << Fileb << endl;
	outb << b_Fasp.row << endl;
	for (int i = 0; i < b_Fasp.row; i++)
	    outb << b_Fasp.val[i] << endl;
	outb.close();
}


template <typename T>
void Solver<T>::initSolver(string& dir, string& file)
{
	SolveDir = dir;
	SolveFile = file;
#ifdef __SOLVER_FASP__
	read_param_Fasp();
#endif //  __SOLVER_FASP__

}

template <typename T>
void Solver<T>::read_param_Fasp()
{
	string file = SolveDir + SolveFile;
	init_param_Fasp(); // Set default solver parameters
	std::ifstream ifs(file);
	if (!ifs.is_open()) {
		std::cout << "The input file " << file << " is missing!" << std::endl;
		file = SolveDir + "../conf/csr.dat";
		ifs.open(file);
		if (!ifs.is_open()) {
			std::cout << "The input file " << file << " is missing!" << std::endl;
			std::cout << "Using the default parameters of FASP" << std::endl;
		}
		else {
			ifs.close();
			std::cout << "Using the input file " << file << std::endl;
			fasp_param_input(file.data(), &inparam);
		}
	}
	else {
		ifs.close(); // if file has been opened, close it first
		fasp_param_input(file.data(), &inparam);
	}
	fasp_param_init(&inparam, &itparam, &amgparam, &iluparam, &swzparam);
}

template <typename T>
void Solver<T>::init_param_Fasp() {
	// Input/output
	inparam.print_level = PRINT_MIN;
	inparam.output_type = 0;

	// Problem information
	inparam.solver_type = SOLVER_VFGMRES;
	inparam.precond_type = PREC_AMG;
	inparam.stop_type = STOP_REL_RES;

	// Solver parameters
	inparam.itsolver_tol = 1e-4;
	inparam.itsolver_maxit = 100;
	inparam.restart = 30;

	// ILU method parameters
	inparam.ILU_type = ILUk;
	inparam.ILU_lfil = 0;
	inparam.ILU_droptol = 0.001;
	inparam.ILU_relax = 0;
	inparam.ILU_permtol = 0.0;

	// Schwarz method parameters
	inparam.SWZ_mmsize = 200;
	inparam.SWZ_maxlvl = 2;
	inparam.SWZ_type = 1;
	inparam.SWZ_blksolver = SOLVER_DEFAULT;

	// AMG method parameters
	inparam.AMG_type = CLASSIC_AMG;
	inparam.AMG_levels = 20;
	inparam.AMG_cycle_type = V_CYCLE;
	inparam.AMG_smoother = SMOOTHER_GS;
	inparam.AMG_smooth_order = CF_ORDER;
	inparam.AMG_presmooth_iter = 1;
	inparam.AMG_postsmooth_iter = 1;
	inparam.AMG_relaxation = 1.0;
	inparam.AMG_coarse_dof = 500;
	inparam.AMG_coarse_solver = 0;
	inparam.AMG_tol = 1e-6;
	inparam.AMG_maxit = 1;
	inparam.AMG_ILU_levels = 0;
	inparam.AMG_SWZ_levels = 0;
	inparam.AMG_coarse_scaling = OFF;
	inparam.AMG_amli_degree = 1;
	inparam.AMG_nl_amli_krylov_type = 2;

	// Classical AMG specific
	inparam.AMG_coarsening_type = 1;
	inparam.AMG_interpolation_type = 1;
	inparam.AMG_max_row_sum = 0.9;
	inparam.AMG_strong_threshold = 0.3;
	inparam.AMG_truncation_threshold = 0.2;
	inparam.AMG_aggressive_level = 0;
	inparam.AMG_aggressive_path = 1;

	// Aggregation AMG specific
	inparam.AMG_aggregation_type = PAIRWISE;
	inparam.AMG_quality_bound = 8.0;
	inparam.AMG_pair_number = 2;
	inparam.AMG_strong_coupled = 0.25;
	inparam.AMG_max_aggregation = 9;
	inparam.AMG_tentative_smooth = 0.67;
	inparam.AMG_smooth_filter = ON;
	inparam.AMG_smooth_restriction = ON;
}