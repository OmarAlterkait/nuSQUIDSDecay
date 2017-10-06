#include <vector>
#include <iostream>
#include <string>
#include <limits>
#include <nuSQuIDS/nuSQuIDS.h>
#include <nuSQuIDS/marray.h>
#include <nuSQuIDS/tools.h>
#include "nusquids_decay.h"

using namespace nusquids;

int main(int argc, char* argv[])
{

  squids::Const units;
  const unsigned int e = 0;
  const unsigned int mu = 1;
  const unsigned int tau = 2;
  const unsigned int numneu = 4;

  //nusquids::marray<double,1> e_nodes = logspace(1.0e2*units.GeV,1.0e5*units.GeV,100);
  nusquids::marray<double,1> e_nodes = logspace(1.0e2*units.GeV,1.0e3*units.GeV,10);

  nuSQUIDSDecay nusqdec(e_nodes,numneu);


	double tolerance=1.0e-16;
	// setup integration settings
	nusqdec.Set_rel_error(tolerance);
	nusqdec.Set_abs_error(tolerance);

  std::shared_ptr<EarthAtm> body = std::make_shared<EarthAtm>();
  std::shared_ptr<EarthAtm::Track> track = std::make_shared<EarthAtm::Track>(acos(-1.));

  //std::shared_ptr<Vacuum> body = std::make_shared<Vacuum>();
  //std::shared_ptr<Vacuum::Track> track = std::make_shared<Vacuum::Track>(12000.*units.km);
  //std::shared_ptr<Vacuum::Track> track = std::make_shared<Vacuum::Track>(2.0*6371.0*units.km);

  nusqdec.Set_Body(body);
  nusqdec.Set_Track(track);

  marray<double,3> neutrino_state({e_nodes.size(),2,nusqdec.GetNumNeu()});
  std::fill(neutrino_state.begin(),neutrino_state.end(),0);

  //fill this with a real neutrino flux later
  for(size_t ie=0; ie<neutrino_state.extent(0); ie++){
    for(size_t ir=0; ir<neutrino_state.extent(1); ir++){
      for(size_t iflv=0; iflv<neutrino_state.extent(2); iflv++){

        neutrino_state[ie][ir][iflv] = (iflv == mu) ? 1.: 0;
		//Dummy spectrum w/ soft tail.
		//neutrino_state[ie][ir][iflv]*= (1.0/e_nodes[ie]);
        //neutrino_state[ie][ir][iflv] = (iflv == mu) ? 1.: 0;
      }
    }
  }

  double m1 = 0.0;
  double m2 = sqrt(nusqdec.Get_SquareMassDifference(1));
  double m3 = sqrt(nusqdec.Get_SquareMassDifference(2));
  double m4 = 1.0;
  double mphi = 0.0;

  std::vector<double> nu_mass(numneu);

  nu_mass[0]=m1;	
  nu_mass[1]=m2;	
  nu_mass[2]=m3;	
  nu_mass[3]=m4;	

  nusqdec.Set_SquareMassDifference(3,m4*m4 - m1*m1);  //dm^2_41

	//std::cout << "dm2_21: " << nusqdec.Get_SquareMassDifference(1) << std::endl;
	//std::cout << "dm2_31: " << nusqdec.Get_SquareMassDifference(2) << std::endl;
	//std::cout << "dm2_41: " << nusqdec.Get_SquareMassDifference(3) << std::endl;



  nusqdec.Set_MixingParametersToDefault();

  // mixing angles
  //nusqdec.Set_MixingAngle(0,1,0.563942);
  //nusqdec.Set_MixingAngle(0,2,0.154085);
  //nusqdec.Set_MixingAngle(1,2,0.785398);
  nusqdec.Set_MixingAngle(0,3,0.785398);
  nusqdec.Set_MixingAngle(1,3,0.785398);
  nusqdec.Set_MixingAngle(2,3,0.785398);

	/*
  for (int row=0; row<numneu; row++)
	{
  	for (int col=row+1; col<numneu; col++)
		{
			std::cout << "(" << row << "," << col << "): " << nusqdec.Get_MixingAngle(row,col) << std::endl;	
		}
	}
	*/

	
  nusqdec.Set_m_phi(mphi);
  nusqdec.Set_m_nu(m1, 0);
  nusqdec.Set_m_nu(m2, 1);
  nusqdec.Set_m_nu(m3, 2);
  nusqdec.Set_m_nu(m4, 3);

  nusqdec.Set_initial_state(neutrino_state,flavor);

	//------------------------//
	//   Physics Switches	    //
	//------------------------//

  //nusqdec.Set_ProgressBar(true);
  nusqdec.Set_IncoherentInteractions(false);  //turns off and on earth absorption, glashow, ...
  nusqdec.Set_Majorana(true);
  //nusqdec.Set_IncoherentInteractions(true);
  //nusqdec.Set_OtherRhoTerms(false); //turns on and off regeneration. 
  nusqdec.Set_OtherRhoTerms(true);

	//------------------------//

  //Set chirality-preserving scalar process lifetimes.
  gsl_matrix* cpp_scalar_tau_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_all(cpp_scalar_tau_mat, 1e60); // Set lifetimes to effective stability.
  //Setting for 4 neutrino case with stable nu_1. 
  double cpp_scalar_lifetime = 1.0e2;
  //  gsl_matrix_set(cpp_scalar_tau_mat,0,1,cpp_scalar_lifetime); //tau_21
  //  gsl_matrix_set(cpp_scalar_tau_mat,0,2,cpp_scalar_lifetime); //tau_31
  //  gsl_matrix_set(cpp_scalar_tau_mat,1,2,cpp_scalar_lifetime); //tau_32
  gsl_matrix_set(cpp_scalar_tau_mat,0,3,cpp_scalar_lifetime); //tau_41
  gsl_matrix_set(cpp_scalar_tau_mat,1,3,cpp_scalar_lifetime); //tau_42
  gsl_matrix_set(cpp_scalar_tau_mat,2,3,cpp_scalar_lifetime); //tau_43

  //Set chirality-violating scalar process lifetimes.
  gsl_matrix* cvp_scalar_tau_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_all(cvp_scalar_tau_mat, 1e60); // Set lifetimes to effective stability.
  //Setting for 4 neutrino case with stable nu_1. 
  double cvp_scalar_lifetime = 1.0e2;
  //  gsl_matrix_set(cvp_scalar_tau_mat,0,1,cvp_scalar_lifetime); //tau_21
  //  gsl_matrix_set(cvp_scalar_tau_mat,0,2,cvp_scalar_lifetime); //tau_31
  //  gsl_matrix_set(cvp_scalar_tau_mat,1,2,cvp_scalar_lifetime); //tau_32
  gsl_matrix_set(cvp_scalar_tau_mat,0,3,cvp_scalar_lifetime); //tau_41
  gsl_matrix_set(cvp_scalar_tau_mat,1,3,cvp_scalar_lifetime); //tau_42
  gsl_matrix_set(cvp_scalar_tau_mat,2,3,cvp_scalar_lifetime); //tau_43

  //Set chirality-preserving pseudoscalar process lifetimes.
  gsl_matrix* cpp_pseudoscalar_tau_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_all(cpp_pseudoscalar_tau_mat, 1e60); // Set cpp_pseudoscalar_lifetimes to effective stability.
  //Setting for 4 neutrino case with stable nu_1. 
  double cpp_pseudoscalar_lifetime = 1.0e2;
  //double cpp_pseudoscalar_lifetime = 1.0e60;
  //  gsl_matrix_set(cpp_pseudoscalar_tau_mat,0,1,cpp_pseudoscalar_lifetime); //tau_21
  //  gsl_matrix_set(cpp_pseudoscalar_tau_mat,0,2,cpp_pseudoscalar_lifetime); //tau_31
  //  gsl_matrix_set(cpp_pseudoscalar_tau_mat,1,2,cpp_pseudoscalar_lifetime); //tau_32
  gsl_matrix_set(cpp_pseudoscalar_tau_mat,0,3,cpp_pseudoscalar_lifetime); //tau_41
  gsl_matrix_set(cpp_pseudoscalar_tau_mat,1,3,cpp_pseudoscalar_lifetime); //tau_42
  gsl_matrix_set(cpp_pseudoscalar_tau_mat,2,3,cpp_pseudoscalar_lifetime); //tau_43

  //Set chirality-violating pseudoscalar process lifetimes.
  gsl_matrix* cvp_pseudoscalar_tau_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_all(cvp_pseudoscalar_tau_mat, 1e60); // Set cvp_pseudoscalar_lifetimes to effective stability.
  //Setting for 4 neutrino case with stable nu_1. 
  double cvp_pseudoscalar_lifetime = 1.0e2;
  //double cvp_pseudoscalar_lifetime = 1.0e60;
  //  gsl_matrix_set(cvp_pseudoscalar_tau_mat,0,1,cvp_pseudoscalar_lifetime); //tau_21
  //  gsl_matrix_set(cvp_pseudoscalar_tau_mat,0,2,cvp_pseudoscalar_lifetime); //tau_31
  //  gsl_matrix_set(cvp_pseudoscalar_tau_mat,1,2,cvp_pseudoscalar_lifetime); //tau_32
  gsl_matrix_set(cvp_pseudoscalar_tau_mat,0,3,cvp_pseudoscalar_lifetime); //tau_41
  gsl_matrix_set(cvp_pseudoscalar_tau_mat,1,3,cvp_pseudoscalar_lifetime); //tau_42
  gsl_matrix_set(cvp_pseudoscalar_tau_mat,2,3,cvp_pseudoscalar_lifetime); //tau_43


  gsl_matrix* cpp_scalar_decay_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_zero(cpp_scalar_decay_mat);

	double rate;
	double colrate;
	for (size_t col=0; col<numneu; col++)
	{
		colrate=0;

		for (size_t row=0; row<col; row++)
		{	
			rate = 1.0/gsl_matrix_get(cpp_scalar_tau_mat,row,col);
			gsl_matrix_set(cpp_scalar_decay_mat,row,col,rate);
			colrate+=rate*nu_mass[col];
		}

		gsl_matrix_set(cpp_scalar_decay_mat,col,col,colrate);
	}	


  gsl_matrix* cvp_scalar_decay_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_zero(cvp_scalar_decay_mat);

	for (size_t col=0; col<numneu; col++)
	{
		colrate=0;

		for (size_t row=0; row<col; row++)
		{	
			rate = 1.0/gsl_matrix_get(cvp_scalar_tau_mat,row,col);
			gsl_matrix_set(cvp_scalar_decay_mat,row,col,rate);
			colrate+=rate*nu_mass[col];
		}

		gsl_matrix_set(cvp_scalar_decay_mat,col,col,colrate);
	}	

  nusqdec.Set_Scalar_Matrices(cpp_scalar_decay_mat,cvp_scalar_decay_mat);

  gsl_matrix* cpp_pseudoscalar_decay_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_zero(cpp_pseudoscalar_decay_mat);

	for (size_t col=0; col<numneu; col++)
	{
		colrate=0;

		for (size_t row=0; row<col; row++)
		{	
			rate = 1.0/gsl_matrix_get(cpp_pseudoscalar_tau_mat,row,col);
			gsl_matrix_set(cpp_pseudoscalar_decay_mat,row,col,rate);
			colrate+=rate*nu_mass[col];
		}

		gsl_matrix_set(cpp_pseudoscalar_decay_mat,col,col,colrate);
	}	


  gsl_matrix* cvp_pseudoscalar_decay_mat = gsl_matrix_alloc(numneu,numneu);
  gsl_matrix_set_zero(cvp_pseudoscalar_decay_mat);

	for (size_t col=0; col<numneu; col++)
	{
		colrate=0;

		for (size_t row=0; row<col; row++)
		{	
			rate = 1.0/gsl_matrix_get(cvp_pseudoscalar_tau_mat,row,col);
			gsl_matrix_set(cvp_pseudoscalar_decay_mat,row,col,rate);
			colrate+=rate*nu_mass[col];
		}

		gsl_matrix_set(cvp_pseudoscalar_decay_mat,col,col,colrate);
	}	

  nusqdec.Set_Pseudoscalar_Matrices(cpp_pseudoscalar_decay_mat,cvp_pseudoscalar_decay_mat);
  nusqdec.Compute_DT();

	//nusqdec.Set_IncludeOscillations(false); 

  nusqdec.EvolveState();

  for(size_t ie=0; ie<e_nodes.size(); ie++){
    std::cout << e_nodes[ie]/units.GeV << " ";
    for(size_t flv=0; flv< numneu; flv++){
      std::cout << nusqdec.EvalFlavorAtNode(flv,ie,0) << " ";
    }
    for(size_t flv=0; flv< numneu; flv++){
	  if (flv==numneu-1)
	  {
        std::cout << nusqdec.EvalFlavorAtNode(flv,ie,1) << std::endl;
	  }
	  else
	  {
        std::cout << nusqdec.EvalFlavorAtNode(flv,ie,1) << " ";
	  }
    }
  }

  gsl_matrix_free(cpp_scalar_tau_mat);  
  gsl_matrix_free(cvp_scalar_tau_mat);  
  gsl_matrix_free(cpp_scalar_decay_mat);  
  gsl_matrix_free(cvp_scalar_decay_mat);  
  gsl_matrix_free(cpp_pseudoscalar_tau_mat);  
  gsl_matrix_free(cvp_pseudoscalar_tau_mat);  
  gsl_matrix_free(cpp_pseudoscalar_decay_mat);  
  gsl_matrix_free(cvp_pseudoscalar_decay_mat);  

  return 0;
}
