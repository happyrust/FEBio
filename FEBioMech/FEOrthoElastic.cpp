#include "stdafx.h"
#include "FEOrthoElastic.h"

//-----------------------------------------------------------------------------
// define the material parameters
BEGIN_PARAMETER_LIST(FEOrthoElastic, FEElasticMaterial)
	ADD_PARAMETER2(E1, FE_PARAM_DOUBLE, FE_RANGE_GREATER(0.0), "E1");
	ADD_PARAMETER2(E2, FE_PARAM_DOUBLE, FE_RANGE_GREATER(0.0), "E2");
	ADD_PARAMETER2(E3, FE_PARAM_DOUBLE, FE_RANGE_GREATER(0.0), "E3");
	ADD_PARAMETER2(G12, FE_PARAM_DOUBLE, FE_RANGE_GREATER_OR_EQUAL(0.0), "G12");
	ADD_PARAMETER2(G23, FE_PARAM_DOUBLE, FE_RANGE_GREATER_OR_EQUAL(0.0), "G23");
	ADD_PARAMETER2(G31, FE_PARAM_DOUBLE, FE_RANGE_GREATER_OR_EQUAL(0.0), "G31");
	ADD_PARAMETER2(v12, FE_PARAM_DOUBLE, FE_RANGE_GREATER_OR_EQUAL(0.0), "v12");
	ADD_PARAMETER(v23, FE_PARAM_DOUBLE, "v23");
	ADD_PARAMETER(v31, FE_PARAM_DOUBLE, "v31");
END_PARAMETER_LIST();

//-----------------------------------------------------------------------------
//! Material initialization.
void FEOrthoElastic::Init()
{
	FEElasticMaterial::Init();

	if (v12 > sqrt(E1/E2)) throw MaterialError("Invalid value for v12. Let v12 <= sqrt(E1/E2)");
	if (v23 > sqrt(E2/E3)) throw MaterialError("Invalid value for v23. Let v23 <= sqrt(E2/E3)");
	if (v31 > sqrt(E3/E1)) throw MaterialError("Invalid value for v31. Let v31 <= sqrt(E3/E1)");

	// Evaluate Lame coefficients
	mu[0] = G12 + G31 - G23;
	mu[1] = G12 - G31 + G23;
	mu[2] =-G12 + G31 + G23;
	lam[0][0] = 1.0/E1; lam[0][1] = -v12/E1; lam[0][2] = -v31/E3;
	lam[1][0] = -v12/E1; lam[1][1] = 1.0/E2; lam[1][2] = -v23/E2;
	lam[2][0] = -v31/E3; lam[2][1] = -v23/E2; lam[2][2] = 1.0/E3;
	
	// check that compliance matrix is positive definite
	mat3ds c(lam[0][0],lam[1][1],lam[2][2],lam[0][1],lam[1][2],lam[0][2]);
	double l[3];
	c.exact_eigen(l);

	if ((l[0]<0) || (l[1]<0) || (l[2]<0))
		throw MaterialError("Stiffness matrix is not positive definite.");

	// evaluate stiffness matrix and extract Lame constants
	c = c.inverse();
	lam[0][0] = c(0,0) - 2*mu[0];
	lam[1][1] = c(1,1) - 2*mu[1];
	lam[2][2] = c(2,2) - 2*mu[2];
	lam[1][2] = c(1,2); lam[2][1] = c(2,1);
	lam[2][0] = c(2,0); lam[0][2] = c(0,2);
	lam[0][1] = c(0,1); lam[1][0] = c(1,0);
}

