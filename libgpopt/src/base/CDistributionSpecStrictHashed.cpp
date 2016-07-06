//	Greenplum Database
//	Copyright (C) 2016 Pivotal Software, Inc.

#include "gpopt/base/CDistributionSpecStrictHashed.h"

namespace gpopt
{
	CDistributionSpecStrictHashed::CDistributionSpecStrictHashed(DrgPexpr *pdrgpexpr, BOOL fNullsColocated)
		: CDistributionSpecHashed(pdrgpexpr, fNullsColocated) {}

	CDistributionSpec::EDistributionType CDistributionSpecStrictHashed::Edt() const
	{
		return CDistributionSpec::EdtStrictHashed;
	}

	BOOL CDistributionSpecStrictHashed::FMatch(const CDistributionSpec *pds) const
	{
		return Edt() == pds->Edt();
	}

	IOstream &CDistributionSpecStrictHashed::OsPrint(IOstream &os) const
	{
		return os << "Omer";
	}
}
