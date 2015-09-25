#pragma once
#include "FECore/FEPlotData.h"

//=============================================================================
//                            N O D E   D A T A
//=============================================================================

//-----------------------------------------------------------------------------
//! Nodal effective fluid pressures
class FEPlotFluidDilatation : public FEDomainData
{
public:
    FEPlotFluidDilatation(FEModel* pfem) : FEDomainData(PLT_FLOAT, FMT_NODE){}
    bool Save(FEDomain& m, vector<float>& a);
};

//=============================================================================
//							D O M A I N   D A T A
//=============================================================================

//-----------------------------------------------------------------------------
//! Element elastic fluid pressure
class FEPlotElasticFluidPressure : public FEDomainData
{
public:
	FEPlotElasticFluidPressure(FEModel* pfem) : FEDomainData(PLT_FLOAT, FMT_ITEM){}
	bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid volume ratio
class FEPlotFluidVolumeRatio : public FEDomainData
{
public:
    FEPlotFluidVolumeRatio(FEModel* pfem) : FEDomainData(PLT_FLOAT, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid density
class FEPlotFluidDensity : public FEDomainData
{
public:
    FEPlotFluidDensity(FEModel* pfem) : FEDomainData(PLT_FLOAT, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid velocity
class FEPlotFluidVelocity : public FEDomainData
{
public:
    FEPlotFluidVelocity(FEModel* pfem) : FEDomainData(PLT_VEC3F, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid acceleration
class FEPlotFluidAcceleration : public FEDomainData
{
public:
    FEPlotFluidAcceleration(FEModel* pfem) : FEDomainData(PLT_VEC3F, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid vorticity
class FEPlotFluidVorticity : public FEDomainData
{
public:
    FEPlotFluidVorticity(FEModel* pfem) : FEDomainData(PLT_VEC3F, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid stresses
class FEPlotElementFluidStress : public FEDomainData
{
public:
    FEPlotElementFluidStress(FEModel* pfem) : FEDomainData(PLT_MAT3FS, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};

//-----------------------------------------------------------------------------
//! Element fluid rate of deformation
class FEPlotElementFluidRateOfDef : public FEDomainData
{
public:
    FEPlotElementFluidRateOfDef(FEModel* pfem) : FEDomainData(PLT_MAT3FS, FMT_ITEM){}
    bool Save(FEDomain& dom, vector<float>& a);
};
