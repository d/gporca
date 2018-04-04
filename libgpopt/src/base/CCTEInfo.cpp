//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CCTEInfo.cpp
//
//	@doc:
//		Information about CTEs in a query
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/sync/CAutoMutex.h"

#include "gpopt/base/CCTEInfo.h"
#include "gpopt/base/CCTEReq.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/operators/CLogicalCTEProducer.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CExpressionPreprocessor.h"


using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::CCTEInfoEntry
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfoEntry::CCTEInfoEntry(IMemoryPool *pmp,
									   CExpression *pexprCTEProducer)
	: m_pmp(pmp),
	  m_pexprCTEProducer(pexprCTEProducer),
	  m_phmcrulConsumers(NULL),
	  m_fUsed(true)
{
	GPOS_ASSERT(NULL != pmp);
	GPOS_ASSERT(NULL != pexprCTEProducer);

	m_phmcrulConsumers = GPOS_NEW(pmp) HMCrUl(pmp);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::CCTEInfoEntry
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfoEntry::CCTEInfoEntry(IMemoryPool *pmp,
									   CExpression *pexprCTEProducer,
									   BOOL fUsed)
	: m_pmp(pmp),
	  m_pexprCTEProducer(pexprCTEProducer),
	  m_phmcrulConsumers(NULL),
	  m_fUsed(fUsed)
{
	GPOS_ASSERT(NULL != pmp);
	GPOS_ASSERT(NULL != pexprCTEProducer);

	m_phmcrulConsumers = GPOS_NEW(pmp) HMCrUl(pmp);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::~CCTEInfoEntry
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfoEntry::~CCTEInfoEntry()
{
	m_pexprCTEProducer->Release();
	m_phmcrulConsumers->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::AddConsumerCols
//
//	@doc:
//		Add given columns to consumers column map
//
//---------------------------------------------------------------------------
void
CCTEInfo::CCTEInfoEntry::AddConsumerCols(DrgPcr *pdrgpcr)
{
	GPOS_ASSERT(NULL != pdrgpcr);

	const ULONG ulSize = pdrgpcr->UlLength();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		if (NULL == m_phmcrulConsumers->PtLookup(pcr))
		{
#ifdef GPOS_DEBUG
			BOOL fSuccess =
#endif  // GPOS_DEBUG
				m_phmcrulConsumers->FInsert(pcr, GPOS_NEW(m_pmp) ULONG(ul));
			GPOS_ASSERT(fSuccess);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::UlConsumerColPos
//
//	@doc:
//		Return position of given consumer column,
//		return ULONG_MAX if column is not found in local map
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::CCTEInfoEntry::UlConsumerColPos(CColRef *pcr)
{
	GPOS_ASSERT(NULL != pcr);

	ULONG *pul = m_phmcrulConsumers->PtLookup(pcr);
	if (NULL == pul)
	{
		return ULONG_MAX;
	}

	return *pul;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::UlCTEId
//
//	@doc:
//		CTE id
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::CCTEInfoEntry::UlCTEId() const
{
	return CLogicalCTEProducer::PopConvert(m_pexprCTEProducer->Pop())
		->UlCTEId();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfo(IMemoryPool *pmp)
	: m_pmp(pmp),
	  m_phmulcteinfoentry(NULL),
	  m_ulNextCTEId(0),
	  m_fEnableInlining(true)
{
	GPOS_ASSERT(NULL != pmp);
	m_phmulcteinfoentry = GPOS_NEW(m_pmp) HMUlCTEInfoEntry(m_pmp);
	m_phmulprodconsmap = GPOS_NEW(m_pmp) HMUlProdConsMap(m_pmp);
}

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::~CCTEInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CCTEInfo::~CCTEInfo()
{
	CRefCount::SafeRelease(m_phmulcteinfoentry);
	CRefCount::SafeRelease(m_phmulprodconsmap);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PreprocessCTEProducer
//
//	@doc:
//		Preprocess CTE producer expression
//
//---------------------------------------------------------------------------
CExpression *
CCTEInfo::PexprPreprocessCTEProducer(const CExpression *pexprCTEProducer)
{
	GPOS_ASSERT(NULL != pexprCTEProducer);

	CExpression *pexprProducerChild = (*pexprCTEProducer)[0];

	// get cte output cols for preprocessing use
	CColRefSet *pcrsOutput =
		CLogicalCTEProducer::PopConvert(pexprCTEProducer->Pop())->pcrsOutput();

	CExpression *pexprChildPreprocessed =
		CExpressionPreprocessor::PexprPreprocess(m_pmp, pexprProducerChild,
												 pcrsOutput);

	COperator *pop = pexprCTEProducer->Pop();
	pop->AddRef();

	CExpression *pexprProducerPreprocessed =
		GPOS_NEW(m_pmp) CExpression(m_pmp, pop, pexprChildPreprocessed);

	pexprProducerPreprocessed->ResetStats();
	InitDefaultStats(pexprProducerPreprocessed);

	return pexprProducerPreprocessed;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::AddCTEProducer
//
//	@doc:
//		Add CTE producer to hashmap
//
//---------------------------------------------------------------------------
void
CCTEInfo::AddCTEProducer(CExpression *pexprCTEProducer)
{
	CExpression *pexprProducerToAdd =
		PexprPreprocessCTEProducer(pexprCTEProducer);

	COperator *pop = pexprCTEProducer->Pop();
	ULONG ulCTEId = CLogicalCTEProducer::PopConvert(pop)->UlCTEId();

#ifdef GPOS_DEBUG
	BOOL fInserted =
#endif
		m_phmulcteinfoentry->FInsert(
			GPOS_NEW(m_pmp) ULONG(ulCTEId),
			GPOS_NEW(m_pmp) CCTEInfoEntry(m_pmp, pexprProducerToAdd));
	GPOS_ASSERT(fInserted);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::ReplaceCTEProducer
//
//	@doc:
//		Replace cte producer with given expression
//
//---------------------------------------------------------------------------
void
CCTEInfo::ReplaceCTEProducer(CExpression *pexprCTEProducer)
{
	COperator *pop = pexprCTEProducer->Pop();
	ULONG ulCTEId = CLogicalCTEProducer::PopConvert(pop)->UlCTEId();

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->PtLookup(&ulCTEId);
	GPOS_ASSERT(NULL != pcteinfoentry);

#ifdef GPOS_DBUG
	CExpression *pexprCTEProducerOld = pcteinfoentry->Pexpr();
	COperator *popCTEProducerOld = pexprCTEProducerOld->Pop();
	GPOS_ASSERT(ulCTEId ==
				CLogicalCTEProducer::PopConvert(popCTEProducerOld)->UlCTEId());
#endif  // GPOS_DEBUG

	CExpression *pexprCTEProducerNew =
		PexprPreprocessCTEProducer(pexprCTEProducer);

#ifdef GPOS_DEBUG
	BOOL fReplaced =
#endif
		m_phmulcteinfoentry->FReplace(
			&ulCTEId, GPOS_NEW(m_pmp) CCTEInfoEntry(m_pmp, pexprCTEProducerNew,
													pcteinfoentry->FUsed()));
	GPOS_ASSERT(fReplaced);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::InitDefaultStats
//
//	@doc:
//		Initialize default statistics for a given CTE Producer
//
//---------------------------------------------------------------------------
void
CCTEInfo::InitDefaultStats(CExpression *pexprCTEProducer)
{
	// Generate statistics with empty requirement. This handles cases when
	// the CTE is a N-Ary join that will require statistics calculation

	CReqdPropRelational *prprel =
		GPOS_NEW(m_pmp) CReqdPropRelational(GPOS_NEW(m_pmp) CColRefSet(m_pmp));
	(void) pexprCTEProducer->PstatsDerive(prprel, NULL /* pdrgpstatCtxt */);

	// cleanup
	prprel->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::DeriveProducerStats
//
//	@doc:
//		Derive the statistics on the CTE producer
//
//---------------------------------------------------------------------------
void
CCTEInfo::DeriveProducerStats(CLogicalCTEConsumer *popConsumer,
							  CColRefSet *pcrsStat)
{
	const ULONG ulCTEId = popConsumer->UlCTEId();

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->PtLookup(&ulCTEId);
	GPOS_ASSERT(NULL != pcteinfoentry);

	CExpression *pexprCTEProducer = pcteinfoentry->Pexpr();

	// for multi-threaded optimization two consumers can potentially try deriving
	// stats on the producer at the same time, hence lock the mutex
	CAutoMutex am(pcteinfoentry->m_mutex);
	am.Lock();

	// Given the subset of CTE consumer columns needed for statistics derivation,
	// compute its corresponding set of columns in the CTE Producer
	CColRefSet *pcrsCTEProducer =
		CUtils::PcrsCTEProducerColumns(m_pmp, pcrsStat, popConsumer);
	GPOS_ASSERT(pcrsStat->CElements() == pcrsCTEProducer->CElements());

	CReqdPropRelational *prprel =
		GPOS_NEW(m_pmp) CReqdPropRelational(pcrsCTEProducer);
	(void) pexprCTEProducer->PstatsDerive(prprel, NULL /* pdrgpstatCtxt */);

	// cleanup
	prprel->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PexprCTEProducer
//
//	@doc:
//		Return the logical cte producer with given id
//
//---------------------------------------------------------------------------
CExpression *
CCTEInfo::PexprCTEProducer(ULONG ulCTEId) const
{
	const CCTEInfoEntry *pcteinfoentry =
		m_phmulcteinfoentry->PtLookup(&ulCTEId);
	GPOS_ASSERT(NULL != pcteinfoentry);

	return pcteinfoentry->Pexpr();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::UlConsumersInParent
//
//	@doc:
//		Number of consumers of given CTE inside a given parent
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::UlConsumersInParent(ULONG ulConsumerId, ULONG ulParentId) const
{
	// get map of given parent
	const HMUlConsumerMap *phmulconsumermap =
		m_phmulprodconsmap->PtLookup(&ulParentId);
	if (NULL == phmulconsumermap)
	{
		return 0;
	}

	// find counter of given consumer inside this map
	const SConsumerCounter *pconsumercounter =
		phmulconsumermap->PtLookup(&ulConsumerId);
	if (NULL == pconsumercounter)
	{
		return 0;
	}

	return pconsumercounter->UlCount();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::UlConsumers
//
//	@doc:
//		Return number of CTE consumers of given CTE
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::UlConsumers(ULONG ulCTEId) const
{
	// find consumers in main query
	ULONG ulConsumers = UlConsumersInParent(ulCTEId, ULONG_MAX);

	// find consumers in other CTEs
	HMUlCTEInfoEntryIter hmulei(m_phmulcteinfoentry);
	while (hmulei.FAdvance())
	{
		const CCTEInfoEntry *pcteinfoentry = hmulei.Pt();
		if (pcteinfoentry->FUsed())
		{
			ulConsumers +=
				UlConsumersInParent(ulCTEId, pcteinfoentry->UlCTEId());
		}
	}

	return ulConsumers;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::FUsed
//
//	@doc:
//		Check if given CTE is used
//
//---------------------------------------------------------------------------
BOOL
CCTEInfo::FUsed(ULONG ulCTEId) const
{
	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->PtLookup(&ulCTEId);
	GPOS_ASSERT(NULL != pcteinfoentry);
	return pcteinfoentry->FUsed();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::IncrementConsumers
//
//	@doc:
//		Increment number of CTE consumers
//
//---------------------------------------------------------------------------
void
CCTEInfo::IncrementConsumers(ULONG ulConsumerId, ULONG ulParentCTEId)
{
	// get map of given parent
	HMUlConsumerMap *phmulconsumermap = const_cast<HMUlConsumerMap *>(
		m_phmulprodconsmap->PtLookup(&ulParentCTEId));
	if (NULL == phmulconsumermap)
	{
		phmulconsumermap = GPOS_NEW(m_pmp) HMUlConsumerMap(m_pmp);
#ifdef GPOS_DEBUG
		BOOL fInserted =
#endif
			m_phmulprodconsmap->FInsert(GPOS_NEW(m_pmp) ULONG(ulParentCTEId),
										phmulconsumermap);
		GPOS_ASSERT(fInserted);
	}

	// find counter of given consumer inside this map
	SConsumerCounter *pconsumercounter = const_cast<SConsumerCounter *>(
		phmulconsumermap->PtLookup(&ulConsumerId));
	if (NULL == pconsumercounter)
	{
		// no existing counter - start a new one
#ifdef GPOS_DEBUG
		BOOL fInserted =
#endif
			phmulconsumermap->FInsert(GPOS_NEW(m_pmp) ULONG(ulConsumerId),
									  GPOS_NEW(m_pmp)
										  SConsumerCounter(ulConsumerId));
		GPOS_ASSERT(fInserted);
	}
	else
	{
		// counter already exists - just increment it
		// to be sure that no one else does this at the same time, lock the mutex
		CAutoMutex am(pconsumercounter->m_mutex);
		am.Lock();
		pconsumercounter->Increment();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PcterProducers
//
//	@doc:
//		Return a CTE requirement with all the producers as optional
//
//---------------------------------------------------------------------------
CCTEReq *
CCTEInfo::PcterProducers(IMemoryPool *pmp) const
{
	CCTEReq *pcter = GPOS_NEW(pmp) CCTEReq(pmp);

	HMUlCTEInfoEntryIter hmulei(m_phmulcteinfoentry);
	while (hmulei.FAdvance())
	{
		const CCTEInfoEntry *pcteinfoentry = hmulei.Pt();
		pcter->Insert(pcteinfoentry->UlCTEId(), CCTEMap::EctProducer,
					  false /*fRequired*/, NULL /*pdpplan*/);
	}

	return pcter;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PdrgPexpr
//
//	@doc:
//		Return an array of all stored CTE expressions
//
//---------------------------------------------------------------------------
DrgPexpr *
CCTEInfo::PdrgPexpr(IMemoryPool *pmp) const
{
	DrgPexpr *pdrgpexpr = GPOS_NEW(pmp) DrgPexpr(pmp);
	HMUlCTEInfoEntryIter hmulei(m_phmulcteinfoentry);
	while (hmulei.FAdvance())
	{
		CExpression *pexpr = const_cast<CExpression *>(hmulei.Pt()->Pexpr());
		pexpr->AddRef();
		pdrgpexpr->Append(pexpr);
	}

	return pdrgpexpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::MapComputedToUsedCols
//
//	@doc:
//		Walk the producer expressions and add the mapping between computed
//		column and its used columns
//
//---------------------------------------------------------------------------
void
CCTEInfo::MapComputedToUsedCols(CColumnFactory *pcf) const
{
	HMUlCTEInfoEntryIter hmulei(m_phmulcteinfoentry);
	while (hmulei.FAdvance())
	{
		CExpression *pexprProducer =
			const_cast<CExpression *>(hmulei.Pt()->Pexpr());
		GPOS_ASSERT(NULL != pexprProducer);
		CQueryContext::MapComputedToUsedCols(pcf, pexprProducer);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::AddConsumerCols
//
//	@doc:
//		Add given columns to consumers column map
//
//---------------------------------------------------------------------------
void
CCTEInfo::AddConsumerCols(ULONG ulCTEId, DrgPcr *pdrgpcr)
{
	GPOS_ASSERT(NULL != pdrgpcr);

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->PtLookup(&ulCTEId);
	GPOS_ASSERT(NULL != pcteinfoentry);

	CAutoMutex am(pcteinfoentry->m_mutex);
	am.Lock();

	pcteinfoentry->AddConsumerCols(pdrgpcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::UlConsumerColPos
//
//	@doc:
//		Return position of given consumer column in consumer output
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::UlConsumerColPos(ULONG ulCTEId, CColRef *pcr)
{
	GPOS_ASSERT(NULL != pcr);

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->PtLookup(&ulCTEId);
	GPOS_ASSERT(NULL != pcteinfoentry);

	CAutoMutex am(pcteinfoentry->m_mutex);
	am.Lock();

	return pcteinfoentry->UlConsumerColPos(pcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::FindConsumersInParent
//
//	@doc:
//		Find all CTE consumers inside given parent, and push them to the given stack
//
//---------------------------------------------------------------------------
void
CCTEInfo::FindConsumersInParent(ULONG ulParentId, CBitSet *pbsUnusedConsumers,
								CStack<ULONG> *pstack)
{
	HMUlConsumerMap *phmulconsumermap = const_cast<HMUlConsumerMap *>(
		m_phmulprodconsmap->PtLookup(&ulParentId));
	if (NULL == phmulconsumermap)
	{
		// no map found for given parent - there are no consumers inside it
		return;
	}

	HMUlConsumerMapIter hmulci(phmulconsumermap);
	while (hmulci.FAdvance())
	{
		const SConsumerCounter *pconsumercounter = hmulci.Pt();
		ULONG ulConsumerId = pconsumercounter->UlCTEId();
		if (pbsUnusedConsumers->FBit(ulConsumerId))
		{
			pstack->Push(GPOS_NEW(m_pmp) ULONG(ulConsumerId));
			pbsUnusedConsumers->FExchangeClear(ulConsumerId);
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::MarkUnusedCTEs
//
//	@doc:
//		Mark unused CTEs
//
//---------------------------------------------------------------------------
void
CCTEInfo::MarkUnusedCTEs()
{
	CBitSet *pbsUnusedConsumers = GPOS_NEW(m_pmp) CBitSet(m_pmp);

	// start with all CTEs
	HMUlCTEInfoEntryIter hmulei(m_phmulcteinfoentry);
	while (hmulei.FAdvance())
	{
		const CCTEInfoEntry *pcteinfoentry = hmulei.Pt();
		pbsUnusedConsumers->FExchangeSet(pcteinfoentry->UlCTEId());
	}

	// start with the main query and find out which CTEs are used there
	CStack<ULONG> stack(m_pmp);
	FindConsumersInParent(ULONG_MAX, pbsUnusedConsumers, &stack);

	// repeatedly find CTEs that are used in these CTEs
	while (!stack.FEmpty())
	{
		// get one CTE id from list, and see which consumers are inside this CTE
		ULONG *pulCTEId = stack.Pop();
		FindConsumersInParent(*pulCTEId, pbsUnusedConsumers, &stack);
		GPOS_DELETE(pulCTEId);
	}

	// now the only CTEs remaining in the bitset are the unused ones. mark them as such
	HMUlCTEInfoEntryIter hmulei2(m_phmulcteinfoentry);
	while (hmulei2.FAdvance())
	{
		CCTEInfoEntry *pcteinfoentry =
			const_cast<CCTEInfoEntry *>(hmulei2.Pt());
		if (pbsUnusedConsumers->FBit(pcteinfoentry->UlCTEId()))
		{
			pcteinfoentry->MarkUnused();
		}
	}

	pbsUnusedConsumers->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PhmulcrConsumerToProducer
//
//	@doc:
//		Return a map from Id's of consumer columns in the given column set
//		to their corresponding producer columns in the given column array
//
//---------------------------------------------------------------------------
HMUlCr *
CCTEInfo::PhmulcrConsumerToProducer(
	IMemoryPool *pmp, ULONG ulCTEId,
	CColRefSet *pcrs,		 // set of columns to check
	DrgPcr *pdrgpcrProducer  // producer columns
)
{
	GPOS_ASSERT(NULL != pcrs);
	GPOS_ASSERT(NULL != pdrgpcrProducer);

	HMUlCr *phmulcr = GPOS_NEW(pmp) HMUlCr(pmp);

	CColRefSetIter crsi(*pcrs);
	while (crsi.FAdvance())
	{
		CColRef *pcr = crsi.Pcr();
		ULONG ulPos = UlConsumerColPos(ulCTEId, pcr);

		if (ULONG_MAX != ulPos)
		{
			GPOS_ASSERT(ulPos < pdrgpcrProducer->UlLength());

			CColRef *pcrProducer = (*pdrgpcrProducer)[ulPos];
#ifdef GPOS_DEBUG
			BOOL fSuccess =
#endif  // GPOS_DEBUG
				phmulcr->FInsert(GPOS_NEW(pmp) ULONG(pcr->UlId()), pcrProducer);
			GPOS_ASSERT(fSuccess);
		}
	}
	return phmulcr;
}


// EOF
