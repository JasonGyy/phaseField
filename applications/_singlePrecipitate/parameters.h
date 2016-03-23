//Parameters list for beta prime precipitation evolution problem

//define problem dimensions
#define problemDIM 2
#define spanX 15.0 //14.0
#define spanY 15.0 //14.0
#define spanZ 15.0 //10.0 //14.0

//define mesh parameters
#define subdivisionsX 3 //90
#define subdivisionsY 3 //90
#define subdivisionsZ 3 //90
#define refineFactor 5 //0
#define finiteElementDegree 2

//define time step parameters
#define timeStep (1.65e-5*0.25) //5e-6 //1.67e-5
#define timeIncrements 50000*4 //200000
#define timeFinal 100000000 //(timeStep*timeIncrements)
#define skipImplicitSolves 1

//define solver parameters
#define solverType SolverCG
#define abs_tol true
#define relSolverTolerance 1.0e-2
#define absSolverTolerance 1.0e-5
#define maxSolverIterations 10000

//define results output parameters
#define writeOutput true
#define skipOutputSteps (timeIncrements/10) //50000 //timeIncrements/10 //5000

// flag to allow or disallow nucleation
#define nucleation_occurs false

#define numFields (4+problemDIM)

//define Cahn-Hilliard parameters (No Gradient energy)
#define McV 1.0

//define Allen-Cahn parameters
#define Mn1V 1000.0
#define Mn2V 50.0
#define Mn3V 50.0

// define gradient penalty tensors
//double Kn1[3][3]={{0.0280,0,0},{0,0.0350,0},{0,0,0.0107}};
double Kn1[3][3]={{0.0350,0,0},{0,0.0350,0},{0,0,0.0350}};
double Kn2[3][3]={{0.123,0,0},{0,0.123,0},{0,0,0.123}};
double Kn3[3][3]={{0.123,0,0},{0,0.123,0},{0,0,0.123}};

//define energy barrier coefficient (used to tune the interfacial energy)
#define W 0.0

// Define Mechanical properties

#define n_dependent_stiffness false
// Mechanical symmetry of the material and stiffness parameters
// Used throughout system if n_dependent_stiffness == false, used in n=0 phase if n_dependent_stiffness == true


//#define MaterialModelV ANISOTROPIC
// 3D order of constants ANISOTROPIC - 21 constants [11, 22, 33, 44, 55, 66, 12, 13, 14, 15, 16, 23, 24, 25, 26, 34, 35, 36, 45, 46, 56]
//#define MaterialConstantsV {62.6,62.6,64.9,13.3,13.3,18.3,26.0,20.9,0,0,0,20.9,0,0,0,0,0,0,0,0,0} //these are in GPa-need to be non-dimensionalized
//#define MaterialConstantsV {31.3,31.3,32.45,6.65,6.65,9.15,13.0,10.45,0,0,0,10.45,0,0,0,0,0,0,0,0,0} //scaled by E* = 2e9 J/m^3

// 2D order of constants ANISOTROPIC - 6 constants [C11 C22 C33 C12 C13 C23]
//#define MaterialConstantsV {31.3,31.3,6.65,13.0,0.0,0.0} //scaled by E* = 2e9 J/m^3

#define MaterialModelV ISOTROPIC
#define MaterialConstantsV {22.5,0.3}

// Used in n=1 phase if n_dependent_stiffness == true
#define MaterialModelBetaV ISOTROPIC
#define MaterialConstantsBetaV {22.5,0.3}

#define c_dependent_misfit true
// Stress-free transformation strains (concentration independent, used if c_dependent_misfit == false)
double sf1Strain[3][3] = {{-0.00,0,0},{0,-0.00,0},{0,0,0}}; // test
//double sf1Strain[3][3] = {{0.1305,0,0},{0,-0.0152,0},{0,0,-0.014}}; //Mg-Nd beta-prime
double sf2Strain[3][3] = {{0.0212,0.0631,0},{0.0631,0.0941,0},{0,0,0}};
double sf3Strain[3][3] = {{0.0212,-0.0631,0},{-0.0631,0.0941,0},{0,0,0}};

