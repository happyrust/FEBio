#pragma once
#include "FECore/FEMaterial.h"

//-----------------------------------------------------------------------------
// Base class for heat-transfer problems
class FEHeatTransferMaterial : public FEMaterial
{
public:
	FEMaterialPoint* CreateMaterialPointData() { return new FEHeatMaterialPoint; }
};

//-----------------------------------------------------------------------------
// Isotropic Fourer heat-transfer material
class FEIsotropicFourier : public FEHeatTransferMaterial
{
public:
	FEIsotropicFourier() {}
	void Init();

public:
	double	m_k;	//!< heat conductivity
	double	m_c;	//!< heat capacitance
	double	m_rho;	//!< density

public:
	void Conductivity(double D[3][3]);

	// declare as registered
	DECLARE_REGISTERED(FEIsotropicFourier);

	// declare parameter list
	DECLARE_PARAMETER_LIST();
};
