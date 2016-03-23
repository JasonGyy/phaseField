//Matrix Free implementation of coupled Cahn-Hilliard, Allen-Cahn and Mechanics formulation 
#ifndef CHACMECHANICS_H
#define CHACMECHANICS_H
//this source file is temporarily treated as a header file (hence
//#ifndef's) till library packaging scheme is finalized

#include "../../../include/matrixFreePDE.h"

//material models
#include "../mechanics/computeStress.h"

template <int dim>
class CoupledCHACMechanicsProblem: public MatrixFreePDE<dim>
{
 public: 
  CoupledCHACMechanicsProblem();

  void shiftConcentration();

 private:
  //elasticity matrix
  Table<2, double> CIJ;
  Table<2, double> CIJ_alpha;
  Table<2, double> CIJ_beta;

  //RHS implementation for explicit solve
  void getRHS(const MatrixFree<dim,double> &data, 
	      std::vector<vectorType*> &dst, 
	      const std::vector<vectorType*> &src,
	      const std::pair<unsigned int,unsigned int> &cell_range) const;
    
  //LHS implementation for implicit solve 
  void  getLHS(const MatrixFree<dim,double> &data, 
	       vectorType &dst, 
	       const vectorType &src,
	       const std::pair<unsigned int,unsigned int> &cell_range) const;

  //method to apply initial conditions
  void applyInitialConditions();
 
  //methods to apply dirichlet BC's on displacement
  void applyDirichletBCs();

  // method to modify the fields for nucleation
  void modifySolutionFields();

  // calculate (and output) the total free energy of the system
  void computeFreeEnergyValue(std::vector<double>& freeEnergyValues);

  void computeIntegral(double& integratedField);

};

//constructor
template <int dim>
CoupledCHACMechanicsProblem<dim>::CoupledCHACMechanicsProblem(): MatrixFreePDE<dim>(),
  CIJ(2*dim-1+dim/3,2*dim-1+dim/3), CIJ_alpha(2*dim-1+dim/3,2*dim-1+dim/3), CIJ_beta(2*dim-1+dim/3,2*dim-1+dim/3)
{
  //initialize elasticity matrix
#if defined(MaterialModelV) && defined(MaterialConstantsV)
	if (n_dependent_stiffness == true){
		double materialConstants[]=MaterialConstantsV;
		getCIJMatrix<dim>(MaterialModelV, materialConstants, CIJ_alpha, this->pcout);

		double materialConstantsBeta[]=MaterialConstantsBetaV;
		getCIJMatrix<dim>(MaterialModelBetaV, materialConstantsBeta, CIJ_beta, this->pcout);
	}
	else{
		double materialConstants[]=MaterialConstantsV;
		getCIJMatrix<dim>(MaterialModelV, materialConstants, CIJ, this->pcout);
	}

#else
#error Compile ERROR: missing material property variable: MaterialModelV, MaterialConstantsV
#endif
}

