//	Greenplum Database
//	Copyright (C) 2018 Pivotal, Inc.

#include "unittest/dxl/CParseHandlerCostModelTest.h"

#include <memory>

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#include "gpdbcost/CCostModelParamsGPDB.h"
#include "gpdbcost/CCostModelParamsGPDBLegacy.h"
#include "gpos/base.h"
#include "gpos/test/CUnittest.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/common/CAutoP.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/io/COstreamString.h"
#include "gpopt/cost/ICostModel.h"
#include "naucrates/dxl/parser/CParseHandlerCostModel.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/xml/CDXLMemoryManager.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

namespace
{
	class Fixture
	{
		private:
			CAutoMemoryPool m_amp;
			gpos::CAutoP<CDXLMemoryManager> m_apmm;
			std::auto_ptr<SAX2XMLReader> m_apxmlreader;
			gpos::CAutoP<CParseHandlerManager> m_apphm;
			gpos::CAutoP<CParseHandlerCostModel> m_apphCostModel;

	public:
			Fixture():
					m_apmm(GPOS_NEW(Pmp()) CDXLMemoryManager(Pmp())),
					m_apxmlreader(XMLReaderFactory::createXMLReader(Pmm())),
					m_apphm(GPOS_NEW(Pmp()) CParseHandlerManager(Pmm(), Pxmlreader())),
					m_apphCostModel(GPOS_NEW(Pmp()) CParseHandlerCostModel(Pmp(), Pphm(), NULL))
			{
				m_apphm->ActivateParseHandler(PphCostModel());
			}

			IMemoryPool *Pmp() const
			{
				return m_amp.Pmp();
			}

			CDXLMemoryManager *Pmm()
			{
				return m_apmm.Pt();
			}

			SAX2XMLReader *Pxmlreader()
			{
				return m_apxmlreader.get();
			}

			CParseHandlerManager *Pphm()
			{
				return m_apphm.Pt();
			}

			CParseHandlerCostModel *PphCostModel()
			{
				return m_apphCostModel.Pt();
			}

			void Parse(const XMLByte szDXL[], size_t size)
			{
				MemBufInputSource mbis(
					szDXL,
					size,
					"dxl test",
					false,
					Pmm()
				);
				Pxmlreader()->parse(mbis);
			}

	};
}

static gpos::GPOS_RESULT Eres_CalibratedCostModel()
{
	Fixture fixture;

	IMemoryPool *pmp = fixture.Pmp();

	const XMLByte szDXL[] =
			"<dxl:CostModelConfig CostModelType=\"1\" SegmentsForCosting=\"3\" xmlns:dxl=\"http://greenplum.com/dxl/2010/12/\"/>";

	CParseHandlerCostModel *pphcm = fixture.PphCostModel();

	fixture.Parse(szDXL, sizeof(szDXL));

	ICostModel *pcm = pphcm->Pcm();

	GPOS_RTL_ASSERT(ICostModel::EcmtGPDBCalibrated == pcm->Ecmt());
	GPOS_RTL_ASSERT(3 == pcm->UlHosts());

	CAutoRef<CCostModelParamsGPDB> pcpExpected(GPOS_NEW(pmp) CCostModelParamsGPDB(pmp));
	GPOS_RTL_ASSERT(pcpExpected.Pt()->FEquals(pcm->Pcp()));

	return gpos::GPOS_OK;
}

static gpos::GPOS_RESULT Eres_LegacyCostModel()
{
	Fixture fixture;

	IMemoryPool *pmp = fixture.Pmp();

	const XMLByte szDXL[] =
			"<dxl:CostModelConfig CostModelType=\"0\" SegmentsForCosting=\"3\" xmlns:dxl=\"http://greenplum.com/dxl/2010/12/\"/>";

	CParseHandlerCostModel *pphcm = fixture.PphCostModel();

	fixture.Parse(szDXL, sizeof(szDXL));

	ICostModel *pcm = pphcm->Pcm();

	GPOS_RTL_ASSERT(ICostModel::EcmtGPDBLegacy == pcm->Ecmt());
	GPOS_RTL_ASSERT(3 == pcm->UlHosts());

	CAutoRef<CCostModelParamsGPDBLegacy> pcpExpected(GPOS_NEW(pmp) CCostModelParamsGPDBLegacy(pmp));
	GPOS_RTL_ASSERT(pcpExpected.Pt()->FEquals(pcm->Pcp()));

	return gpos::GPOS_OK;
}

gpos::GPOS_RESULT CParseHandlerCostModelTest::EresUnittest()
{
	CUnittest rgut[] =
			{
				GPOS_UNITTEST_FUNC(Eres_CalibratedCostModel),
				GPOS_UNITTEST_FUNC(Eres_LegacyCostModel),
			};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}
