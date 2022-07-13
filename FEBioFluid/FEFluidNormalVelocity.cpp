/*This file is part of the FEBio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio.txt for details.

Copyright (c) 2021 University of Utah, The Trustees of Columbia University in
the City of New York, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/



#include "stdafx.h"
#include "FEFluidNormalVelocity.h"
#include "FECore/FEElemElemList.h"
#include "FECore/FEGlobalMatrix.h"
#include "FECore/FEGlobalVector.h"
#include "FECore/log.h"
#include "FECore/LinearSolver.h"
#include "FEBioFluid.h"
#include <FECore/FEParabolicMap.h>
#include <FECore/FEFacetSet.h>

//=============================================================================
BEGIN_FECORE_CLASS(FEFluidNormalVelocity, FESurfaceLoad)
	ADD_PARAMETER(m_velocity, "velocity"                  );
    ADD_PARAMETER(m_bpv     , "prescribe_nodal_velocities");
    ADD_PARAMETER(m_bpar    , "parabolic");
    ADD_PARAMETER(m_brim    , "prescribe_rim_pressure"    );
END_FECORE_CLASS();

//-----------------------------------------------------------------------------
//! constructor
FEFluidNormalVelocity::FEFluidNormalVelocity(FEModel* pfem) : FESurfaceLoad(pfem), m_dofW(pfem)
{
    m_velocity = 0.0;
    m_bpv = true;
    m_bpar = false;
    m_brim = false;

    // TODO: Can this be done in Init, since  there is no error checking
    if (pfem)
    {
        m_dofW.AddVariable(FEBioFluid::GetVariableName(FEBioFluid::RELATIVE_FLUID_VELOCITY));
        m_dofEF = GetDOFIndex(FEBioFluid::GetVariableName(FEBioFluid::FLUID_DILATATION), 0);
    }
}

//-----------------------------------------------------------------------------
//! Calculate the residual for the prescribed normal velocity
void FEFluidNormalVelocity::LoadVector(FEGlobalVector& R)
{
    const FETimeInfo& tp = GetTimeInfo();

	FEDofList dofE(GetFEModel());
	dofE.AddDof(m_dofEF);
	m_psurf->LoadVector(R, dofE, false, [=](FESurfaceMaterialPoint& mp, const FESurfaceDofShape& dof_a, vector<double>& fa) {

		FESurfaceElement& el = *mp.SurfaceElement();

		// nodal coordinates
		vec3d rt[FEElement::MAX_NODES];
		m_psurf->GetNodalCoordinates(el, tp.alphaf, rt);

		// calculate the tangent vectors
		vec3d dxr = el.eval_deriv1(rt, mp.m_index);
		vec3d dxs = el.eval_deriv2(rt, mp.m_index);

		// normal velocity
		double vn = m_velocity(mp);
		double da = (dxr ^ dxs).norm();

		double H = dof_a.shape;
		fa[0] = H * vn * da;
	});
}

//-----------------------------------------------------------------------------
double FEFluidNormalVelocity::ScalarLoad(FESurfaceMaterialPoint& mp)
{
    return m_velocity(mp);
}

//-----------------------------------------------------------------------------
//! initialize
bool FEFluidNormalVelocity::Init()
{
    if (FESurfaceLoad::Init() == false) return false;
    
    m_dof.Clear();
    m_dof.AddDofs(m_dofW);
    m_dof.AddDof(m_dofEF);

    return true;
}

//-----------------------------------------------------------------------------
//! Activate the degrees of freedom for this BC
void FEFluidNormalVelocity::Activate()
{
	FESurface& surface = GetSurface();

	// evaluate surface normals
	vector<vec3d> sn(surface.Elements(), vec3d(0, 0, 0));
	m_nu.resize(surface.Nodes(), vec3d(0, 0, 0));
	vector<int> nf(surface.Nodes(), 0);
	vec3d r0[FEElement::MAX_NODES];
	for (int iel = 0; iel< surface.Elements(); ++iel)
	{
		FESurfaceElement& el = surface.Element(iel);

		// nr integration points
		int nint = el.GaussPoints();

		// nr of element nodes
		int neln = el.Nodes();

		// nodal coordinates
		for (int i = 0; i<neln; ++i) {
			r0[i] = surface.Node(el.m_lnode[i]).m_r0;
			++nf[el.m_lnode[i]];
		}

		double* Nr, *Ns;

		vec3d dxr, dxs;

		// repeat over integration points
		for (int n = 0; n<nint; ++n)
		{
			Nr = el.Gr(n);
			Ns = el.Gs(n);

			// calculate the tangent vectors at integration point
			dxr = dxs = vec3d(0, 0, 0);
			for (int i = 0; i<neln; ++i)
			{
				dxr += r0[i] * Nr[i];
				dxs += r0[i] * Ns[i];
			}

			sn[iel] = dxr ^ dxs;
			sn[iel].unit();
		}

		// evaluate nodal normals by averaging surface normals
		for (int i = 0; i<neln; ++i)
			m_nu[el.m_lnode[i]] += sn[iel];
	}

	for (int i = 0; i< surface.Nodes(); ++i) {
		m_nu[i].unit();
	}

    // Set up data structure for setting rim pressure
    if (m_brim) {
        // find rim nodes (rim)
        FEElemElemList EEL;
        EEL.Create(&surface);
        
        vector<bool> boundary(surface.Nodes(), false);
        for (int i=0; i< surface.Elements(); ++i) {
            FESurfaceElement& el = surface.Element(i);
            for (int j=0; j<el.facet_edges(); ++j) {
                FEElement* nel = EEL.Neighbor(i, j);
                if (nel == nullptr) {
                    int en[3] = {-1,-1,-1};
                    el.facet_edge(j, en);
                    boundary[en[0]] = true;
                    boundary[en[1]] = true;
                    if (en[2] > -1) boundary[en[2]] = true;
                }
            }
        }
        
        // store boundary nodes for which the velocity DOFs are fixed
        // which defines the rim on which the dilatation needs to be prescribed
        m_rim.reserve(surface.Nodes());
        for (int i=0; i<surface.Nodes(); ++i)
            if (boundary[i]) {
                FENode& node = surface.Node(i);
                if ((node.get_bc(m_dofW[0]) == DOF_FIXED) &&
                    (node.get_bc(m_dofW[1]) == DOF_FIXED) &&
                    (node.get_bc(m_dofW[2]) == DOF_FIXED))
                    m_rim.push_back(i);
            }
        
        if (m_rim.size() == surface.Nodes())
        {
            feLogError("Unable to set rim pressure\n");
        }
    }
    
    // Set parabolic velocity profile if requested.
    // Note that this may override any maps assigned to velocity
    if (m_bpar)
    {
        if (SetParabolicVelocity() == false)
        {
            feLogError("Unable to set parablic velocity\n");
        }
    }
    else
    {
        // We need to make sure that mapped values are defined at nodes. 
        FEMappedValue* val = dynamic_cast<FEMappedValue*>(m_velocity.valuator());
        if (val)
        {
            FESurfaceMap* map = dynamic_cast<FESurfaceMap*>(val->dataMap());
            if (map && map->StorageFormat() == Storage_Fmt::FMT_MULT)
            {
                const FEFacetSet* surf = map->GetFacetSet();
                FESurfaceMap* nodeMap = new FESurfaceMap(FE_DOUBLE);
                nodeMap->Create(map->GetFacetSet(), 0.0, FMT_NODE);

                vector<double> v(surface.Nodes(), 0.0);
                vector<int> tag(surface.Nodes(), 0);
                for (int i = 0; i < surface.Elements(); ++i)
                {
                    FESurfaceElement& el = surface.Element(i);
                    for (int j = 0; j < el.Nodes(); ++j)
                    {
                        double vj = map->value<double>(i, j);
                        v[el.m_lnode[j]] += vj;
                        tag[el.m_lnode[j]]++;
                    }
                }

                for (int i = 0; i < surface.Nodes(); ++i)
                {
                    int n = tag[i];
                    if (n != 0) v[i] /= (double)n;
                    nodeMap->set(i, v[i]);
                }

                val->setDataMap(nodeMap);
            }
        }
    }

    if (m_bpv) {
        for (int i = 0; i < surface.Nodes(); ++i)
        {
            FENode& node = surface.Node(i);
            // mark node as having prescribed DOF
            if (node.get_bc(m_dofW[0]) != DOF_FIXED) node.set_bc(m_dofW[0], DOF_PRESCRIBED);
            if (node.get_bc(m_dofW[1]) != DOF_FIXED) node.set_bc(m_dofW[1], DOF_PRESCRIBED);
            if (node.get_bc(m_dofW[2]) != DOF_FIXED) node.set_bc(m_dofW[2], DOF_PRESCRIBED);
        }
    }
    
    if (m_brim) {
        for (int i=0; i<m_rim.size(); ++i) {
            FENode& node = surface.Node(m_rim[i]);
            // mark node as having prescribed DOF
            if (node.get_bc(m_dofEF) != DOF_FIXED) node.set_bc(m_dofEF, DOF_PRESCRIBED);
        }
    }
}

//-----------------------------------------------------------------------------
//! Evaluate and prescribe the velocities
void FEFluidNormalVelocity::Update()
{
    // prescribe this velocity at the nodes
    FESurface* ps = &GetSurface();

   
    if (m_bpv) {

        // The velocity map is read in as surface data with MULT format. 
        // However, we need to have (unique) values at the nodes
        // so we need to convert this to nodal data. 
        int N = ps->Nodes();
        vector<double> VN(N, 0.0);
        vector<int> tag(N, 0);
        for (int i = 0; i < ps->Elements(); ++i)
        {
            FESurfaceElement& el = ps->Element(i);
            for (int j = 0; j < el.Nodes(); ++j)
            {
                FEMaterialPoint mp;
                mp.m_elem = &el;
                mp.m_index = j + 0x10000;

                double vnj = m_velocity(mp);

                VN[el.m_lnode[j]] += vnj;
                tag[el.m_lnode[j]]++;
            }
        }
        for (int i = 0; i < N; ++i)
        {
            if (tag[i] != 0) VN[i] /= (double)tag[i];
        }

        for (int i=0; i<ps->Nodes(); ++i)
        {
            // evaluate the velocity
            vec3d v = m_nu[i]*VN[i];
            FENode& node = ps->Node(i);
            if (node.get_bc(m_dofW[0]) == DOF_PRESCRIBED) node.set(m_dofW[0], v.x);
            if (node.get_bc(m_dofW[1]) == DOF_PRESCRIBED) node.set(m_dofW[1], v.y);
            if (node.get_bc(m_dofW[2]) == DOF_PRESCRIBED) node.set(m_dofW[2], v.z);
        }
    }
    
    if (m_brim) SetRimPressure();
    
    ForceMeshUpdate();
}

//-----------------------------------------------------------------------------
//! Evaluate normal velocities by solving Poiseuille flow across the surface
bool FEFluidNormalVelocity::SetParabolicVelocity()
{
    FEParabolicMap gen(GetFEModel());
    FESurfaceMap* map = new FESurfaceMap(FE_DOUBLE);
    FEFacetSet* surf = GetSurface().GetFacetSet(); assert(surf);

    // only consider fluid dofs
    gen.SetDOFConstraint(m_dofW);

    if (surf == nullptr) return false;
    map->Create(surf);
    if (gen.Generate(*map) == false)
    {
        assert(false);
        return false;
    }

    FEMappedValue* val = fecore_alloc(FEMappedValue, GetFEModel());
    val->setDataMap(map);

    if (m_velocity.isConst() == false) return false;
    double vn = m_velocity.constValue();
    val->setScaleFactor(vn);

    m_velocity.setValuator(val);

    return true;
}

//-----------------------------------------------------------------------------
//! Evaluate normal velocities by solving Poiseuille flow across the surface
bool FEFluidNormalVelocity::SetRimPressure()
{
    FESurface* ps = &GetSurface();
    
    // evaluate dilatation everywhere but rim
    double es = 0;
    int neq = 0;
    for (int i=0; i<ps->Nodes(); ++i) {
        FENode& node = ps->Node(i);
        if (node.get_bc(m_dofEF) == DOF_OPEN) {
            es += node.get(m_dofEF);
            ++neq;
        }
    }
    es /= neq;

    // set the rim pressure (dilatation)
    for (int i=0; i<m_rim.size(); ++i) {
        FENode& node = ps->Node(m_rim[i]);
        node.set(m_dofEF,es);
    }
    
    return true;
}

//-----------------------------------------------------------------------------
//! serializatsion
void FEFluidNormalVelocity::Serialize(DumpStream& ar)
{
	FESurfaceLoad::Serialize(ar);
	ar & m_nu;
}