template <int dim>
void  CoupledCHACMechanicsProblem<dim>::getRHS(const MatrixFree<dim,double> &data, 
					       std::vector<vectorType*> &dst, 
					       const std::vector<vectorType*> &src,
					       const std::pair<unsigned int,unsigned int> &cell_range) const{
  

  //double test = 0.0;
  //computeIntegral(integratedField);

  //initialize fields
  typeScalar cVals(data, 0), n1Vals(data,1), n2Vals(data,2), n3Vals(data,3);
  typeVector uVals(data, 4);

  //loop over cells
  for (unsigned int cell=cell_range.first; cell<cell_range.second; ++cell){
    //initialize c field
    cVals.reinit(cell); cVals.read_dof_values_plain(*src[0]); cVals.evaluate(true, true, false);

    //initialize n fields
    n1Vals.reinit(cell); n1Vals.read_dof_values_plain(*src[1]); n1Vals.evaluate(true, true, false);
    n2Vals.reinit(cell); n2Vals.read_dof_values_plain(*src[2]); n2Vals.evaluate(true, true, false);
    n3Vals.reinit(cell); n3Vals.read_dof_values_plain(*src[3]); n3Vals.evaluate(true, true, false);

    //initialize u field 
    uVals.reinit(cell); uVals.read_dof_values_plain(*src[4]);

    if (c_dependent_misfit == true){
    	uVals.evaluate(false, true, true);
    }
    else{
    	uVals.evaluate(false, true, false);
    }

    //loop over quadrature points
    for (unsigned int q=0; q<cVals.n_q_points; ++q){
      //c
      scalarvalueType c = cVals.get_value(q);
      scalargradType cx = cVals.get_gradient(q);

      //n1
      scalarvalueType n1 = n1Vals.get_value(q);
      scalargradType n1x = n1Vals.get_gradient(q);

      //n2
      scalarvalueType n2 = n2Vals.get_value(q);
      scalargradType n2x = n2Vals.get_gradient(q);

      //n3 
      scalarvalueType n3 = n3Vals.get_value(q);
      scalargradType n3x = n3Vals.get_gradient(q);
      
      //u
      vectorgradType ux = uVals.get_gradient(q);
      vectorgradType Rux;

      vectorhessType uxx;

      if (c_dependent_misfit == true){
    	  uxx = uVals.get_hessian(q);
      }

      // Calculate the stress-free transformation strain and its derivatives at the quadrature point
      dealii::VectorizedArray<double> sfts1[dim][dim], sfts1c[dim][dim], sfts1cc[dim][dim], sfts2[dim][dim], sfts2c[dim][dim], sfts2cc[dim][dim], sfts3[dim][dim], sfts3c[dim][dim], sfts3cc[dim][dim];
      dealii::VectorizedArray<double> tanh_c_plus_cp_times_dp;

      dealii::VectorizedArray<double> H_strain1, H_strain1n;
      //      tanh_c_plus_cp_times_dp = (constV(1.0)-std::exp(constV(-2.0*10.0)*(n1+constV(-0.6))))/(constV(1.0)+std::exp(constV(-2.0*10.0)*(c+constV(-0.6))));
      //      H_strain1 = constV(0.5) + constV(0.5)*tanh_c_plus_cp_times_dp;
      //      H_strain1n = constV(-0.5*10.0)*(tanh_c_plus_cp_times_dp*tanh_c_plus_cp_times_dp - constV(1.0));
            //H_strain1 = constV(-6.63)*n1*n1*n1*n1*n1 + constV(10.7)*n1*n1*n1*n1 + constV(6.63*3.0-2.0*10.7-2.0)*n1*n1*n1 + constV(-6.63*2.0+10.7+3.0)*n1*n1;
            //H_strain1 = H_strain1*H_strain1*H_strain1*H_strain1;
            //H_strain1n = constV(4.0) * H_strain1*H_strain1*H_strain1 * (constV(-6.63*5.0)*n1*n1*n1*n1 + constV(4.0*10.7)*n1*n1*n1 + constV(3.0*(6.63*3.0-2.0*10.7-2.0))*n1*n1 + constV(2.0*(-6.63*2.0+10.7+3.0))*n1);
      H_strain1 = h1V*h1V*h1V*h1V*h1V;
      H_strain1n = 5.0*h1V*h1V*h1V*h1V*hn1V;


      for (unsigned int i=0; i<dim; i++){
    	  for (unsigned int j=0; j<dim; j++){
    		  if (c_dependent_misfit == true){
    			  // Polynomial fits for the stress-free transformation strains, of the form: sfts = a_p * c + b_p
    			  sfts1[i][j] = constV(a1[i][j])*c + constV(b1[i][j]);
    			  sfts1c[i][j] = constV(a1[i][j]);
    			  sfts1cc[i][j] = constV(0.0);

    			  // Polynomial fits for the stress-free transformation strains, of the form: sfts = a_p * c + b_p
    			  sfts2[i][j] = constV(a2[i][j])*c + constV(b2[i][j]);
    			  sfts2c[i][j] = constV(a2[i][j]);
    			  sfts2cc[i][j] = constV(0.0);

    			  // Polynomial fits for the stress-free transformation strains, of the form: sfts = a_p * c + b_p
    			  sfts3[i][j] = constV(a3[i][j])*c + constV(b3[i][j]);
    			  sfts3c[i][j] = constV(a3[i][j]);
    			  sfts3cc[i][j] = constV(0.0);
    		  }
    		  else{
    			  sfts1[i][j] = constV(sf1Strain[i][j]);
    			  sfts1c[i][j] = constV(0.0);
    			  sfts1cc[i][j] = constV(0.0);

    			  sfts2[i][j] = constV(sf2Strain[i][j]);
    			  sfts2c[i][j] = constV(0.0);
    			  sfts2cc[i][j] = constV(0.0);

    			  sfts3[i][j] = constV(sf3Strain[i][j]);
    			  sfts3c[i][j] = constV(0.0);
    			  sfts3cc[i][j] = constV(0.0);
    		  }
    	  }
      }


      //compute E2=(E-E0)
      dealii::VectorizedArray<double> E2[dim][dim], S[dim][dim];

      for (unsigned int i=0; i<dim; i++){
    	  for (unsigned int j=0; j<dim; j++){
    		  //E2[i][j]= constV(0.5)*(ux[i][j]+ux[j][i])-( sfts1[i][j]*h1V + sfts2[i][j]*h2V + sfts3[i][j]*h3V);
    		  E2[i][j]= constV(0.5)*(ux[i][j]+ux[j][i])-( sfts1[i][j]*H_strain1 + sfts2[i][j]*h2V + sfts3[i][j]*h3V);

    	  }
      }
      
      //compute stress
      //S=C*(E-E0)
      dealii::VectorizedArray<double> CIJ_combined[2*dim-1+dim/3][2*dim-1+dim/3];
      if (n_dependent_stiffness == true){
    	  for (unsigned int i=0; i<2*dim-1+dim/3; i++){
    		  for (unsigned int j=0; j<2*dim-1+dim/3; j++){
    			  CIJ_combined[i][j] = constV(CIJ_alpha(i,j))*(constV(1.0)-h1V-h2V-h3V) + constV(CIJ_beta(i,j))*(h1V+h2V+h3V);
    		  }
    	  }
    	  computeStress<dim>(CIJ_combined, E2, S);
      }
      else{
    	  computeStress<dim>(CIJ, E2, S);
      }
      
      //fill residual corresponding to mechanics
      //R=-C*(E-E0)
      for (unsigned int i=0; i<dim; i++){
    	  for (unsigned int j=0; j<dim; j++){
				Rux[i][j] = - S[i][j];
    	  }
      }
      // Compute one of the stress terms in the order parameter chemical potential, nDependentMisfitACp = C*(E-E0)*(E0_p*Hn)
      dealii::VectorizedArray<double> nDependentMisfitAC1=make_vectorized_array(0.0);
      dealii::VectorizedArray<double> nDependentMisfitAC2=make_vectorized_array(0.0);
      dealii::VectorizedArray<double> nDependentMisfitAC3=make_vectorized_array(0.0);

      for (unsigned int i=0; i<dim; i++){
    	  for (unsigned int j=0; j<dim; j++){
    		  nDependentMisfitAC1+=S[i][j]*(sfts1[i][j]);
    		  nDependentMisfitAC2+=S[i][j]*(sfts2[i][j]);
    		  nDependentMisfitAC3+=S[i][j]*(sfts3[i][j]);
    	  }
      }

      //nDependentMisfitAC1*=-hn1V;
      nDependentMisfitAC1*=-H_strain1n;
      nDependentMisfitAC2*=-hn2V;
      nDependentMisfitAC3*=-hn3V;

      // Compute the other stress term in the order parameter chemical potential, heterMechACp = 0.5*Hn*(C_alpha-C_beta)*(E-E0)*(E-E0)
      dealii::VectorizedArray<double> heterMechAC1=constV(0.0);
      dealii::VectorizedArray<double> heterMechAC2=constV(0.0);
      dealii::VectorizedArray<double> heterMechAC3=constV(0.0);
      dealii::VectorizedArray<double> S2[dim][dim];

      if (n_dependent_stiffness == true){
    	  Table<2, double> CIJ_diff(2*dim-1+dim/3,2*dim-1+dim/3);
    	  for (unsigned int i=0; i<2*dim-1+dim/3; i++){
    		  for (unsigned int j=0; j<2*dim-1+dim/3; j++){
    			  CIJ_diff(i,j) = CIJ_beta(i,j) - CIJ_alpha(i,j);
    		  }
    	  }
    	  computeStress<dim>(CIJ_diff, E2, S2);
    	  for (unsigned int i=0; i<dim; i++){
    		  for (unsigned int j=0; j<dim; j++){
    			  heterMechAC1 += S[i][j]*E2[i][j];
    		  }
    	  }
    	  // Aside from HnpV, heterMechAC1, heterMechAC2, and heterMechAC3 are equal
    	  heterMechAC2 = 0.5*hn2V*heterMechAC1;
    	  heterMechAC3 = 0.5*hn3V*heterMechAC1;

    	  heterMechAC1 = 0.5*hn1V*heterMechAC1;
      }


		// compute the stress term in the gradient of the concentration chemical potential, grad_mu_el = [C*(E-E0)*E0c]x, must be a vector with length dim
		dealii::VectorizedArray<double> grad_mu_el[dim];

		for (unsigned int k=0; k<dim; k++){
		  grad_mu_el[k] = constV(0.0);
		}

		if (c_dependent_misfit == true){
			dealii::VectorizedArray<double> E3[dim][dim], S3[dim][dim];

			for (unsigned int i=0; i<dim; i++){
				for (unsigned int j=0; j<dim; j++){
					//E3[i][j] =  -( sfts1c[i][j]*h1V + sfts2c[i][j]*h2V + sfts3c[i][j]*h3V);
					E3[i][j] =  -( sfts1c[i][j]*H_strain1 + sfts2c[i][j]*h2V + sfts3c[i][j]*h3V);
				}
			}

			if (n_dependent_stiffness == true){
				computeStress<dim>(CIJ_combined, E3, S3);
			}
			else{
				computeStress<dim>(CIJ, E3, S3);
			}

			for (unsigned int i=0; i<dim; i++){
				for (unsigned int j=0; j<dim; j++){
					for (unsigned int k=0; k<dim; k++){
//						grad_mu_el[k]+=S3[i][j] * (constV(0.5)*(uxx[i][j][k]+uxx[j][i][k]) - (sfts1c[i][j]*h1V + sfts2c[i][j]*h2V + sfts3c[i][j]*h3V)*cx[k]
//										  - (sfts1[i][j]*hn1V*n1x[k] + sfts2[i][j]*hn2V*n2x[k] + sfts3[i][j]*hn3V*n3x[k]));
//
//						grad_mu_el[k]+= - S[i][j] * (sfts1c[i][j]*hn1V*n1x[k] + sfts2c[i][j]*hn2V*n2x[k] + sfts3c[i][j]*hn3V*n3x[k]
//										  + (sfts1cc[i][j]*h1V + sfts2cc[i][j]*h2V + sfts3cc[i][j]*h3V)*cx[k]);

						grad_mu_el[k]+=S3[i][j] * (constV(0.5)*(uxx[i][j][k]+uxx[j][i][k]) - (sfts1c[i][j]*H_strain1 + sfts2c[i][j]*h2V + sfts3c[i][j]*h3V)*cx[k]
																  - (sfts1[i][j]*H_strain1n*n1x[k] + sfts2[i][j]*hn2V*n2x[k] + sfts3[i][j]*hn3V*n3x[k]));

						grad_mu_el[k]+= - S[i][j] * (sfts1c[i][j]*H_strain1n*n1x[k] + sfts2c[i][j]*hn2V*n2x[k] + sfts3c[i][j]*hn3V*n3x[k]
																  + (sfts1cc[i][j]*H_strain1 + sfts2cc[i][j]*h2V + sfts3cc[i][j]*h3V)*cx[k]);


						if (n_dependent_stiffness == true){
							grad_mu_el[k]+= - S2[i][j] * (sfts1c[i][j]*hn1V*n1x[k] + sfts2c[i][j]*hn2V*n2x[k] + sfts3c[i][j]*hn3V*n3x[k]);

						}
					}
				}
			}
		}



      //compute K*nx
      scalargradType Knx1, Knx2, Knx3;
      for (unsigned int a=0; a<dim; a++) {
    	  Knx1[a]=0.0;
    	  Knx2[a]=0.0;
    	  Knx3[a]=0.0;
    	  for (unsigned int b=0; b<dim; b++){
    		  Knx1[a]+=constV(Kn1[a][b])*n1x[b];
    		  Knx2[a]+=constV(Kn2[a][b])*n2x[b];
    		  Knx3[a]+=constV(Kn3[a][b])*n3x[b];
    	  }
      }
  
      //submit values
      cVals.submit_value(rcV,q); cVals.submit_gradient(rcxV,q);
      n1Vals.submit_value(rn1V,q); n1Vals.submit_gradient(rn1xV,q);
      n2Vals.submit_value(rn2V,q); n2Vals.submit_gradient(rn2xV,q);
      n3Vals.submit_value(rn3V,q); n3Vals.submit_gradient(rn3xV,q);
      uVals.submit_gradient(Rux,q);

    }
    
    //integrate values
    cVals.integrate(true, true);  cVals.distribute_local_to_global(*dst[0]);
    n1Vals.integrate(true, true); n1Vals.distribute_local_to_global(*dst[1]);
    n2Vals.integrate(true, true); n2Vals.distribute_local_to_global(*dst[2]);
    n3Vals.integrate(true, true); n3Vals.distribute_local_to_global(*dst[3]);
    uVals.integrate(false, true); uVals.distribute_local_to_global(*dst[4]);
  }
}

