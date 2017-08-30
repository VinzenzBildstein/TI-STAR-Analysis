#include "Reconstruction.hh"

Reconstruction::Reconstruction() { 
}

Reconstruction::Reconstruction(Nucleus* projectile, Compound* target, double thickness) {
	SetTargetThickness(thickness);
	SetProj(projectile);
	SetTarget(target);
}

Reconstruction::Reconstruction(Nucleus* projectile, Compound* target) {
	SetProj(projectile);
	SetTarget(target);
}

double Reconstruction::StoppingPower(double energy, bool gaseous) {
	double stopping = 0;
	for(int i=0;i<fTarget->GetNofElements();i++) {
		//std::cout << fTarget->GetNucleus(i)->GetA() <<" "<< " with fraction " << fTe Carget->GetFrac(i) << std::endl;
		stopping += fTarget->GetFrac(i)*StoppingPower(fTarget->GetNucleus(i), energy, gaseous);
		//  std::cout << " stopping <fTarget->GetNofElements() " << fTarget->GetNofElements() << std::endl;
		//  std::cout << " stopping  fTarget->GetFrac(i) " <<  fTarget->GetFrac(i) << std::endl;
		//  std::cout << " stopping fTarget->GetNucleus(i)" <<  fTarget->GetNucleus(i)<< std::endl;
		//  std::cout << " stopping energy" << energy << std::endl;
		//  std::cout << " stopping gaseous" << gaseous << std::endl;
	}
	return stopping;
}

double Reconstruction::StoppingPower(Nucleus* target, double energy, bool gaseous) {
	int z_p = fProj->GetZ();
	double a_p = fProj->GetA();
	int z_t = target->GetZ();
	double a_t = target->GetA();
	if( (z_p < 1) || (a_p < 1) || (z_t < 1) || (a_t < 1) || (energy<1e-30) )
		return 0;
	if(z_t > 92)
		z_t = 92;
	double vsq;
	double vovc;
	double factor;  //to convert units from eV/10^15 Atoms/cm^2 to MeV/mg/cm^2
	double a;
	double eps;
	double dedx_n;
	double dedx_e;
	double e_h;
	double z_eff;
	double hexp;
	double beta;
	double e_he;

	vsq = energy/a_p;
	vovc = 0.04634*sqrt(vsq);
	if(vovc > 1) {
		vovc = 0.999999;
	}
	factor = 0.60225/a_t;

	//nuclear stopping powers
	a = a_p + a_t;
	z_eff = pow(z_p,0.23) + pow(z_t,0.23);
	eps = 32520.*energy*a_t/(z_eff*z_p*z_t*a);
	if(eps > 30.) {
		dedx_n = 0.5*log(eps)/eps;
	}
	else{
		dedx_n = log(1. + 1.1383*eps)/(2.*(eps + 0.01321*pow(eps,0.21226) + 0.19593*sqrt(eps)));
	}
	dedx_n *= 8.462*z_p*z_t*a_p/(a*z_eff)*factor;

	//master electronic stopping powers for hydrogen
	if(z_p != 2) {
		e_h = 1000.*vsq;
		// old version with strips 
		/*
			if(e_h >= 1000.) {
			bsq = pow(vovc,2);
			dedx_e = factor*a_h(6,z_t)/bsq*log(a_h(7,z_t)*bsq/(1.-bsq))+shell_correction(z_t)/e_h;
			}
			else if(e_h <= 10.) {
			dedx_e = factor*a_h(1,z_t)*sqrt(e_h);
			}
			else{
			dedx_e = factor/(1./(a_h(2,z_t)*pow(e_h,0.45)) + e_h/(a_h(3,z_t)*log(1. + a_h(4,z_t)/e_h + a_h(5,z_t)*e_h)));
			}
			*/

		// new version
		dedx_e = factor/(1./(a_h(2,z_t)*pow(e_h,0.45)) + e_h/(a_h(3,z_t)*log(1. + a_h(4,z_t)/e_h + a_h(5,z_t)*e_h)));


		//calculate Ziegler's effective Z's and electronic stopping powers (nothing to be done for helium and special case Lithium) //irma.f
		if(z_p == 1) {
			return (dedx_n + dedx_e);
		}
		else if(z_p == 3) {
			z_eff = 3.;
			hexp = 0.7138 + 0.002797*e_h + 1.348e-6*e_h*e_h;
			if(hexp < 60)
				z_eff -= 3./exp(hexp);
			dedx_e *= z_eff*z_eff;
			return (dedx_n + dedx_e);
		}
		else{
			beta = 0.1772*sqrt(e_h)/pow(z_p,2./3.);
			z_eff = z_p*(1. - (1.034 - 0.1777/exp(0.08114*z_p))/exp(beta + 0.0378*sin(1.5708*beta)));
			if(z_eff < 1.)
				z_eff = 1.;
			dedx_e *= z_eff*z_eff;
			return (dedx_n + dedx_e);
		}
	}
	else{
		e_he = 4.*vsq;
		if(e_he >= 10.) { //solid and gaseous matter , high energies
			dedx_e = factor*exp(a_he(6,z_t) + (a_he(7,z_t) + (a_he(8,z_t) + a_he(9,z_t)*log(1./e_he))*log(1./e_he))*log(1./e_he));
			return (dedx_n + dedx_e);
		}
		else{ //low energies
			if(!gaseous) { //solid matter
				dedx_e = factor/(1./(pow(1000.*e_he,a_he(2,z_t))*a_he(1,z_t))+e_he/(a_he(3,z_t)*log(1.+a_he(4,z_t)/e_he+a_he(5,z_t)*e_he)));
				return (dedx_n + dedx_e);
			}
			else{ //gaseous matter
				dedx_e = factor/(1./(pow(1000.*e_he,b_he(2,z_t))*b_he(1,z_t))+e_he/(b_he(3,z_t)*log(1.+b_he(4,z_t)/e_he+b_he(5,z_t)*e_he)));
				return (dedx_n + dedx_e);
			}
		}
	}  
}

