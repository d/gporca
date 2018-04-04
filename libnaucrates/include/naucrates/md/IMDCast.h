//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		IMDCast.h
//
//	@doc:
//		Interface for cast functions in the metadata cache
//---------------------------------------------------------------------------



#ifndef GPMD_IMDCast_H
#define GPMD_IMDCast_H

#include "gpos/base.h"

#include "naucrates/md/IMDCacheObject.h"

namespace gpmd
{
using namespace gpos;


//---------------------------------------------------------------------------
//	@class:
//		IMDCast
//
//	@doc:
//		Interface for cast functions in the metadata cache
//
//---------------------------------------------------------------------------
class IMDCast : public IMDCacheObject
{
public:
	// type of coercion pathway
	enum EmdCoercepathType
	{
		EmdtNone,		 /* failed to find any coercion pathway */
		EmdtFunc,		 /* apply the specified coercion function */
		EmdtRelabelType, /* binary-compatible cast, no function */
		EmdtArrayCoerce  /* need an ArrayCoerceExpr node */
	};

	// object type
	virtual Emdtype
	Emdt() const
	{
		return EmdtCastFunc;
	}

	// source type
	virtual IMDId *
	PmdidSrc() const = 0;

	// destination type
	virtual IMDId *
	PmdidDest() const = 0;

	// is the cast between binary coercible types, i.e. the types are binary compatible
	virtual BOOL
	FBinaryCoercible() const = 0;

	// cast function id
	virtual IMDId *
	PmdidCastFunc() const = 0;

	// return the coercion path type
	virtual EmdCoercepathType
	EmdPathType() const = 0;
};

}  // namespace gpmd

#endif  // !GPMD_IMDCast_H

// EOF