template <int dim>
void  CoupledCHACMechanicsProblem<dim>::getLHS(const MatrixFree<dim,double> &data, 
					       vectorType &dst, 
					       const vectorType &src,
					       const std::pair<unsigned int,unsigned int> &cell_range) const{
  typeVector uVals(data, 4);
  typeScalar n1Vals(data,1), n2Vals(data,2), n3Vals(data,3);
  
  //loop over cells
  for (unsigned int cell=cell_range.first; cell<cell_range.second; ++cell){
    //initialize u field 
    uVals.reinit(cell); uVals.read_dof_values_plain(src); uVals.evaluate(false, true, false);

    //initialize n fields
    n1Vals.reinit(cell); n1Vals.read_dof_values_plain(*MatrixFreePDE<dim>::solutionSet[1]); n1Vals.evaluate(true, false, false);
    n2Vals.reinit(cell); n2Vals.read_dof_values_plain(*MatrixFreePDE<dim>::solutionSet[2]); n2Vals.evaluate(true, false, false);
    n3Vals.reinit(cell); n3Vals.read_dof_values_plain(*MatrixFreePDE<dim>::solutionSet[3]); n3Vals.evaluate(true, false, false);

    //loop over quadrature points
    for (unsigned int q=0; q<uVals.n_q_points; ++q){
      //u
      vectorgradType ux = uVals.get_gradient(q);
      vectorgradType Rux;

      // n
      scalarvalueType n1 = n1Vals.get_value(q);
      scalarvalueType n2 = n2Vals.get_value(q);
      scalarvalueType n3 = n3Vals.get_value(q);

      //compute strain tensor
      dealii::VectorizedArray<double> E[dim][dim], S[dim][dim];
      for (unsigned int i=0; i<dim; i++){
    	  for (unsigned int j=0; j<dim; j++){
    		  E[i][j]= constV(0.5)*(ux[i][j]+ux[j][i]);
    	  }
      }
    
      // Compute stress tensor
      dealii::VectorizedArray<double> CIJ_combined[2*dim-1+dim/3][2*dim-1+dim/3];
      if (n_dependent_stiffness == true){
    	  for (unsigned int i=0; i<2*dim-1+dim/3; i++){
    		  for (unsigned int j=0; j<2*dim-1+dim/3; j++){
    			  CIJ_combined[i][j] = constV(CIJ_alpha(i,j))*(constV(1.0)-h1V-h2V-h3V) + constV(CIJ_beta(i,j))*(h1V+h2V+h3V);
			  }
		  }
		  computeStress<dim>(CIJ_combined, E, S);
		}
		else{
		  computeStress<dim>(CIJ, E, S);
		}

      //compute residual
      for (unsigned int i=0; i<dim; i++){
	for (unsigned int j=0; j<dim; j++){
	  Rux[i][j] = S[i][j]; 
	}
      }
      
      //submit residual value
      uVals.submit_gradient(Rux,q);
    }
    
    //integrate
    uVals.integrate(false, true); uVals.distribute_local_to_global(dst);
  }
}