double Reconstruction::CompoundRange(double energy, int limit, bool gaseous) {           // siehe irma.f
	int z_p = fProj->GetZ();
	double a_p = fProj->GetA();

	double en_1;
	double en_2;
	int n;
	double range;
	double h;

	en_1 = pow(10.,limit);
	if( (energy<en_1) || (z_p<1) || (a_p<1) )
		return 0;
	n = -limit - 1 + (int) floor(log(energy)*0.4342944819 + 0.5);   //0.4342..=log e, floor: returns largest integral value (rundet ab)
	en_2 = energy/pow(10,(double) n);
	//integrate using Weddle formula     http://mathworld.wolfram.com/WeddlesRule.html,  ca 1960
	range =0;  // 1e-30;
	n++;
	for(int i=0;i<n;i++) {
		h = (en_2 - en_1)/6.;
		range += h*(0.3/StoppingPower(en_1, gaseous) + 1.5/StoppingPower(en_1 + h, gaseous)
				+ 0.3/StoppingPower(en_1 + 2*h, gaseous) + 1.8/StoppingPower(en_1 + 3*h, gaseous)
				+ 0.3/StoppingPower(en_2 - 2*h, gaseous) + 1.5/StoppingPower(en_2 - h, gaseous)
				+ 0.3/StoppingPower(en_2, gaseous));
		en_1 = en_2;
		en_2 *= 10;
	}
	return range;
}

double Reconstruction::EnergyAfter(double energy, int limit, bool gaseous) {        // return energy after target!
	double en;
	double range;
	double dedx;
	double dx;
	double x;
	if(fTargetThickness<0) {
		std::cerr << "Target thickness not set!" << std::endl;
		return 0;
	}
	en = energy;
	//std::cout << " ea energy " << en << std::endl;
	x = 0;
	if(energy <= 0) {
		range = 0.;
		dedx = 100000.;
	} else {
		//std::cout << " ea energy2 " << energy << "  " << limit << std::endl;
		range = CompoundRange(energy, limit, gaseous);
		dedx = StoppingPower(energy, gaseous);
		//std::cout << " ea range " << range << std::endl;
		//std::cout << " ea dedx " << dedx << std::endl;
	}

	dx = 0.1/dedx;
	if((range - fTargetThickness) <= 0) {
		std::cout<<"stopped!"<<std::endl;
		return 0.;
	}
	while(1) {
		if(fTargetThickness - x - dx < 0) {
			return (en - dedx*(fTargetThickness - x));
		}
		en -= dedx*dx;
		x += dx;
		//std::cout << " ea x2 " << x << std::endl;
		dedx = StoppingPower(en, gaseous);
		//std::cout << " ea dedx2 " << dedx << std::endl;
	}
}

TSpline3* Reconstruction::Energy2Range(double emax, double size, bool gaseous) {
	double* range = new double[(int)(emax/size)+1];
	double* energy = new double[(int)(emax/size)+1];

	for(int i=0;i<(emax/size);i++) {
		energy[i] = i*size;
		range[i] = CompoundRange(energy[i],-5, gaseous);   // what is -5?
		energy[i]*=1000.; //conversion to keV
	}
	TGraph* graph = new TGraph((int)(emax/size), energy, range);
	TSpline3* spline = new TSpline3("Energy2Range",graph);
	delete graph;
	delete[] range;
	delete[] energy;
	return spline;
}

TSpline3* Reconstruction::Range2Energy(double emax, double size, bool gaseous) {
	double* range = new double[(int)(emax/size)+1];
	double* energy = new double[(int)(emax/size)+1];

	for(int i=0;i<(emax/size);i++) {
		energy[i] = i*size;
		range[i] = CompoundRange(energy[i],-5, gaseous);
		energy[i]*=1000.; //conversion to keV
	}
	TGraph* graph = new TGraph((int)(emax/size), range, energy);
	TSpline3* spline = new TSpline3("Range2Energy",graph);
	delete graph;
	delete[] range;
	delete[] energy;
	return spline;
}

TSpline3* Reconstruction::Energy2EnergyLoss(double emax, double size, bool gaseous) {
	double* eloss = new double[(int)(emax/size)+1];
	double* energy = new double[(int)(emax/size)+1];

	for(int i=0;i<(emax/size);i++) {
		energy[i]  = i*size;
		eloss[i]   = EnergyLoss(energy[i], -5, gaseous)*1000.; //conversion to keV
		energy[i] *= 1000.; //conversion to keV
	}
	TGraph* graph = new TGraph((int)(emax/size), energy, eloss);
	TSpline3* spline = new TSpline3("Energy2EnergyLoss",graph);
	delete graph;
	delete[] eloss;
	delete[] energy;
	return spline;
}

TSpline3* Reconstruction::Energy2EnergyAfter(double emax, double size, bool gaseous) {
	double* eloss = new double[(int)(emax/size)+1];
	double* energy = new double[(int)(emax/size)+1];
	double* eafter = new double[(int)(emax/size)+1];
	int through =0;
	for(int i=0;i<(emax/size);i++) {
		energy[i] = i*size;
		eloss[i]  = EnergyLoss(energy[i], -5, gaseous)*1000.; //conversion to keV
		energy[i] = energy[i]*1000. - eloss[i]; //conversion to keV
		if(energy[i]<10) {
			energy[i] = 0.;
		} else {
			eloss[through]  = eloss[i];
			eafter[through] = energy[i];
			through++;
		}
	}
	TGraph* graph = new TGraph(through, eafter, eloss);
	TSpline3* spline = new TSpline3("Energy2EnergyAfter",graph);
	delete graph;
	delete[] eloss;
	delete[] energy;
	delete[] eafter;
	return spline;
}

