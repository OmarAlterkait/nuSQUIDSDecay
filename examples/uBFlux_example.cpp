/*========================="Couplings" Example==========================//
Below is an example implementation of the NuSQuIDSDecay class.
An example neutrino flux (kaon/pion) is read in over a specified
range and binning in cos(zenith angle) and energy. NuSQuIDSDecay
is then used to evolve this flux through the earth to the South
Pole. Both the initial and final fluxes are written to text files
which can be used to produce oscillograms.
	The neutrinos here are majorana, and incoherent interactions,
tau regeneration, and decay regeneration effects are all being
simulated. We consider a simplified decay scenario where
all mass states except m_4 are stable, the only
decay channel is from m_4 to m_3, and the only non-zero mixing
angle between the light mass states and m_4 is theta_24. The phi
mass is always assumed to be zero. All m_4->m_3 decay processes
({CPP,CVP}) are allowed, but they are
computed internally by NuSQuIDSDecay as functions of the lagrangian
coupling matrix g_ij which we supply to the constructor. The couplings
here are set to be scalar, though the user is free to specify pseudoscalar
couplings as an alternative (recall that this simulation applies to
pure scalar or pure pseudoscalar couplings, and not to mixtures).
The coupling constructor assumes majorana neutrinos
automatically, so we do not need to set the majorana flag.
//==========================================================================*/

#include <vector>
#include <iostream>
#include <string>
#include <limits>
#include <sstream>
#include <iomanip>
#include <nuSQuIDS/nuSQuIDS.h>
#include <nuSQuIDS/marray.h>
#include <nuSQuIDS/tools.h>
#include "nusquids_decay.h"

using namespace nusquids;

bool progressbar = 1; //show progress bar
double error = 1.0e-15;
double density = 5.0; // gr/cm^3
double ye = 0.3; //dimensionless
double baseline = 0.47; //km
double hstep = baseline/2000; // km, integration step size
double hmax = baseline/100; //km, maximum integration step size

//Convenience function to write initial and final fluxes to a text file.
void WriteFlux(std::shared_ptr<nuSQUIDSDecay> nusquids, std::string fname){
	enum {NU_E,NU_MU,NU_TAU};
	enum {NEUTRINO,ANTINEUTRINO};
	std::cout <<"Writing Flux\n";
	std::ofstream ioutput("../output/" + fname + ".dat");
	for(double enu : nusquids->GetERange()){
		ioutput << enu << " ";
		ioutput << nusquids->EvalFlavor(NU_MU,enu,NEUTRINO) << " ";
		ioutput << nusquids->EvalFlavor(NU_MU,enu,ANTINEUTRINO) << " ";
		ioutput << std::endl;
	}
	ioutput.close();
	std::cout << "Wrote Flux\n";
}

//Convenience function to read a flux file into an marray input for nuSQuIDS.
void ReadFlux(std::shared_ptr<nuSQUIDSDecay> nusquids, marray<double,3>& inistate, std::string type,
				std::string input_flux_path, std::string modelname, double GeV){

	std::fill(inistate.begin(),inistate.end(),0);
	// read file
	// marray<double,2> input_flux = quickread(input_flux_path + "/" + "initial_"+ type + "_atmopheric_" + modelname + ".dat");
	marray<double,2> input_flux = quickread(input_flux_path + "/" + "MicroBooNE_SQuIDSFormat_Flux_NumuAndAntiNuMu.dat");
	// marray<double,2> input_flux = quickread(input_flux_path + "/" + "initial_pion_atmopheric_PolyGonato_QGSJET-II-04.dat");


	marray<double,1> e_range = nusquids->GetERange();
	
	for ( int ei = 0 ; ei < nusquids->GetNumE(); ei++){
		double enu = e_range[ei]/GeV;

		inistate[ei][0][0] = 0.;
		inistate[ei][0][1] = input_flux[ei][1];
		inistate[ei][0][2] = 0.;
		inistate[ei][0][3] = 0.;

		inistate[ei][1][0] = 0.;
		inistate[ei][1][1] = input_flux[ei][2];
		inistate[ei][1][2] = 0.;
		inistate[ei][1][3] = 0.;
	}
}

//====================================MAIN=========================================//

