#include "BOMixture.hpp"



void BOMixture::Flash_Sj(const double Pin, const double Pbbin, const double Tin, const double* Sjin, double Vpore, const double* Ziin)
{
	switch (Mode)
	{
	case PHASE_W:
		BOFlash_Sj_W(Pin, Sjin, Vpore);
		break;

	case PHASE_OW:
		BOFlash_Sj_OW(Pin, Sjin, Vpore);
		break;

	case PHASE_OGW:
		BOFlash_Sj_OGW(Pin, Pbbin, Sjin, Vpore);
		break;

	default:
		ERRORcheck("Wrong Mode");
		exit(0);
		break;
	}
}

void BOMixture::Flash_Ni(const double Pin, const double Tin, const double* Niin)
{
	switch (Mode)
	{
	case PHASE_W:
		BOFlash_Ni_W(Pin, Niin);
		break;

	case PHASE_OW:
		BOFlash_Ni_OW(Pin, Niin);
		break;

	case PHASE_OGW:
		BOFlash_Ni_OGW(Pin, Niin);
		break;

	default:
		ERRORcheck("Wrong Mode");
		exit(0);
	}
}


void BOMixture::BOFlash_Ni_W(const double Pin, const double* Niin)
{

}

void BOMixture::BOFlash_Ni_OW(const double Pin, const double* Niin)
{

}

void BOMixture::BOFlash_Sj_W(const double Pin, const double* Sjin, double Vpore)
{

}

void BOMixture::BOFlash_Sj_OW(const double Pin, const double* Sjin, double Vpore)
{

}


double BOMixture::gammaPhaseO(double Pin, double Pbbin)
{
	switch (Mode)
	{
	case PHASE_OW:
		return gammaPhaseO_OW(Pin);
	case PHASE_OGW:
		return gammaPhaseO_OGW(Pin, Pbbin);
	default:
		ERRORcheck("Wrong Mode");
		exit(0);
	}
}

double BOMixture::gammaPhaseO_OW(double Pin)
{

}

double BOMixture::gammaPhaseW(double Pin)
{
	std::vector<double>		data(5, 0);
	std::vector<double>		cdata(5, 0);

	PVTW.eval_all(0, Pin, data, cdata);
	double Pw0 = data[0];
	double bw0 = data[1];
	double cbw = data[2];
	double bw = (bw0 * (1 - cbw * (Pin - Pw0)));

	return Std_GammaW / bw;
}

double BOMixture::gammaPhaseG(double Pin)
{
	if (PVDG.isempty()) {
		ERRORcheck("PVDG is missing");
	}
	double bg = (CONV1 / 1000) * PVDG.eval(0, P, 1);

	return Std_GammaG / bg;
}