TSpline3* Reconstruction::Thickness2EnergyAfter(double energy, double maxThickness, double stepSize, bool gaseous) { // used to get energies in the middel and aftar target **** LA ****
	double* thickness = new double[static_cast<int>(maxThickness/stepSize)+1];
	double* eAfter    = new double[static_cast<int>(maxThickness/stepSize)+1];
	int i;
	thickness[0] = 0.;
	eAfter[0] = energy;
	for(i = 1; i < static_cast<int>(maxThickness/stepSize); ++i) {
		thickness[i] = i*stepSize;
		fTargetThickness = thickness[i];
		eAfter[i]  = EnergyAfter(energy, -5, gaseous)*1000.; //conversion to keV
		if(eAfter[i] < 10.) { // or  if(eAfter[i] < 10. && eAfter[i] >0.) not to get negative energies **** LA ****
			break;
		}
	}
	TGraph* graph = new TGraph(i, thickness, eAfter);
	TSpline3* spline = new TSpline3("Thickness2EnergyAfter", graph);
	delete graph;
	delete[] thickness;
	delete[] eAfter;
	return spline;
}

TGraph* Reconstruction::EnergyAfter2Energy(double emax, double size, bool gaseous) {
	double* eloss = new double[(int)(emax/size)+1];
	double* energy = new double[(int)(emax/size)+1];
	double* eafter = new double[(int)(emax/size)+1];
	int through =0;
	for(int i=0;i<(emax/size);i++) {
		energy[i] = i*size;
		eloss[i] = EnergyLoss(energy[i],-5, gaseous)*1000.; //conversion to keV
		energy[i]=energy[i]*1000.-eloss[i]; //conversion to keV
		if(energy[i]<10)
			energy[i]=0.;
		else if(eloss[i]<10)
			eloss[i]=0.;
		else{
			eloss[through]=eloss[i];
			eafter[through]=energy[i];
			through++;
		}
	}
	TGraph* graph = new TGraph(through, eloss, eafter);
	delete[] eloss;
	delete[] energy;
	return graph;
}

