/*=============================================================================
	UnArc.h: Unreal archive class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Archive.
-----------------------------------------------------------------------------*/

//
// Archive class. Used for loading, saving, and garbage collecting
// in a byte order neutral way.
//
class FArchive
{
public:
	// FArchive interface.
    virtual ~FArchive() noexcept(false)
	{}
	virtual void Serialize( void* V, INT Length )
	{}
	virtual void SerializeBits( void* V, INT LengthBits )
	{
		Serialize( V, (LengthBits+7)/8 );
		if( IsLoading() )
			((BYTE*)V)[LengthBits/8] &= ((1<<(LengthBits&7))-1);
	}
	virtual void SerializeInt( DWORD& Value, DWORD Max )
	{
		ByteOrderSerialize(&Value, sizeof(Value));
	}
	virtual void Preload( UObject* Object )
	{}
	virtual void CountBytes( size_t InNum, size_t InMax )
	{}
	virtual FArchive& operator<<( class FName& N )
	{
		return *this;
	}
	virtual FArchive& operator<<( class UObject*& Res )
	{
		return *this;
	}
	virtual INT MapName( FName* Name )
	{
		return 0;
	}
	virtual INT MapObject( UObject* Object )
	{
		return 0;
	}
	virtual INT Tell()
	{
		return INDEX_NONE;
	}
	virtual INT TotalSize()
	{
		return INDEX_NONE;
	}
	virtual UBOOL AtEnd()
	{
		INT Pos = Tell();
		return Pos!=INDEX_NONE && Pos>=TotalSize();
	}
	virtual void Seek( INT InPos )
	{}
	virtual void AttachLazyLoader( FLazyLoader* LazyLoader )
	{}
	virtual void DetachLazyLoader( FLazyLoader* LazyLoader )
	{}
	virtual void Precache( INT HintCount )
	{}
	virtual void Flush()
	{}
	virtual UBOOL Close()
	{
		return !ArIsError;
	}
	virtual UBOOL GetError()
	{
		return ArIsError;
	}

	// Hardcoded datatype routines that may not be overridden.
	FArchive& ByteOrderSerialize( void* V, INT Length )
	{
#if __INTEL_BYTE_ORDER__
		Serialize( V, Length );
#else
		if( ArIsPersistent )
		{
			// Transferring between memory and file, so flip the byte order.
			for( INT i=Length-1; i>=0; i-- )
				Serialize( (BYTE*)V + i, 1 );
		}
		else
		{
			// Transferring around within memory, so keep the byte order.
			Serialize( V, Length );
		}
#endif
		return *this;
	}

	// Constructor.
	FArchive()
	:	ArVer			(PACKAGE_FILE_VERSION)
	,	ArNetVer		(ENGINE_NEGOTIATION_VERSION)
	,	ArLicenseeVer	(PACKAGE_FILE_VERSION_LICENSEE)
	,	ArIsLoading		(FALSE)
	,	ArIsSaving		(FALSE)
	,   ArIsTrans		(FALSE)
	,	ArIsPersistent  (FALSE)
	,	ArForEdit		(TRUE)
	,	ArForClient		(TRUE)
	,	ArForServer		(TRUE)
	,   ArIsError       (FALSE)
    ,	ArIsCriticalError(FALSE) //elmuerte: security fix
    ,	ArMaxSerializeSize(0) //elmuerte: security fix
	,	ArIsObjectReferenceCollector(FALSE)
	{}

	// Status accessors.
	INT Ver()				{return ArVer;}
	INT NetVer()			{return ArNetVer&0x7fffffff;}
#if 1 //LVer added by Legend on 4/12/2000
	INT LicenseeVer()		{return ArLicenseeVer;}
#endif
	UBOOL IsLoading()		{return ArIsLoading;}
	UBOOL IsSaving()		{return ArIsSaving;}
	UBOOL IsTrans()			{return ArIsTrans;}
	UBOOL IsNet()			{return (ArNetVer&0x80000000)!=0;}
	UBOOL IsPersistent()    {return ArIsPersistent;}
	UBOOL IsError()         {return ArIsError;}
	UBOOL IsCriticalError() {return ArIsCriticalError;} //elmuerte: security fix
	UBOOL ForEdit()			{return ArForEdit;}
	UBOOL ForClient()		{return ArForClient;}
	UBOOL ForServer()		{return ArForServer;}
	inline UBOOL IsObjectReferenceCollector() const { return ArIsObjectReferenceCollector; }

	// Friend archivers.
	friend FArchive& operator<<( FArchive& Ar, ANSICHAR& C )
	{
		Ar.Serialize( &C, 1 );
		return Ar;
	}
	#if UNICODE
	friend FArchive& operator<<( FArchive& Ar, UNICHAR& U )
	{
		Ar.ByteOrderSerialize( &U, sizeof(U) ); // Smirftsch, added for Unicode
		return Ar;
	}
	#endif
	friend FArchive& operator<<( FArchive& Ar, BYTE& B )
	{
		Ar.Serialize( &B, 1 );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, SBYTE& B )
	{
		Ar.Serialize( &B, 1 );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, _WORD& W )
	{
		Ar.ByteOrderSerialize( &W, sizeof(W) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, SWORD& S )
	{
		Ar.ByteOrderSerialize( &S, sizeof(S) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, DWORD& D )
	{
		Ar.ByteOrderSerialize( &D, sizeof(D) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, INT& I )
	{
		Ar.ByteOrderSerialize( &I, sizeof(I) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, FLOAT& F )
	{
		Ar.ByteOrderSerialize( &F, sizeof(F) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, DOUBLE& F )
	{
		Ar.ByteOrderSerialize( &F, sizeof(F) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive &Ar, QWORD& Q )
	{
		Ar.ByteOrderSerialize( &Q, sizeof(Q) );
		return Ar;
	}
	friend FArchive& operator<<( FArchive& Ar, SQWORD& S )
	{
		Ar.ByteOrderSerialize( &S, sizeof(S) );
		return Ar;
	}


	friend FArchive& operator<<( FArchive& Ar, FTime& F );
	friend FArchive& operator<<( FArchive& Ar, FString& A ); //elmuerte: security fix
protected:
	// Status variables.
	INT ArVer;
	INT ArNetVer;
	INT ArLicenseeVer;
	UBOOL ArIsLoading;
	UBOOL ArIsSaving;
	UBOOL ArIsTrans;
	UBOOL ArIsPersistent;
	UBOOL ArForEdit;
	UBOOL ArForClient;
	UBOOL ArForServer;
	UBOOL ArIsError;
    UBOOL ArIsCriticalError; //elmuerte: security fix
    INT ArMaxSerializeSize; //elmuerte: security fix
	UBOOL ArIsObjectReferenceCollector;
};

/*-----------------------------------------------------------------------------
	FArchive macros.
-----------------------------------------------------------------------------*/

//
// Class for serializing objects in a compactly, mapping small values
// to fewer bytes.
//
class FCompactIndex
{
public:
	INT Value;
	FCompactIndex(void) : Value(0) {}
	CORE_API friend FArchive& operator<<( FArchive& Ar, FCompactIndex& I );
};

//
// Archive constructor.
//
template <class T> T Arctor( FArchive& Ar )
{
	T Tmp;
	Ar << Tmp;
	return Tmp;
}

// Macro to serialize an integer as a compact index.
#define AR_INDEX(intref) \
	(*(FCompactIndex*)&(intref))

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/