//-----------------------------------------------------------------------------
//! Calculates the stress for a linear orthotropic material
mat3ds FEOrthoElastic::Stress(FEMaterialPoint& mp)
{
	FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
	
	int i,j;
	vec3d a0[3];		// texture direction in reference configuration
	vec3d a[3];			// texture direction in current configuration
	mat3ds A[3];		// texture tensor in current configuration
	double K[3];		// Ka
	double L[3];		// La
	mat3ds bmi;			// B - I
	// Evaluate the deformation gradient
	mat3d &F = pt.m_F;
	double J = pt.m_J;
	
	// calculate left and right Cauchy-Green tensor
	mat3ds b = pt.LeftCauchyGreen();
	mat3ds c = pt.RightCauchyGreen();
	mat3ds c2 = c*c;
	mat3dd I(1.);
	
	for (i=0; i<3; i++) {	// Perform sum over all three texture directions
		// Copy the texture direction in the reference configuration to a0
		a0[i].x = pt.m_Q[0][i]; a0[i].y = pt.m_Q[1][i]; a0[i].z = pt.m_Q[2][i];
		K[i] = a0[i]*(c*a0[i]);
		L[i] = a0[i]*(c2*a0[i]);
		a[i] = F*a0[i]/sqrt(K[i]);	// Evaluate the texture direction in the current configuration
		A[i] = dyad(a[i]);			// Evaluate the texture tensor in the current configuration
	}
	
	// Evaluate the stress
	mat3ds s;
	s.zero();		// Initialize for summation
	bmi = b - I;
	for (i=0; i<3; i++) {
		s += mu[i]*K[i]*(A[i]*bmi + bmi*A[i]);
		for (j=0; j<3; j++)
			s += lam[i][j]*((K[i]-1)*K[j]*A[j]+(K[j]-1)*K[i]*A[i])/2.;
	}
	s /= 2.0*J;
	
	return s;
}

//-----------------------------------------------------------------------------
tens4ds FEOrthoElastic::Tangent(FEMaterialPoint& mp)
{
	FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
	
	int i,j;
	vec3d a0[3];		// texture direction in reference configuration
	vec3d a[3];			// texture direction in current configuration
	mat3ds A[3];		// texture tensor in current configuration
	double K[3];		// Ka
	// Evaluate the strain and texture
	mat3d &F = pt.m_F;
	double J = pt.m_J;
	
	// calculate left and right Cauchy-Green tensor
	mat3ds b = pt.LeftCauchyGreen();
	mat3ds c = pt.RightCauchyGreen();
	mat3dd I(1.);
	
	for (i=0; i<3; i++) {	// Perform sum over all three texture directions
		// Copy the texture direction in the reference configuration to a0
		a0[i].x = pt.m_Q[0][i]; a0[i].y = pt.m_Q[1][i]; a0[i].z = pt.m_Q[2][i];
		K[i] = a0[i]*(c*a0[i]);
		a[i] = F*a0[i]/sqrt(K[i]);	// Evaluate the texture direction in the current configuration
		A[i] = dyad(a[i]);			// Evaluate the texture tensor in the current configuration
	}
	
	tens4ds C(0.0);
	for (i=0; i<3; i++) {
		C += mu[i]*K[i]*dyad4s(A[i],b);
		for (j=0; j<3; j++)
			C += lam[i][j]*K[i]*K[j]*dyad1s(A[i],A[j])/2.;
	}
	
	// Elasticity tensor
	C /= J;
	
	return C;
}

//-----------------------------------------------------------------------------
double FEOrthoElastic::StrainEnergyDensity(FEMaterialPoint& mp)
{
	FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
    
    mat3ds E = (pt.RightCauchyGreen() - mat3dd(1))/2;
    mat3ds E2 = E*E;
    
	vec3d a0[3];		// texture direction in reference configuration
	mat3ds A0[3];		// texture tensor in current configuration
    double AE[3], AE2[3];

	for (int i=0; i<3; i++) {
		// Copy the texture direction in the reference configuration to a0
		a0[i].x = pt.m_Q[0][i]; a0[i].y = pt.m_Q[1][i]; a0[i].z = pt.m_Q[2][i];
		A0[i] = dyad(a0[i]);			// Evaluate the texture tensor in the reference configuration
        AE[i] = A0[i].dotdot(E);
        AE2[i] = A0[i].dotdot(E2);
	}
    
    double sed = mu[0]*AE2[0] + mu[1]*AE2[1] + mu[2]*AE2[2]
    +0.5*(lam[0][0]*AE[0]*AE[0]+lam[1][1]*AE[1]*AE[1]+lam[2][2]*AE[2]*AE[2])
    +lam[0][1]*AE[0]*AE[1]+lam[1][2]*AE[1]*AE[2]+lam[2][0]*AE[2]*AE[0];
    
	return sed;
}