int main(int argc, char** argv){
	bool oscillogram = true;
	bool quiet = false;
	// getting input parameters
	double nu4mass, theta24, coupling;
	if (argc>=4){
	  nu4mass = std::stof(argv[1]);
	  theta24 = std::stof(argv[2]);
	  coupling = std::stof(argv[3]);
	  std::cout << "nu4mass = " << nu4mass << '\n';
	  std::cout << "theta24 = " << theta24 << '\n';
	  std::cout << "coupling = " << coupling << '\n';
	}
	else {
	  nu4mass = 1.0; //Set the mass of the sterile neutrino (eV)
	  theta24 = 1.0; //Set the mixing angle [rad] between sterile and tau flavors.
	  //Set coupling (we are assuming m4->m3 decay only for simplicity).
	  coupling=1.0;
	}

	//Toggle, incoherent interactions scalar/pseudoscalar, and decay regeneration.
	bool iinteraction=true;
        bool decay_regen=true;
	bool pscalar=false;

	//Path for input fluxes
	std::string input_flux_path = "../fluxes";

	//Flux model
	// const std::string modelname = "PolyGonato_QGSJET-II-04";
	const std::string modelname = "PolyGonato_QGSJET-II-04";

	// oscillation physics parameters and nusquids setup
	// Note: only m_1 may be set to zero. Our computations
	// do not apply if more than one neutrino mass is zero!
	double dm41sq = nu4mass*nu4mass; // assume m_1 is massless
	const unsigned int numneu = 4;
	const squids::Const units;
	double m1 = 0.0;
	double m2 = sqrt(7.65e-05);
	double m3 = sqrt(0.0024);
	double m4 = nu4mass;
	std::vector<double> nu_mass{m1,m2,m3,m4};

	//Allocate memory for rate matrices
	gsl_matrix* couplings;
	couplings = gsl_matrix_alloc(numneu,numneu);
	gsl_matrix_set_zero(couplings);

	//Set coupling (43 nonzero).
	gsl_matrix_set(couplings,3,2,coupling); //g_43

	//Declare NuSQuIDSDecay objects. They are declared within a NuSQuIDSAtm wrapper to incorporate atmospheric simulation.
	//Here, we use the partial rate constructor of NuSQuIDSDecay. One object is created for the kaon flux component, and
	//one for the pion flux component.
	//The first two arguments (linspaces) define ranges of cos(zenith) and energy over which to simulate, respectively.
	//The cos(zenith) argument is passed to the wrapping class. The arguments to nuSQUIDSDecay begin at the energy argument.
	if(!quiet){std::cout << "Declaring nuSQuIDSDecay atmospheric objects" << std::endl;}
	std::shared_ptr<nuSQUIDSDecay> nusquids_pion = std::make_shared<nuSQUIDSDecay>(linspace(2.5e-2*units.GeV,9.975e0*units.GeV,200),numneu,both,iinteraction,
																decay_regen,pscalar,nu_mass,couplings);



	const double layer = baseline*units.km;
  std::shared_ptr<ConstantDensity> constdens = std::make_shared<ConstantDensity>(density,ye); // density [gr/cm^3[, ye [dimensionless]
  std::shared_ptr<ConstantDensity::Track> track = std::make_shared<ConstantDensity::Track>(layer);	


	//Include tau regeneration in simulation.	
	nusquids_pion->Set_TauRegeneration(true);

	//Include tau regeneration in simulation.
	nusquids_pion->Set_Body(constdens);
	nusquids_pion->Set_Track(track);


	//Set mixing angles and masses.
	nusquids_pion->Set_MixingAngle(0,1,0.563942);
	nusquids_pion->Set_MixingAngle(0,2,0.154085);
	nusquids_pion->Set_MixingAngle(1,2,0.785398);
	nusquids_pion->Set_MixingAngle(0,3,0.0);
	nusquids_pion->Set_MixingAngle(1,3,theta24);
	nusquids_pion->Set_MixingAngle(2,3,0.0);

	nusquids_pion->Set_SquareMassDifference(1,7.65e-05);
	nusquids_pion->Set_SquareMassDifference(2,0.00247);
	nusquids_pion->Set_SquareMassDifference(3,dm41sq);
	nusquids_pion->Set_CPPhase(0,2,0.0);
	nusquids_pion->Set_CPPhase(0,3,0.0);
	nusquids_pion->Set_CPPhase(1,3,0.0);

	//Setup integration settings
	nusquids_pion->Set_GSL_step(gsl_odeiv2_step_rkf45);
	nusquids_pion->Set_rel_error(error);
	nusquids_pion->Set_abs_error(error);
	nusquids_pion->Set_h(hstep*units.km); //initial integration step size
	nusquids_pion->Set_h_max(hmax*units.km); //maximum integration step size
	nusquids_pion->Set_ProgressBar(1);

	std::ostringstream tempObj;
	// Set Fixed -Point Notation
	tempObj << std::fixed;
	// Set precision to 2 digits
	tempObj << std::setprecision(3);

	std::string outstr = "ub_final";
        outstr += "_m";
	tempObj << nu4mass;
        outstr += tempObj.str();
        outstr += "_t";
	tempObj.str(std::string());
	tempObj << theta24;
        outstr += tempObj.str();
        outstr += "_c";
	tempObj.str(std::string());
	tempObj << coupling;
        outstr += tempObj.str();
        std::cout << outstr << '\n';




	if(!quiet)
		std::cout << "Setting up the initial fluxes for the nuSQuIDSDecay objects." << std::endl;

	//Read pion flux and initialize nusquids object with it.
	marray<double,3> inistate_pion {nusquids_pion->GetNumE(),2,numneu};
	std::cout << "Made Object\n";
	ReadFlux(nusquids_pion,inistate_pion,std::string("NOT_USED"),input_flux_path,modelname,units.GeV);
	std::cout << "Read Object\n";
	nusquids_pion->Set_initial_state(inistate_pion,flavor);
	std::cout << "Initial State Set\n";
	//Write initial flux to text file.
	if(oscillogram){WriteFlux(nusquids_pion, std::string("ub_initial"));}
	std::cout << "Wrote Initial\n";
	//Evolve flux through the earth.
	if(!quiet){std::cout << "Evolving the pion fluxes." << std::endl;}
	nusquids_pion->EvolveState();
	//Write final flux to text file.
	if(oscillogram){WriteFlux(nusquids_pion, outstr);}
	std::cout << "Wrote Final\n";
	//Free memory for couplings
	gsl_matrix_free(couplings);
	return 0;
}