double Reconstruction::a_h(int index, int z) {
	double res[92][7] = {{1.262, 1.440,  242.6,12000.0,0.115900,0.000510,54360.0},         
								{1.229, 1.397,  484.5, 5873.0,0.052250,0.001020,24510.0},
								{1.411, 1.600,  725.6, 3013.0,0.045780,0.001530,21470.0},         
								{2.248, 2.590,  966.0,  153.8,0.034750,0.002039,16300.0},         
								{2.474, 2.815, 1206.0, 1060.0,0.028550,0.002549,13450.0},         
								{2.631, 2.989, 1445.0,  957.2,0.028190,0.003059,13220.0},         
								{2.954, 3.350, 1683.0, 1900.0,0.025130,0.003569,11790.0},         
								{2.652, 3.000, 1920.0, 2000.0,0.022300,0.004079,10460.0},         
								{2.085, 2.352, 2157.0, 2634.0,0.018160,0.004589, 8517.0},         
								{1.951, 2.199, 2393.0, 2699.0,0.015680,0.005099, 7353.0},         
								{2.542, 2.869, 2628.0, 1854.0,0.014720,0.005609, 6905.0},         
								{3.792, 4.293, 2862.0, 1009.0,0.013970,0.006118, 6551.0},         
								{4.154, 4.739, 2766.0,  164.5,0.020230,0.006628, 6309.0},         
								{4.150, 4.700, 3329.0,  550.0,0.013210,0.007138, 6194.0},         
								{3.232, 3.647, 3561.0, 1560.0,0.012670,0.007648, 5942.0},         
								{3.447, 3.891, 3792.0, 1219.0,0.012110,0.008158, 5678.0},         
								{5.047, 5.714, 4023.0,  878.6,0.011780,0.008668, 5524.0},         
								{5.731, 6.500, 4253.0,  530.0,0.011230,0.009178, 5268.0},         
								{5.151, 5.833, 4482.0,  545.7,0.011290,0.009687, 5295.0},         
								{5.521, 6.252, 4710.0,  553.3,0.011120,0.010200, 5214.0},         
								{5.201, 5.884, 4938.0,  560.9,0.009995,0.010710, 4688.0},         
								{4.862, 5.496, 5165.0,  568.5,0.009474,0.011220, 4443.0},         
								{4.480, 5.055, 5391.0,  952.3,0.009117,0.011730, 4276.0},         
								{3.983, 4.489, 5616.0, 1336.0,0.008413,0.012240, 3946.0},         
								{3.469, 3.907, 5725.0, 1461.0,0.008829,0.012750, 3785.0},         
								{3.519, 3.963, 6065.0, 1243.0,0.007782,0.013260, 3650.0},         
								{3.140, 3.535, 6288.0, 1372.0,0.007361,0.013770, 3453.0},         
								{3.553, 4.004, 6205.0,  555.1,0.008763,0.014280, 3297.0},         
								{3.696, 4.175, 4673.0,  387.8,0.021880,0.014790, 3174.0},         
								{4.210, 4.750, 6953.0,  295.2,0.006809,0.015300, 3194.0},         
								{5.041, 5.697, 7173.0,  202.6,0.006725,0.015810, 3154.0},         
								{5.554, 6.300, 6496.0,  110.0,0.009689,0.016320, 3097.0},         
								{5.323, 6.012, 7611.0,  292.5,0.006447,0.016830, 3024.0},         
								{5.874, 6.656, 7395.0,  117.5,0.007684,0.017340, 3006.0},         
								{5.611, 6.335, 8046.0,  365.2,0.006244,0.017850, 2928.0},         
								{6.411, 7.250, 8262.0,  220.0,0.006087,0.018360, 2855.0},         
								{5.694, 6.429, 8478.0,  292.9,0.006087,0.018860, 2855.0},         
								{6.339, 7.159, 8693.0,  330.3,0.006003,0.019370, 2815.0},         
								{6.407, 7.234, 8907.0,  367.8,0.005889,0.019880, 2762.0},         
								{6.734, 7.603, 9120.0,  405.2,0.005765,0.020390, 2700.0},         
								{6.902, 7.791, 9333.0,  442.7,0.005587,0.020900, 2621.0},         
								{6.425, 7.248, 9545.0,  480.2,0.005367,0.021410, 2517.0},         
								{6.799, 7.671, 9756.0,  517.6,0.005315,0.021920, 2493.0},         
								{6.108, 6.887, 9966.0,  555.1,0.005151,0.022430, 2416.0},         
								{5.924, 6.677,10180.0,  592.5,0.004919,0.022940, 2307.0},         
								{5.238, 5.900,10380.0,  630.0,0.004758,0.023450, 2231.0},         
								{5.623, 6.354, 7160.0,  337.6,0.013940,0.023960, 2193.0},         
								{5.814, 6.554,10800.0,  355.5,0.004626,0.024470, 2170.0},         
								{6.230, 7.024,11010.0,  370.9,0.004540,0.024980, 2129.0},         
								{6.410, 7.227,11210.0,  386.4,0.004474,0.025490, 2099.0},         
								{7.500, 8.480, 8608.0,  348.0,0.009074,0.026000, 2069.0},         
								{6.979, 7.871,11620.0,  392.4,0.004402,0.026510, 2065.0},         
								{7.725, 8.716,11830.0,  394.8,0.004376,0.027020, 2052.0},         
								{8.231, 9.289,12030.0,  397.3,0.004384,0.027530, 2056.0},         
								{7.287, 8.218,12230.0,  399.7,0.004447,0.028040, 2086.0},         
								{7.899, 8.911,12430.0,  402.1,0.004511,0.028550, 2116.0},         
								{8.041, 9.071,12630.0,  404.5,0.004540,0.029060, 2129.0},         
								{7.489, 8.444,12830.0,  406.9,0.004420,0.029570, 2073.0},         
								{7.291, 8.219,13030.0,  409.3,0.004298,0.030080, 2016.0},         
								{7.098, 8.000,13230.0,  411.8,0.004182,0.030590, 1962.0},         
								{6.910, 7.786,13430.0,  414.2,0.004058,0.031100, 1903.0},         
								{6.728, 7.580,13620.0,  416.6,0.003976,0.031610, 1865.0},         
								{6.551, 7.380,13820.0,  419.0,0.003877,0.032120, 1819.0},         
								{6.739, 7.592,14020.0,  421.4,0.003863,0.032630, 1812.0},         
								{6.212, 7.996,14210.0,  423.9,0.003725,0.033140, 1747.0},         
								{5.517, 6.210,14440.0,  426.3,0.003632,0.033650, 1703.0},         
								{5.219, 5.874,14600.0,  428.7,0.003498,0.034160, 1640.0},         
								{5.071, 5.706,14790.0,  433.0,0.003405,0.034670, 1597.0},         
								{4.926, 5.542,14980.0,  433.5,0.003342,0.035180, 1567.0},         
								{4.787, 5.386,15170.0,  435.9,0.003292,0.035690, 1544.0},         
								{4.893, 5.505,15360.0,  438.4,0.003243,0.036200, 1521.0},         
								{5.028, 5.657,15550.0,  440.8,0.003195,0.036710, 1499.0},         
								{4.738, 5.329,15740.0,  443.2,0.003186,0.037220, 1494.0},         
								{4.574, 5.144,15930.0,  442.4,0.003144,0.037730, 1475.0},         
								{5.200, 5.851,16120.0,  441.6,0.003122,0.038240, 1464.0},         
								{5.070, 5.704,16300.0,  440.9,0.003082,0.038750, 1446.0},         
								{4.945, 5.563,16490.0,  440.1,0.002965,0.039260, 1390.0},         
								{4.476, 5.034,16670.0,  439.3,0.002871,0.039770, 1347.0},         
								{4.856, 5.460,18320.0,  438.5,0.002542,0.040280, 1354.0},         
								{4.308, 4.843,17040.0,  487.8,0.002882,0.040790, 1352.0},         
								{4.723, 5.311,17220.0,  537.0,0.002913,0.041300, 1366.0},         
								{5.319, 5.982,17400.0,  586.3,0.002871,0.041810, 1347.0},         
								{5.956, 6.700,17800.0,  677.0,0.002660,0.042320, 1336.0},         
								{6.158, 6.928,17770.0,  586.3,0.002813,0.042830, 1319.0},         
								{6.204, 6.979,17950.0,  586.3,0.002776,0.043340, 1302.0},         
								{6.181, 6.954,18120.0,  586.3,0.002748,0.043850, 1289.0},         
								{6.949, 7.820,18300.0,  586.3,0.002737,0.044360, 1284.0},         
								{7.506, 8.448,18480.0,  586.3,0.002727,0.044870, 1279.0},         
								{7.649, 8.609,18660.0,  586.3,0.002697,0.045380, 1265.0},         
								{7.710, 8.679,18830.0,  586.3,0.002641,0.045890, 1239.0},         
								{7.407, 8.336,19010.0,  586.3,0.002603,0.046400, 1221.0},         
								{7.290, 8.204,19180.0,  586.3,0.002573,0.046910, 1207.0}};

	return res[z-1][index-1];
}