//compute integrated free energy value over the domain
template <int dim>
void CoupledCHACMechanicsProblem<dim>::computeFreeEnergyValue(std::vector<double>& freeEnergyValues){
  double value=0.0;
  QGauss<dim>  quadrature_formula(finiteElementDegree+1);
  FE_Q<dim> FE (QGaussLobatto<1>(finiteElementDegree+1));
  FEValues<dim> fe_values (FE, quadrature_formula, update_values | update_gradients | update_JxW_values | update_quadrature_points);
  const unsigned int   dofs_per_cell = FE.dofs_per_cell;
  const unsigned int   n_q_points    = quadrature_formula.size();
  std::vector<double> cVal(n_q_points), n1Val(n_q_points), n2Val(n_q_points), n3Val(n_q_points);
  std::vector<Tensor<1,dim,double> > cxVal(n_q_points), n1xVal(n_q_points), n2xVal(n_q_points), n3xVal(n_q_points);
  std::vector<std::vector<Tensor<1,dim,double> > > uxVal(n_q_points,std::vector<Tensor<1,dim,double> >(dim));
  //std::vector<Tensor<2,dim,double> > uxVal(n_q_points);
  //std::vector<std::vector<Tensor<1,dim,double> > > uxVal(n_q_points,dim);
  //std::vector<std::vector<Tensor<1,dim,double> > > uxVal[n_q_points][dim];
  //std::vector<vectorgradType> uxVal(n_q_points);

  // remove later
  double value_homo = 0, value_grad = 0, value_el = 0;

  typename DoFHandler<dim>::active_cell_iterator cell= this->dofHandlersSet[0]->begin_active(), endc = this->dofHandlersSet[0]->end();

  for (; cell!=endc; ++cell) {
	  if (cell->is_locally_owned()){
    	fe_values.reinit (cell);

    	unsigned int fieldIndex;
    	fieldIndex=this->getFieldIndex("c");
    	fe_values.get_function_values(*this->solutionSet[fieldIndex], cVal);
    	fe_values.get_function_gradients(*this->solutionSet[fieldIndex], cxVal);

    	fieldIndex=this->getFieldIndex("n1");
    	fe_values.get_function_values(*this->solutionSet[fieldIndex], n1Val);
    	fe_values.get_function_gradients(*this->solutionSet[fieldIndex], n1xVal);

    	fieldIndex=this->getFieldIndex("n2");
    	fe_values.get_function_values(*this->solutionSet[fieldIndex], n2Val);
    	fe_values.get_function_gradients(*this->solutionSet[fieldIndex], n2xVal);

    	fieldIndex=this->getFieldIndex("n3");
    	fe_values.get_function_values(*this->solutionSet[fieldIndex], n3Val);
    	fe_values.get_function_gradients(*this->solutionSet[fieldIndex], n3xVal);

    	fieldIndex=this->getFieldIndex("u");
    	//fe_values.get_function_gradients(*this->solutionSet[fieldIndex], uxVal); // This step doesn't work, uxVal not the right type


    	for (unsigned int q=0; q<n_q_points; ++q){
    		double c=cVal[q];
    		double n1 = n1Val[q];
    		double n2 = n2Val[q];
    		double n3 = n3Val[q];

//    		Tensor<2,dim> ux;
//    		for (unsigned int i=0; i<dofs_per_cell; i++){
//    				ux[i] = fe_values[uxVal].gradient(i,q);
//    		}


//    		vectorgradType ux;
//    		for (unsigned int i=0; i<dim; i++){
//    			for (unsigned int j=0; j<dim; j++){
//    				ux[i][j] = uxVal[q][i][j];
//    			}
//    		}

//    		double ux[dim][dim];
//    		for (unsigned int i=0; i<dim; i++){
//    			for (unsigned int j=0; j<dim; j++){
//    				ux[i][j] = uxVal[q][i][j];
//    			}
//    		}

    		// calculate the interfacial energy
    		double fgrad = 0;
    		for (int i=0; i<dim; i++){
    			for (int j=0; j<dim; j++){
    				fgrad += Kn1[i][j]*n1xVal[q][i]*n1xVal[q][j];
    			}
    		}
    		for (int i=0; i<dim; i++){
    			for (int j=0; j<dim; j++){
    				fgrad += Kn2[i][j]*n2xVal[q][i]*n2xVal[q][j];
    			}
    		}
    		for (int i=0; i<dim; i++){
    			for (int j=0; j<dim; j++){
    				fgrad += Kn3[i][j]*n3xVal[q][i]*n3xVal[q][j];
    			}
    		}
    		fgrad = 0.5*fgrad + W*fbarrierV; // need to generalize for multiple order parameters

    		// calculate the homogenous chemical energy
    		double fhomo = (1.0-(h1V+h2V+h3V))*faV + (h1V+h2V+h3V)*fbV;

    		// Calculatate the elastic energy
    		double fel = 0;

//    		VectorizedArray<double> test;
//    		test = constV(1.5);
//    		double test_2 = test[0];

/*
    		//vectorgradType E2;
    		dealii::Table<2, double> E2;
    		for (unsigned int i=0; i<dim; i++){
    			for (unsigned int j=0; j<dim; j++){
    				E2[i][j]= ux[i][j]+ux[j][i]; //constV(0.5)*(ux[i][j]+ux[j][i])-(sf1Strain[i][j]*h1V+sf2Strain[i][j]*h2V+sf3Strain[i][j]*h3V);
    			}
    		}

			#if problemDIM==3
    		dealii::Table<1, dealii::VectorizedArray<double> > E(6);

    		E(0)=E2[0][0]; E(1)=E2[1][1]; E(2)=E2[2][2];
    		E(3)=E2[1][2]+E2[2][1];
    		E(4)=E2[0][2]+E2[2][0];
    		E(5)=E2[0][1]+E2[1][0];

    		for (unsigned int i=0; i<6; i++){
    			for (unsigned int j=0; j<6; j++){
    				fel+=CIJ(i,j)*E(i)[0]*E(j)[0];
    			}
    		}
			#elif problemDIM==2
			  //dealii::Table<1, dealii::VectorizedArray<double> > E(3);
    		dealii::Table<1,double> E(3);
			  E(0)=E2[0][0]; E(1)=E2[1][1];
			  E(2)=E2[0][1]+E2[1][0];

			  for (unsigned int i=0; i<3; i++){
				for (unsigned int j=0; j<3; j++){
					fel+=CIJ(i,j)*E[i]*E[j];
				}
			  }
			#elif problemDIM==1
			  dealii::Table<1, dealii::VectorizedArray<double> > E(1);
			  E(0)=E2[0][0];
			  fel=CIJ(0,0)*E(0).operator[](0);
			#endif

*/


    		// Sum the energies at each integration point
    		value+=(fhomo+fgrad+fel)*fe_values.JxW(q);

    		// Remove this later, sum the components of the energies at each integration point
    		value_homo += fhomo*fe_values.JxW(q);
    		value_grad += fgrad*fe_values.JxW(q);
    		value_el += fel*fe_values.JxW(q);
    	}
	  }
  }

  value=Utilities::MPI::sum(value, MPI_COMM_WORLD);
  //freeEnergyValues.push_back(value);

  // remove later
  value_homo=Utilities::MPI::sum(value_homo, MPI_COMM_WORLD);
  value_grad=Utilities::MPI::sum(value_grad, MPI_COMM_WORLD);
  value_el=Utilities::MPI::sum(value_el, MPI_COMM_WORLD);

  freeEnergyValues.push_back(value_grad);

  if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0){
	  std::cout<<"Homogenous Free Energy: "<<value_homo<<std::endl;
  	  std::cout<<"Interfacial Free Energy: "<<value_grad<<std::endl;
  	  std::cout<<"Elastic Free Energy: "<<value_el<<std::endl;
  }
}


