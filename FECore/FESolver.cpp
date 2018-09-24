#include "stdafx.h"
#include "FESolver.h"
#include "FEModel.h"

BEGIN_PARAMETER_LIST(FESolver, FECoreBase)
	ADD_PARAMETER(m_bsymm, "symmetric_stiffness");
END_PARAMETER_LIST();

//-----------------------------------------------------------------------------
FESolver::FESolver(FEModel* pfem) : FECoreBase(FESOLVER_ID), m_fem(*pfem)
{ 
	m_bsymm = true; // assume symmetric stiffness matrix
	m_niter = 0;

	m_nref = 0;
	
	m_baugment = false;
	m_naug = 0;
}

//-----------------------------------------------------------------------------
FESolver::~FESolver()
{
}

//-----------------------------------------------------------------------------
//! Get the FE model
FEModel& FESolver::GetFEModel()
{ 
	return m_fem; 
}

//-----------------------------------------------------------------------------
void FESolver::Clean()
{
}

//-----------------------------------------------------------------------------
//! This function is called right before SolveStep and should be used to initialize
//! time dependent information and other settings.
bool FESolver::InitStep(double time)
{
	FEModel& fem = GetFEModel();

	// evaluate load curve values at current time
	fem.EvaluateLoadCurves(time);

	// evaluate the parameter lists
	fem.EvaluateAllParameterLists();

	// re-validate materials
	// This is necessary since the material parameters can have changed (e.g. via load curves) and thus 
	// a new validation needs to be done to see if the material parameters are still valid. 
	if (fem.ValidateMaterials() == false) return false;

	return true;
}

//-----------------------------------------------------------------------------
bool FESolver::Init()
{
	// parameter checking
	return Validate();
}

//-----------------------------------------------------------------------------
void FESolver::Serialize(DumpStream& ar)
{
	FECoreBase::Serialize(ar);
	if (ar.IsShallow())
	{
		if (ar.IsSaving())
		{
			ar << m_bsymm;
			ar << m_nrhs << m_niter << m_nref << m_ntotref << m_naug;
		}
		else
		{
			ar >> m_bsymm;
			ar >> m_nrhs >> m_niter >> m_nref >> m_ntotref >> m_naug;
		}
	}
}