double Reconstruction::a_he(int index, int z) {
	double res [92][9] = {{ 0.9661,  0.4126,  6.9200,  8.8310,  2.5820, 2.371000, 0.546200,-0.079320,-0.006853},	      
								 { 2.0270,  0.2931, 26.3400,  6.6600,  0.3409, 2.809000, 0.484700,-0.087560,-0.007281},
								 { 1.4200,  0.4900, 12.2500, 32.0000,  9.1610, 3.095000, 0.443400,-0.092590,-0.007459},
								 { 2.2060,  0.5100, 15.3200,  0.2500,  8.9950, 3.280000, 0.418800,-0.095640,-0.007604},
								 { 3.6910,  0.4128, 18.4800, 50.7200,  9.0000, 3.426000, 0.400000,-0.097960,-0.007715},
								 { 4.2320,  0.3877, 22.9900, 35.0000,  7.9930, 3.588000, 0.392100,-0.099350,-0.007804},
								 { 2.5100,  0.4752, 38.2600, 13.0200,  1.8920, 3.759000, 0.409400,-0.096460,-0.007661},
								 { 1.7660,  0.5261, 37.1100, 15.2400,  2.8040, 3.782000, 0.373400,-0.101100,-0.007874},
								 { 1.5330,  0.5310, 40.4400, 18.4100,  2.7180, 3.816000, 0.350400,-0.104600,-0.008074},
								 { 1.1830,  0.5500, 39.8300, 17.4900,  4.0010, 3.863000, 0.334200,-0.107200,-0.008231},
								 { 9.8940,  0.3081, 23.6500,  0.3840, 92.9300, 3.898000, 0.319100,-0.108600,-0.008271},	      
								 { 4.3000,  0.4700, 34.3000,  3.3000, 12.7400, 3.961000, 0.314000,-0.109100,-0.008297},
								 { 2.5000,  0.6250, 45.7000,  0.1000,  4.3590, 4.024000, 0.311300,-0.109300,-0.008306},
								 { 2.1000,  0.6500, 49.3400,  1.7880,  4.1330, 4.077000, 0.307400,-0.108900,-0.008219},
								 { 1.7290,  0.6562, 53.4100,  2.4050,  3.8450, 4.124000, 0.302300,-0.109400,-0.008240},
								 { 1.4020,  0.6791, 58.9800,  3.5280,  3.2110, 4.164000, 0.296400,-0.110100,-0.008267},
								 { 1.1170,  0.7044, 69.6900,  3.7050,  2.1560, 4.210000, 0.293600,-0.110300,-0.008270},
								 { 0.9172,  0.7240, 79.4400,  3.6480,  1.6460, 4.261000, 0.299400,-0.108500,-0.008145},
								 { 8.5540,  0.3817, 83.6100, 11.8400,  1.8750, 4.300000, 0.290300,-0.110300,-0.008259},
								 { 6.2970,  0.4622, 65.3900, 10.1400,  5.0360, 4.344000, 0.289700,-0.110200,-0.008245},
								 { 5.3070,  0.4918, 61.7400, 12.4000,  6.6650, 4.327000, 0.270700,-0.112700,-0.008370},	      
								 { 4.7100,  0.5087, 65.2800,  8.8060,  5.9480, 4.340000, 0.261800,-0.113800,-0.008420},
								 { 6.1510,  0.4524, 83.0000, 18.3100,  2.7100, 4.361000, 0.255900,-0.114500,-0.008447},
								 { 6.5700,  0.4322, 84.7600, 15.5300,  2.7790, 4.349000, 0.240000,-0.116600,-0.008550},
								 { 5.7380,  0.4492, 84.6100, 14.1800,  3.1010, 4.362000, 0.232700,-0.117400,-0.008588},
								 { 5.0310,  0.4707, 85.5800, 16.5500,  3.2110, 4.375000, 0.225300,-0.118500,-0.008648},
								 { 4.3200,  0.4947, 76.1400, 10.8500,  5.4410, 4.362000, 0.206900,-0.121400,-0.008815},
								 { 4.6520,  0.4571, 80.7300, 22.0000,  4.9520, 4.346000, 0.185700,-0.124900,-0.009021},
								 { 3.1140,  0.5236, 76.6700,  7.6200,  6.3850, 4.355000, 0.180000,-0.125500,-0.009045},
								 { 3.1140,  0.5236, 76.6700,  7.6200,  7.5020, 4.389000, 0.180600,-0.125300,-0.009028},
								 { 3.1140,  0.5236, 76.6700,  7.6200,  8.5140, 4.407000, 0.175900,-0.125800,-0.009054},	      
								 { 5.7460,  0.4662, 79.2400,  1.1850,  7.9930, 4.419000, 0.169400,-0.126700,-0.009094},
								 { 2.7920,  0.6346,106.1000,  0.2986,  2.3310, 4.412000, 0.154500,-0.128900,-0.009202},
								 { 4.6670,  0.5095,124.3000,  2.1020,  1.6670, 4.419000, 0.144800,-0.130300,-0.009269},
								 { 2.4400,  0.6346,105.0000,  0.8300,  2.8510, 4.436000, 0.144300,-0.129900,-0.009229},
								 { 1.4910,  0.7118,120.6000,  1.1010,  1.8770, 4.478000, 0.160800,-0.126200,-0.008983},
								 {11.7200,  0.3826,102.8000,  9.2310,  4.3710, 4.489000, 0.151700,-0.127800,-0.009078},
								 { 7.1260,  0.4804,119.3000,  5.7840,  2.4540, 4.514000, 0.155100,-0.126800,-0.009005},
								 {11.6100,  0.3955,146.7000,  7.0310,  1.4230, 4.533000, 0.156800,-0.126100,-0.008945},
								 {10.9900,  0.4100,163.9000,  7.1000,  1.0520, 4.548000, 0.157200,-0.125600,-0.008901},
								 { 9.2410,  0.4275,163.1000,  7.9540,  1.1020, 4.553000, 0.154400,-0.125500,-0.008883},	      
								 { 9.2760,  0.4180,157.1000,  8.0380,  1.2900, 4.548000, 0.148500,-0.125900,-0.008889},
								 { 3.9990,  0.6152, 97.6000,  1.2970,  5.7920, 4.489000, 0.112800,-0.130900,-0.009107},
								 { 4.3060,  0.5658, 97.9900,  5.5140,  5.7540, 4.402000, 0.066560,-0.137500,-0.009421},
								 { 3.6150,  0.6197, 86.2600,  0.3330,  8.6890, 4.292000, 0.010120,-0.145900,-0.009835},
								 { 5.8000,  0.4900,147.2000,  6.9030,  1.2890, 4.187000,-0.045390,-0.154200,-0.010250},
								 { 5.6000,  0.4900,130.0000, 10.0000,  2.8440, 4.577000, 0.130000,-0.128500,-0.009067},
								 { 3.5500,  0.6068,124.7000,  1.1120,  3.1190, 4.583000, 0.125300,-0.129100,-0.009084},
								 { 3.6000,  0.6200,105.8000,  0.1692,  6.0260, 4.580000, 0.117400,-0.130100,-0.009129},
								 { 5.4000,  0.5300,103.1000,  3.9310,  7.7670, 4.581000, 0.111000,-0.130900,-0.009161},
								 { 3.9700,  0.6459,131.8000,  0.2233,  2.7230, 4.582000, 0.104600,-0.131700,-0.009193},	      
								 { 3.6500,  0.6400,126.8000,  0.6834,  3.4110, 4.600000, 0.105200,-0.131500,-0.009178},
								 { 3.1180,  0.6519,164.9000,  1.2080,  1.5100, 4.614000, 0.104300,-0.131500,-0.009175},
								 { 2.0310,  0.7181,153.1000,  1.3620,  1.9580, 4.619000, 0.097690,-0.132500,-0.009231},
								 {14.4000,  0.3923,152.5000,  8.3540,  2.5970, 4.671000, 0.113600,-0.129800,-0.009078},
								 {10.9900,  0.4599,138.4000,  4.8110,  3.7260, 4.706000, 0.120600,-0.128700,-0.009009},
								 {16.6000,  0.3773,224.1000,  6.2800,  0.9121, 4.732000, 0.124400,-0.128000,-0.008968},
								 {10.5400,  0.4533,159.3000,  4.8320,  2.5290, 4.722000, 0.115600,-0.129200,-0.009030},
								 {10.3300,  0.4502,162.0000,  5.1320,  2.4440, 4.710000, 0.106000,-0.130500,-0.009100},
								 {10.1500,  0.4471,165.6000,  5.3780,  2.3280, 4.698000, 0.096470,-0.131900,-0.009169},
								 { 9.9760,  0.4439,168.0000,  5.7210,  2.2580, 4.681000, 0.085360,-0.133500,-0.009252},	      
								 { 9.8040,  0.4408,176.2000,  5.6750,  1.9970, 4.676000, 0.078190,-0.134500,-0.009302},
								 {14.2200,  0.3630,228.4000,  7.0240,  1.0160, 4.663000, 0.068670,-0.135800,-0.009373},
								 { 9.9520,  0.4318,233.5000,  5.0650,  0.9244, 4.676000, 0.068610,-0.135700,-0.009363},
								 { 9.2720,  0.4345,210.0000,  4.9110,  1.2580, 4.649000, 0.053620,-0.137900,-0.009480},
								 {10.1300,  0.4146,225.7000,  5.5250,  1.0550, 4.634000, 0.043350,-0.139400,-0.009558},
								 { 8.9490,  0.4304,213.3000,  5.0710,  1.2210, 4.603000, 0.026790,-0.141800,-0.009690},
								 {11.9400,  0.3783,247.2000,  6.6550,  0.8490, 4.584000, 0.014940,-0.143600,-0.009783},
								 { 8.4720,  0.4405,195.5000,  4.0510,  1.6040, 4.576000, 0.007043,-0.144700,-0.009841},
								 { 8.3010,  0.4399,203.7000,  3.6670,  1.4590, 4.571000, 0.000705,-0.145600,-0.009886},
								 { 6.5670,  0.4858,193.0000,  2.6500,  1.6600, 4.566000,-0.005626,-0.146400,-0.009930},	      
								 { 5.9510,  0.5016,196.1000,  2.6620,  1.5890, 4.561000,-0.011970,-0.147300,-0.009975},
								 { 7.4950,  0.4523,251.4000,  3.4330,  0.8619, 4.572000,-0.012000,-0.147200,-0.009965},
								 { 6.3350,  0.4825,255.1000,  2.8340,  0.8228, 4.569000,-0.017550,-0.148000,-0.010000},
								 { 4.3140,  0.5558,214.8000,  2.3540,  1.2630, 4.573000,-0.019920,-0.148200,-0.010010},
								 { 4.0200,  0.5681,219.9000,  2.4020,  1.1910, 4.570000,-0.025470,-0.149000,-0.010050},
								 { 3.8360,  0.5765,210.2000,  2.7420,  1.3050, 4.528000,-0.046130,-0.152100,-0.010220},
								 { 4.6800,  0.5247,244.7000,  2.7490,  0.8962, 4.494000,-0.063700,-0.154800,-0.010370},
								 { 3.2230,  0.5883,232.7000,  2.9540,  1.0500, 4.564000,-0.027000,-0.147100,-0.009852},
								 { 2.8920,  0.6204,208.6000,  2.4150,  1.4160, 4.546000,-0.049630,-0.152300,-0.010220},
								 { 4.7280,  0.5522,217.0000,  3.0910,  1.3860, 4.594000,-0.033390,-0.149600,-0.010060},	      
								 { 6.1800,  0.5200,170.0000,  4.0000,  3.2240, 4.608000,-0.028860,-0.148500,-0.009990},
								 { 9.0000,  0.4700,198.0000,  3.8000,  2.0320, 4.624000,-0.026390,-0.148100,-0.009971},
								 { 2.3240,  0.6997,216.0000,  1.5990,  1.3990, 4.636000,-0.024220,-0.147700,-0.009939},
								 { 1.9610,  0.7286,223.0000,  1.6210,  1.2960, 4.648000,-0.021720,-0.147100,-0.009903},
								 { 1.7500,  0.7427,350.1001,  0.9789,  0.5507, 4.662000,-0.119200,-0.175200,-0.011960},
								 {10.3100,  0.4613,261.2000,  4.7380,  0.9899, 4.690000,-0.009867,-0.144900,-0.009771},
								 { 7.9620,  0.5190,235.7000,  4.3470,  1.3130, 4.715000,-0.002113,-0.143500,-0.009689},
								 { 6.2270,  0.5645,231.9000,  3.9610,  1.3790, 4.729000, 0.001392,-0.142800,-0.009644},
								 { 5.2460,  0.5947,228.6000,  4.0270,  1.4320, 4.729000,-0.000598,-0.143000,-0.009647},
								 { 5.4080,  0.5811,235.7000,  3.9610,  1.3580, 4.738000, 0.001075,-0.142500,-0.009618},	      
								 { 5.2180,  0.5828,245.0000,  3.8380,  1.2500, 4.751000, 0.004244,-0.141900,-0.009576}};

	return res[z-1][index-1];
}