// Stress-free transformation strains (concentration dependent terms, used if c_dependent_misfit == true, currently assumes linear dependence)
// Linear fits for the stress-free transformation strains in for sfts_p = ap * c + bp
double a1[3][3] = {{0.1568,0,0},{0,0.6548,0},{0,0,0}};
double b1[3][3] = {{-0.02756,0,0},{0,-0.09584,0},{0,0,0}};

double a2[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
double b2[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

double a3[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
double b3[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

//define free energy expressions (Mg-Nd data from CASM) (B''', gen 1)
#define faV (46.599*c*c - 1.6907*c + 0.00010827)
#define facV (93.198*c - 1.6907)
#define faccV (93.198)
#define fbV (2.1479*c*c - 2.1743*c + 0.033684)
#define fbcV (4.2958*c - 2.1743)
#define fbccV (4.2958)

//define free energy expressions (Mg-Nd data from CASM) (B', gen 1)
//#define faV (44.547*c*c - 1.6954*c + 0.00011001)
//#define facV (89.094*c - 1.6954)
//#define faccV (89.094)
//#define fbV (12.818*c*c - 4.8583*c + 0.19988)
//#define fbcV (25.636*c - 4.8583)
//#define fbccV (25.636)

// Larry's free energies
//#define faV (24.7939*c*c - 1.6752*c - 1.9453e-06)
//#define facV (49.5878*c - 1.6752)
//#define faccV (49.5878)
//#define fbV (37.9316*c*c - 10.7373*c + 0.5401)
//#define fbcV (75.8633*c - 10.7373)
//#define fbccV (75.8633)

#define h1V (3.0*n1*n1-2.0*n1*n1*n1)
#define h2V (3.0*n2*n2-2.0*n2*n2*n2)
#define h3V (3.0*n3*n3-2.0*n3*n3*n3)
#define hn1V (6.0*n1-6.0*n1*n1)
#define hn2V (6.0*n2-6.0*n2*n2)
#define hn3V (6.0*n3-6.0*n3*n3)

// This double-well function can be used to tune the interfacial energy
#define fbarrierV (n1*n1-2.0*n1*n1*n1+n1*n1*n1*n1)
#define fbarriernV (2.0*n1-6.0*n1*n1+4.0*n1*n1*n1)

// Residuals
#define rcV   (c)
#define rcxTemp ( cx*((1.0-h1V-h2V-h3V)*faccV+(h1V+h2V+h3V)*fbccV) + n1x*((fbcV-facV)*hn1V) + n2x*((fbcV-facV)*hn2V) + n3x*((fbcV-facV)*hn3V) + grad_mu_el)
#define rcxV  (constV(-timeStep*McV)*rcxTemp)

#define rn1V   (n1-constV(timeStep*Mn1V)*((fbV-faV)*hn1V+W*fbarriernV+nDependentMisfitAC1+heterMechAC1))
#define rn2V   (n2-constV(timeStep*Mn2V)*((fbV-faV)*hn2V))
#define rn3V   (n3-constV(timeStep*Mn3V)*((fbV-faV)*hn3V))
#define rn1xV  (constV(-timeStep*Mn1V)*Knx1)
#define rn2xV  (constV(-timeStep*Mn2V)*Knx2)
#define rn3xV  (constV(-timeStep*Mn3V)*Knx3)

// Initial geometry
#define x_denom 16.0
#define y_denom 16.0
#define z_denom 16.0
#define initial_interface_coeff 0.1
#define initial_radius 1.0 //3.0
#define c_matrix 0.004 //0.0
#define c_precip 0.16
#define adjust_avg_c false
#define c_avg 0.004
