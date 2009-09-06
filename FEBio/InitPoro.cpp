#include "stdafx.h"
#include "fem.h"
#include "FEPoroElastic.h"

//-----------------------------------------------------------------------------
//! Initialize poro-elastic data

bool FEM::InitPoro()
{
	int i, j;

	// see if there are any poro-elastic materials present
	for (i=0; i<Materials(); ++i)
		if (dynamic_cast<FEPoroElastic*>(&m_MAT[i]))
		{
			m_pStep->m_itype = FE_STATIC_PORO;
			break;
		}

	if (m_pStep->m_itype != FE_STATIC_PORO)
	{
		// if there is no poroelasticity
		// we set all pressure degrees of freedoms as fixed
		// just to make sure they do not get assigned an equation number
		for (i=0; i<m_mesh.Nodes(); ++i) m_mesh.Node(i).m_ID[6] = -1;

		// also remove prescribed pressures
		for (i=0; i<m_DC.size(); ++i)
		{
			int& node = m_DC[i].node;
			int& bc   = m_DC[i].bc;

			if (bc == 6) bc = -1;
		}

		// let's go back
		return true;
	}

	// see if we are the symmetric version or not
	if (m_bsym_poro == false) 
	{
		SetSymmetryFlag(false);
	
		// make sure we are using full-Newton
		if (m_pStep->m_psolver->m_maxups != 0)
		{
			m_pStep->m_psolver->m_maxups = 0;
			m_log.printbox("WARNING", "The non-symmetric biphasic algorithm does not work with BFGS yet.\nThe full-Newton method will be used instead.");
		}
	}

	// fix all pressure dofs that are not used
	// that is, that are not part of a poro-elastic element
	// this is done in three steps
	// step 1. mark all poro-elastic nodes
	for (i=0; i<m_mesh.SolidElements(); ++i)
	{
		FESolidElement& el = m_mesh.SolidElement(i);
		FEPoroElastic* pm = dynamic_cast<FEPoroElastic*>(GetMaterial(el.GetMatID()));
		if (pm)
		{
			int N = el.Nodes();
			int* n = el.m_node;
			for (j=0; j<N; ++j) 
				if (m_mesh.Node(n[j]).m_ID[6] == 0) m_mesh.Node(n[j]).m_ID[6] = 1;
		}
	}
	for (i=0; i<m_mesh.ShellElements(); ++i)
	{
		FEShellElement& el = m_mesh.ShellElement(i);
		FEPoroElastic* pm = dynamic_cast<FEPoroElastic*>(GetMaterial(el.GetMatID()));
		if (pm)
		{
			int N = el.Nodes();
			int* n = el.m_node;
			for (j=0; j<N; ++j) 
				if (m_mesh.Node(n[j]).m_ID[6] == 0) m_mesh.Node(n[j]).m_ID[6] = 1;
		}
	}

	// step 2. fix pressure dofs of all unmarked nodes
	for (i=0; i<m_mesh.SolidElements(); ++i)
	{
		FESolidElement& el = m_mesh.SolidElement(i);
		FEPoroElastic* pm = dynamic_cast<FEPoroElastic*>(GetMaterial(el.GetMatID()));
		if (pm == 0)
		{
			int N = el.Nodes();
			int* n = el.m_node;
			for (j=0; j<N; ++j)
				if (m_mesh.Node(n[j]).m_ID[6] != 1) m_mesh.Node(n[j]).m_ID[6] = -1;
		}
	}
	for (i=0; i<m_mesh.ShellElements(); ++i)
	{
		FEShellElement& el = m_mesh.ShellElement(i);
		FEPoroElastic* pm = dynamic_cast<FEPoroElastic*>(GetMaterial(el.GetMatID()));
		if (pm == 0)
		{
			int N = el.Nodes();
			int* n = el.m_node;
			for (j=0; j<N; ++j)
				if (m_mesh.Node(n[j]).m_ID[6] != 1) m_mesh.Node(n[j]).m_ID[6] = -1;
		}
	}

	// step 3. free all marked dofs
	for (i=0; i<m_mesh.SolidElements(); ++i)
	{
		FESolidElement& el = m_mesh.SolidElement(i);
		FEPoroElastic* pm = dynamic_cast<FEPoroElastic*>(GetMaterial(el.GetMatID()));
		if (pm)
		{
			int N = el.Nodes();
			int* n = el.m_node;
			for (j=0; j<N; ++j)
				if (m_mesh.Node(n[j]).m_ID[6] == 1) m_mesh.Node(n[j]).m_ID[6] = 0;
		}
	}
	for (i=0; i<m_mesh.ShellElements(); ++i)
	{
		FEShellElement& el = m_mesh.ShellElement(i);
		FEPoroElastic* pm = dynamic_cast<FEPoroElastic*>(GetMaterial(el.GetMatID()));
		if (pm)
		{
			int N = el.Nodes();
			int* n = el.m_node;
			for (j=0; j<N; ++j)
				if (m_mesh.Node(n[j]).m_ID[6] == 1) m_mesh.Node(n[j]).m_ID[6] = 0;
		}
	}

	return true;
}