double Reconstruction::b_he(int index, int z) {
	double res[92][5] = {{ 0.39, 0.63, 4.17, 85.55, 19.55}, 
								{ 0.58, 0.59, 6.3, 130., 44.07},    
								{ 15.23, 0.1076, 10.14, 1232., 31.24},        
								{ 2.2060,  0.5100, 15.3200,  0.2500,  8.9950},
								{ 3.6910,  0.4128, 18.4800, 50.7200,  9.0000},
								{ 3.47,0.4485, 22.37, 36.41, 7.993},
								{ 2.0, 0.548, 29.82, 18.11, 4.37},  
								{ 2.717, 0.4858, 32.88, 25.88, 4.336},        
								{ 2.616, 0.4708, 41.2, 28.07, 2.458},         
								{ 2.303, 0.4861, 37.01, 37.96, 5.092},        
								{ 13.03, 0.2685, 35.65, 44.18, 9.175},       
								{ 4.3000,  0.4700, 34.3000,  3.3000, 12.7400},
								{ 2.5000,  0.6250, 45.7000,  0.1000,  4.3590},
								{ 2.1000,  0.6500, 49.3400,  1.7880,  4.1330},
								{ 1.7290,  0.6562, 53.4100,  2.4050,  4.8450},
								{ 3.116,   0.5988, 53.71,    5.632,    4.536},        
								{ 3.365,   0.571,  63.67,    6.182,    2.969},        
								{ 2.291,   0.6284, 73.88,    4.478,    2.066},        
								{ 16.6,    0.3095, 99.1,     10.98,    1.092},        
								{ 6.2970,  0.4622, 65.3900, 10.1400,  5.0360},  
								{ 5.3070,  0.4918, 61.7400, 12.4000,  6.6650},         
								{ 4.7100,  0.5087, 65.2800,  8.8060,  5.9480},
								{ 6.1510,  0.4524, 83.0000, 18.3100,  2.7100},
								{ 6.5700,  0.4322, 84.7600, 15.5300,  2.7790},
								{ 5.7380,  0.4492, 84.6100, 14.1800,  3.1010},
								{ 5.0310,  0.4707, 85.5800, 16.5500,  3.2110},
								{ 4.3200,  0.4947, 76.1400, 10.8500,  5.4410},
								{ 4.6520,  0.4571, 80.7300, 22.0000,  4.9520},
								{ 3.1140,  0.5236, 76.6700,  7.6200,  6.3850},
								{ 3.1140,  0.5236, 76.6700,  7.6200,  7.5020},
								{ 3.1140,  0.5236, 76.6700,  7.6200,  8.5140},         
								{ 5.7460,  0.4662, 79.2400,  1.1850,  7.9930},
								{ 2.7920,  0.6346,106.1000,  0.2986,  2.3310},
								{ 4.6670,  0.5095,124.3000,  2.1020,  1.6670},
								{ 1.65,    0.7,    148.1,      1.47,  0.9686},       
								{ 1.413,   0.7377, 147.9,     1.466,  1.0160},        
								{11.7200,  0.3826,102.8000,  9.0000,  4.3710},
								{ 7.1260,  0.4804,119.3000,  5.7840,  2.4540},
								{11.6100,  0.3955,146.7000,  7.0310,  1.4230},
								{10.9900,  0.4100,163.9000,  7.1000,  1.0520},
								{ 9.2410,  0.4275,163.1000,  7.9540,  1.1020},         
								{ 9.2760,  0.4180,157.1000,  8.0380,  1.2900},
								{ 3.9990,  0.6152, 97.6000,  1.2970,  5.7920},
								{ 4.3020,  0.5658, 97.9900,  5.5140,  5.7540},
								{ 3.6150,  0.6197, 86.2600,  0.3330,  8.6890},
								{ 5.8000,  0.4900,147.2000,  6.9030,  1.2890},
								{ 5.6000,  0.4900,130.0000, 10.0000,  2.8440},
								{ 3.5500,  0.6068,124.7000,  1.1120,  3.1190},
								{ 3.6000,  0.6200,105.8000,  0.1692,  6.0260},
								{ 5.4000,  0.5300,103.1000,  3.9310,  7.7670},
								{ 3.9700,  0.6459,131.8000,  0.2233,  2.7230},         
								{ 3.6500,  0.6400,126.8000,  0.6834,  3.4110},
								{ 4.13,    0.6177, 152.0,    2.516,   1.9380},        
								{ 3.949,   0.6209, 200.5,    1.878,   0.9126},       
								{ 25.94,   0.3139, 335.1,    2.946,   0.3347},       
								{710.9900,  0.4599,138.4000, 4.8110,  3.7260},
								{16.6000,  0.3773,224.1000,  6.2800,  0.9121},
								{10.5400,  0.4533,159.3000,  4.8320,  2.5290},
								{10.3300,  0.4502,162.0000,  5.1320,  2.4440},
								{10.1500,  0.4471,165.6000,  5.3780,  2.3280},
								{ 9.9760,  0.4439,168.0000,  5.7210,  2.2580},         
								{ 9.8040,  0.4408,176.2000,  5.6750,  1.9970},
								{14.2200,  0.3630,228.4000,  7.0240,  1.0160},
								{ 9.9520,  0.4318,233.5000,  5.0650,  0.9244},
								{ 9.2720,  0.4345,210.0000,  4.9110,  1.2580},
								{10.1300,  0.4146,225.7000,  5.5250,  1.0550},
								{ 8.9490,  0.4304,213.3000,  5.0710,  1.2210},
								{11.9400,  0.3783,247.2000,  6.6550,  0.8490},
								{ 8.4720,  0.4405,195.5000,  4.0510,  1.6040},
								{ 8.3010,  0.4399,203.7000,  3.6670,  1.4590},
								{ 6.5670,  0.4858,193.0000,  2.6500,  1.6600},         
								{ 5.9510,  0.5016,196.1000,  2.6620,  1.5890},
								{ 7.4950,  0.4523,251.4000,  3.4330,  0.8619},
								{ 6.3350,  0.4825,255.1000,  2.8340,  0.8228},
								{ 4.3140,  0.5558,214.8000,  2.3540,  1.2630},
								{ 4.0200,  0.5681,219.9000,  2.4020,  1.1910},
								{ 3.8360,  0.5765,210.2000,  2.7420,  1.3050},
								{ 4.6800,  0.5247,244.7000,  2.7490,  0.9862},
								{ 3.2230,  0.5883,232.7000,  2.9540,  1.0500},
								{ 8.15,    0.4745, 269.2,    2.392,   0.7467},        
								{ 4.7280,  0.5522,217.0000,  3.0910,  1.3860},         
								{ 6.1800,  0.5200,170.0000,  4.0000,  3.2240},
								{ 9.0000,  0.4700,198.0000,  3.8000,  2.0320},
								{ 2.3240,  0.6997,216.0000,  1.5990,  1.3990},
								{ 1.9610,  0.7286,223.0000,  1.6210,  1.2960},
								{ 4.822,   0.605, 418.3,     0.8335,  0.3865},       
								{10.3100,  0.4613,261.2000,  4.7380,  0.9899},
								{ 7.9620,  0.5190,235.7000,  4.3470,  1.3130},
								{ 6.2270,  0.5645,231.9000,  3.9610,  1.3790},
								{ 5.2460,  0.5947,228.6000,  4.0270,  1.4320},
								{ 5.4080,  0.5811,235.7000,  3.9610,  1.3580},         
								{ 5.2180,  0.5828,245.0000,  3.8380,  1.2500}};

	return res[z-1][index-1];
}

double Reconstruction::shell_correction(int z) {
	//      SHELL CORRECTIONS FOR BETHE FORMULA
	//      FOR HYDROGEN AS PROJECTILE.        
	switch(z) {
		case 1:
			return 4.;
			break;
		case 2:
			return -3.;
			break;
		case 3:
			return -9.;
			break;
		case 4:
			return -8.;
			break;
		case 5:
			return -7.;
			break;
		case 6:
			return -9.;
			break;
		case 7:
			return -6.;
			break;
		case 8:
			return -9.;
			break;
		case 9:
			return -10.;     
			break;
		case 10:
			return -11.;    
			break;
		default:
			if(11 <= z && z <= 50)
			{
				return (-21.258 + 2.1282*z - 0.099659*z*z + 0.0011623*z*z*z + 0.000024822*z*z*z*z - 0.0000004405*z*z*z*z*z);
			}
			else if(51 <= z && z <= 92)
			{
				return (1089.1  - 85.686*z + 2.6457*z*z   - 0.040082*z*z*z  + 0.0002976*z*z*z*z   - 0.00000086478*z*z*z*z*z);
			}
			break;
	}
	return -1.;
}
