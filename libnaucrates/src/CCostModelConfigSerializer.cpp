//	Greenplum Database
//	Copyright (C) 2018 Pivotal Software, Inc.

#include "naucrates/dxl/CCostModelConfigSerializer.h"

#include "gpos/common/CAutoRef.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using gpos::CAutoRef;

void CCostModelConfigSerializer::Serialize(CXMLSerializer &xmlser) const
{
	xmlser.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostModelConfig));
	xmlser.AddAttribute(CDXLTokens::PstrToken(EdxltokenCostModelType), m_pcm->Ecmt());
	xmlser.AddAttribute(CDXLTokens::PstrToken(EdxltokenSegmentsForCosting), m_pcm->UlHosts());
	xmlser.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostModelConfig));
}

CCostModelConfigSerializer::CCostModelConfigSerializer
	(
	const gpopt::ICostModel *pcm
	)
	:
	m_pcm(pcm)
{
}
