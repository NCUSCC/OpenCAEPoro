#pragma once
#include <fstream>
#include <vector>
#include "ReadTool.hpp"

typedef vector<vector<double>>				TUNING;

class TuningPair
{
public:
	TuningPair(int t, TUNING& tmp) :d(t), Tuning(tmp) {};
	int				d;
	TUNING			Tuning;
};

class ParamControl
{
public:

	int										Model;   // must be given
	string									Method;
	string									LinearSolve;
	vector<TuningPair>						Tuning_T;
	TUNING									Tuning;
	vector<double>							CriticalTime;
	
	void init();
	void initTime() { CriticalTime.push_back(0); };
	void initMethod();
	void initTuning();
	void inputMETHOD(ifstream& ifs);
	void inputTUNING(ifstream& ifs);
	void showTuning();
};