//structure representing each nucleus
struct nucleus{
  unsigned int index;
  dealii::Point<problemDIM> center;
  double radius;
  double seededTime, seedingTime;
};
//vector of all nucleus seeded in the problem
std::vector<nucleus> nuclei, localNuclei;

//nucleation model implementation
template <int dim>
void CoupledCHACMechanicsProblem<dim>::modifySolutionFields()
{
  //current time
  double t=this->currentTime;
  unsigned int inc=this->currentIncrement;
  double dx=spanX/( (double)subdivisionsX )/std::pow(2.0,refineFactor);
  double rand_val;
  int count = 0;
  //nucleation parameters
  double nRadius = 2.5; //spanX/20.0;
  double minDistBetwenNuclei=4*nRadius;

  unsigned int maxNumberNuclei=5; // doesn't do anything currently

  //get the list of node points in the domain
  std::map<dealii::types::global_dof_index, dealii::Point<dim> > support_points;
  dealii::DoFTools::map_dofs_to_support_points (dealii::MappingQ1<dim>(), *this->dofHandlersSet[0], support_points);
  //fields
  vectorType* n1=this->solutionSet[this->getFieldIndex("n1")];
  vectorType* n2=this->solutionSet[this->getFieldIndex("n2")];
  vectorType* n3=this->solutionSet[this->getFieldIndex("n3")];
  vectorType* c=this->solutionSet[this->getFieldIndex("c")];
  const double k1 = 0.0001; // nucleation probability constant
  const double k2 = 1.0;	// nucleation probability constant
  const double c0 = 0.300;	// baseline concentration?
  double J = 0.0;
  //delete the previous entries in the nuclei vector (old nucleus are still retained in the localNuclei vector)
  nuclei.clear();

  //populate localNuclei vector
  if (inc <= timeIncrements){
    nucleus* temp;
    //add nuclei based on concentration field values
    //loop over all points in the domain
    for (typename std::map<dealii::types::global_dof_index, dealii::Point<dim> >::iterator it=support_points.begin(); it!=support_points.end(); ++it){
      unsigned int dof=it->first;
      //set only local owned values of the parallel vector (eventually turn this into a separate function for each order parameter)
      if (n1->locally_owned_elements().is_element(dof)){
    	  dealii::Point<dim> nodePoint=it->second;
    	  double n1Value=(*n1)(dof);
    	  double n2Value=(*n2)(dof);
    	  double n3Value=(*n3)(dof);
    	  double cValue=(*c)(dof);

    	  rand_val = (double)rand()/(RAND_MAX);

    	  if ((t > 1000000000*timeStep) || (n1Value+n2Value+n3Value > 1.0e-6) || (cValue <= 0.0)) {
    		  J = 0;
    	  }
		  else{
			  J = cValue/c_matrix * dx*dx/((double)spanX * (double)spanY) * 0.01; // Only true in 2D!
    	  }

    	  if (rand_val <= J){
    		  bool isClose=false;
    		  for (std::vector<nucleus>::iterator thisNuclei=localNuclei.begin(); thisNuclei!=localNuclei.end(); ++thisNuclei){
    			  if (thisNuclei->center.distance(nodePoint)<minDistBetwenNuclei){
    				  isClose=true;
    			  }
    		  }

    		  if (!isClose){
    			  temp = new nucleus;
    			  temp->index=localNuclei.size();
    			  temp->center=nodePoint;
    			  temp->radius=nRadius;
    			  temp->seededTime=t;
    			  temp->seedingTime = 10000.0*timeStep;
    			  localNuclei.push_back(*temp);
    		  }
    	  }
      }
    }


    //filter nuclei by comparing with other processors
    int numProcs=Utilities::MPI::n_mpi_processes(MPI_COMM_WORLD);
    int thisProc=Utilities::MPI::this_mpi_process(MPI_COMM_WORLD);
    std::vector<int> numNucleiInProcs(numProcs, 0);
    //send nuclei information to processor 0
    int numNuclei=localNuclei.size();
    //send information about number of nuclei to processor 0
    if (thisProc!=0){
      MPI_Send(&numNuclei, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    else{
      numNucleiInProcs[0]=numNuclei;
      for (int proc=1; proc<numProcs; proc++){
	MPI_Recv(&numNucleiInProcs[proc], 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    //filter nuclei in processor zero
    //receive nuclei info from all processors
    if (thisProc!=0){
    	if (numNuclei>0){
    		std::vector<double> tempData((dim+3)*numNuclei);
    		unsigned int i=0;
    		for (std::vector<nucleus>::iterator thisNuclei=localNuclei.begin(); thisNuclei!=localNuclei.end(); ++thisNuclei){
    			tempData[i*(dim+3)]=thisNuclei->radius;
    			tempData[i*(dim+3)+1]=thisNuclei->seededTime;
    			tempData[i*(dim+3)+2]=thisNuclei->seedingTime;
    			for (unsigned int j=0; j<dim; j++) tempData[i*(dim+3)+3+j]=thisNuclei->center[j];
    			i++;
    		}
    		MPI_Send(&tempData[0], numNuclei*(dim+3), MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    	}
    }
    else{
    	//temporary array to store all the nuclei
    	std::vector<std::vector<double>*> tempNuceli(numProcs);
    	for (int proc=0; proc<numProcs; proc++) {
    		std::vector<double>* temp=new std::vector<double>(numNucleiInProcs[proc]*(dim+3));
    		if (numNucleiInProcs[proc]>0){
    			if (proc==0){
    				unsigned int i=0;
    				for (std::vector<nucleus>::iterator thisNuclei=localNuclei.begin(); thisNuclei!=localNuclei.end(); ++thisNuclei){
    					(*temp)[i*(dim+3)]=thisNuclei->radius;
    					(*temp)[i*(dim+3)+1]=thisNuclei->seededTime;
    					(*temp)[i*(dim+3)+2]=thisNuclei->seedingTime;
    					for (unsigned int j=0; j<dim; j++) (*temp)[i*(dim+3)+3+j]=thisNuclei->center[j];
    					i++;
    				}
    			}
    			else{
    				MPI_Recv(&((*temp)[0]), numNucleiInProcs[proc]*(dim+3), MPI_DOUBLE, proc, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    			}
    			tempNuceli[proc]=temp;
    		}
    	}

    	//filter the nuclei and add to nuclei vector in processor zero
    	for (int proc1=0; proc1<numProcs; proc1++) {
    		for (int i1=0; i1<numNucleiInProcs[proc1]; i1++){
    			double rad1=(*tempNuceli[proc1])[i1*(dim+3)];
    			double time1=(*tempNuceli[proc1])[i1*(dim+3)+1];
    			double seedingTime1=(*tempNuceli[proc1])[i1*(dim+3)+2];
    			dealii::Point<dim> center1;
    			for (unsigned int j1=0; j1<dim; j1++) {
    				center1[j1]=(*tempNuceli[proc1])[i1*(dim+3)+3+j1];
    			}
    			bool addNuclei=true;
    			//check if this nuceli present in any other processor
    			for (int proc2=0; proc2<numProcs; proc2++) {
    				if (proc1!=proc2){
    					for (int i2=0; i2<numNucleiInProcs[proc2]; i2++){
    						double rad2=(*tempNuceli[proc2])[i2*(dim+3)];
    						double time2=(*tempNuceli[proc2])[i2*(dim+3)+1];
    						dealii::Point<dim> center2;
    						for (unsigned int j2=0; j2<dim; j2++) center2(j2)=(*tempNuceli[proc2])[i2*(dim+3)+3+j2];
    						if ((center1.distance(center2)<=minDistBetwenNuclei) && (time1>=time2)){
    							addNuclei=false;
    							break;
    						}
    					}
    					if (!addNuclei) {break;}
    				}
    			}
    			if (addNuclei){
    				temp = new nucleus;
    				temp->index=nuclei.size();
    				temp->radius=rad1;
    				temp->seededTime=time1;
    				temp->seedingTime=seedingTime1;
    				temp->center=center1;
    				nuclei.push_back(*temp);
    			}
    		}
    	}
    }
    MPI_Barrier(MPI_COMM_WORLD);

    //disperse nuclei to all other processors
    unsigned int numGlobalNuclei;
    if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD)==0) {numGlobalNuclei=nuclei.size();}
    MPI_Bcast(&numGlobalNuclei, 1, MPI_INT, 0, MPI_COMM_WORLD);
    this->pcout << "total number of nuclei currently seeded : "  << numGlobalNuclei << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
    //
    std::vector<double> temp2(numGlobalNuclei*(dim+3));
    if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD)==0){
      unsigned int i=0;
      for (std::vector<nucleus>::iterator thisNuclei=nuclei.begin(); thisNuclei!=nuclei.end(); ++thisNuclei){
	temp2[i*(dim+3)]=thisNuclei->radius;
	temp2[i*(dim+3)+1]=thisNuclei->seededTime;
	temp2[i*(dim+3)+2]=thisNuclei->seedingTime;
	for (unsigned int j=0; j<dim; j++) temp2[i*(dim+3)+3+j]=thisNuclei->center[j];
	i++;
      }
    }
    MPI_Bcast(&temp2[0], numGlobalNuclei*(dim+3), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    //receive all nuclei
    if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD)!=0){
    	for(unsigned int i=0; i<numGlobalNuclei; i++){
    		temp = new nucleus;
    		temp->index=nuclei.size();
    		temp->radius=temp2[i*(dim+3)];
    		temp->seededTime=temp2[i*(dim+3)+1];
    		temp->seedingTime=temp2[i*(dim+3)+2];
    		dealii::Point<dim> tempCenter;
    		for (unsigned int j=0; j<dim; j++) tempCenter[j]=temp2[i*(dim+3)+3+j];
    		temp->center=tempCenter;
    		nuclei.push_back(*temp);
      }
    }
  }

  //seed nuclei
  unsigned int fieldIndex=this->getFieldIndex("n1");
  for (std::vector<nucleus>::iterator thisNuclei=nuclei.begin(); thisNuclei!=nuclei.end(); ++thisNuclei){

	  dealii::Point<dim> center=thisNuclei->center;
	  double radius=thisNuclei->radius;
	  double seededTime=thisNuclei->seededTime;
	  double seedingTime=thisNuclei->seedingTime;
	  this->pcout << "times: " << t << " " << seededTime << " " << seedingTime << std::endl;
	  //loop over all points in the domain
	  for (typename std::map<dealii::types::global_dof_index, dealii::Point<dim> >::iterator it=support_points.begin(); it!=support_points.end(); ++it){
		  unsigned int dof=it->first;
		  //set only local owned values of the parallel vector
		  if (n1->locally_owned_elements().is_element(dof)){
			  dealii::Point<dim> nodePoint=it->second;
			  //check conditions and seed nuclei
			  double r=nodePoint.distance(center);
			  if (r<=(2*radius)){
				  if ((t>seededTime) && (t<(seededTime+seedingTime))){
					  //this->pcout << "times: " << t << " " << seededTime << " " << seedingTime << std::endl;
					  //(*n1)(dof)=0.5*(1.0-std::tanh((r-radius)/(dx)));
					  (*n1)(dof)=0.5*(1.0-std::tanh((r-radius)/(0.4)));
				  }
			  }
		  }
	  }
  }
}

//compute the integral of one of the fields
template <int dim>
void CoupledCHACMechanicsProblem<dim>::computeIntegral(double& integratedField){
  QGauss<dim>  quadrature_formula(finiteElementDegree+1);
  FE_Q<dim> FE (QGaussLobatto<1>(finiteElementDegree+1));
  FEValues<dim> fe_values (FE, quadrature_formula, update_values | update_JxW_values | update_quadrature_points);
  const unsigned int   dofs_per_cell = FE.dofs_per_cell;
  const unsigned int   n_q_points    = quadrature_formula.size();
  std::vector<double> cVal(n_q_points), n1Val(n_q_points), n2Val(n_q_points), n3Val(n_q_points);

  typename DoFHandler<dim>::active_cell_iterator cell= this->dofHandlersSet[0]->begin_active(), endc = this->dofHandlersSet[0]->end();

  double value = 0.0;

  unsigned int fieldIndex;
  fieldIndex=this->getFieldIndex("c");

  for (; cell!=endc; ++cell) {
	  if (cell->is_locally_owned()){
    	fe_values.reinit (cell);

    	fe_values.get_function_values(*this->solutionSet[fieldIndex], cVal);

    	for (unsigned int q=0; q<n_q_points; ++q){
    		value+=(cVal[q])*fe_values.JxW(q);
    	}
	  }
  }

  value=Utilities::MPI::sum(value, MPI_COMM_WORLD);

  if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0){
  std::cout<<"Integrated field: "<<value<<std::endl;
  }

  integratedField = value;
}

#endif
