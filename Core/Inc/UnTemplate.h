/*=============================================================================
	UnTemplate.h: Unreal templates.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Type information.
-----------------------------------------------------------------------------*/

#pragma once

//
// Type information for initialization.
//
template <class T> struct TTypeInfoBase
{
public:
	typedef const T& ConstInitType;
	static UBOOL NeedsDestructor() {return 1;}
	static UBOOL DefinitelyNeedsDestructor() {return 0;}
	static const T& ToInit( const T& In ) {return In;}
};
template <class T> struct TTypeInfo : public TTypeInfoBase<T>
{
};

template <> struct TTypeInfo<BYTE> : public TTypeInfoBase<BYTE>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SBYTE> : public TTypeInfoBase<SBYTE>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<ANSICHAR> : public TTypeInfoBase<ANSICHAR>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<INT> : public TTypeInfoBase<INT>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<DWORD> : public TTypeInfoBase<DWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<_WORD> : public TTypeInfoBase<_WORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SWORD> : public TTypeInfoBase<SWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<QWORD> : public TTypeInfoBase<QWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<SQWORD> : public TTypeInfoBase<SQWORD>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<FName> : public TTypeInfoBase<FName>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};
template <> struct TTypeInfo<UObject*> : public TTypeInfoBase<UObject*>
{
public:
	static UBOOL NeedsDestructor() {return 0;}
};

/*-----------------------------------------------------------------------------
	Standard templates.
-----------------------------------------------------------------------------*/

template< class T > inline T Abs( const T A )
{
	return (A>=(T)0) ? A : -A;
}
template< class T > inline T Sgn( const T A )
{
	return (A>0) ? 1 : ((A<0) ? -1 : 0);
}
template< class T > inline T Max( const T A, const T B )
{
	return (A>=B) ? A : B;
}
template< class T > inline T Max( const T A, const T B, const T C )
{
	T X = Max<T>( A, B );
	return (X>=C) ? X : C;
}
template< class T > inline T Max( const T A, const T B, const T C, const T D )
{
	T X = Max<T>( A, B );
	T Y = Max<T>( C, D );
	return (X>=Y) ? X : Y;
}
template< class T > inline T Max3(const T A, const T B, const T C)
{
	return Max(Max(A, B), C);
}
template< class T > inline T Min( const T A, const T B )
{
	return (A<=B) ? A : B;
}
template< class T > inline T Min( const T A, const T B, const T C )
{
	T X = Min<T>( A, B );
	return (X<=C) ? X : C;
}
template< class T > inline T Min( const T A, const T B, const T C, const T D )
{
	T X = Min<T>( A, B );
	T Y = Min<T>( C, D );
	return (X<=Y) ? X : Y;
}
template< class T > inline T Min3(const T A, const T B, const T C)
{
	return Min(Min(A, B), C);
}
template< class T > inline T Square( const T A )
{
	return A*A;
}
template< class T > inline T Cube( const T A )
{
	return A*A*A;
}
template< class T > inline T Quad( const T A )
{
	return A*A*A*A;
}
template< class T > inline T Clamp( const T X, const T Min, const T Max )
{
	return X<Min ? Min : X<Max ? X : Max;
}
template< class T > inline T Align( const T Ptr, uintptr_t Alignment )
{
	return (T)(((uintptr_t)Ptr + Alignment - 1) & ~(Alignment - 1));
}
template< class T > inline void Exchange( T& A, T& B )
{
	const T Temp = A;
	A = B;
	B = Temp;
}
template< class T > T Lerp( T& A, T& B, FLOAT Alpha )
{
	return (T)(A + Alpha * (B - A));
}
/*
template< class T > T* CastUnsafe( UObject* Src )
{
	return Src && Src->GetClass()->IsClassType(T::StaticClass()->CastFlags) ? (T*)Src : NULL;
}
*/
inline DWORD GetTypeHash( const BYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const SBYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const _WORD A )
{
	return A;
}
inline DWORD GetTypeHash( const SWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const INT A )
{
	return A;
}
inline DWORD GetTypeHash( const DWORD A )
{
	return A;
}
inline DWORD GetTypeHash(const QWORD A)
{
	return (DWORD)A ^ ((DWORD)(A >> 16)) ^ ((DWORD)(A >> 32));
}
inline DWORD GetTypeHash(const SQWORD A)
{
	return (DWORD)A ^ ((DWORD)(A >> 16)) ^ ((DWORD)(A >> 32));
}
/*
inline DWORD GetTypeHash( const QWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const SQWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
*/
inline DWORD GetTypeHash( const TCHAR* S )
{
	return appStrihash(S);
}
#define ExchangeB(A,B) {UBOOL T=A; A=B; B=T;}

/*----------------------------------------------------------------------------
	Standard macros.
----------------------------------------------------------------------------*/

// Number of elements in an array.
#define ARRAY_COUNT( array ) \
	( int(sizeof(array) / sizeof((array)[0])) )

// Offset of a struct member.
#define STRUCT_OFFSET( struc, member ) \
	( (intptr_t)&((struc*)NULL)->member )

// stijn: the above macro is invalid in standard C++
// the one below is valid as long as struc is a standard layout data type
//#define STRUCT_OFFSET(struc, member) offsetof(struc, member)

inline UBOOL appIsWhitespace(TCHAR C)
{
	return (C == ' ' || C == '\t');
}

/*-----------------------------------------------------------------------------
	Allocators.
-----------------------------------------------------------------------------*/

template <class T> class TAllocator
{};

/*-----------------------------------------------------------------------------
	Dynamic array template.
-----------------------------------------------------------------------------*/

//
// Base dynamic array.
//
class CORE_API FArray
{
public:
	void* GetData()
	{
		return Data;
	}
	const void* GetData() const
	{
		return Data;
	}
	UBOOL IsValidIndex( INT i ) const
	{
		return i>=0 && i<ArrayNum;
	}
	inline INT Num() const
	{
		//checkSlow(ArrayNum>=0);
		//checkSlow(ArrayMax>=ArrayNum);
		return ArrayNum;
	}

	void InsertZeroed( INT Index, INT Count, INT ElementSize )
	{
		guardSlow(FArray::InsertZeroed);
		// gam ---
		checkSlow(ElementSize>0);
		// sjs rem'd -trips all over- checkSlow(Count>0);
		checkSlow(Index>=0);
		checkSlow(Index<=ArrayNum);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		// --- gam
		Insert( Index, Count, ElementSize );
		appMemzero( (BYTE*)Data+Index*ElementSize, Count*ElementSize );
		unguardSlow;
	}
	void Insert( INT Index, INT Count, INT ElementSize )
	{
		guardSlow(FArray::Insert);
		checkSlow(Count>=0);
		// gam ---
		checkSlow(ElementSize>0);
		// sjs rem'd -trips all over- checkSlow(Count>0);
		// --- gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(Index>=0);
		checkSlow(Index<=ArrayNum);

		INT OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = ArrayNum + 3*ArrayNum/8 + 32;
			Realloc( ElementSize );
		}
		appMemmove
		(
			(BYTE*)Data + (Index+Count )*ElementSize,
			(BYTE*)Data + (Index       )*ElementSize,
			              (OldNum-Index)*ElementSize
		);

		unguardSlow;
	}
	INT AddNoCheck (INT Count)
	{
		INT Index = ArrayNum;
		ArrayNum += Count;
		return Index;
	}
	INT Add( INT Count, INT ElementSize )
	{
		guardSlow(FArray::Add);
		// gam ---
		// sjs rem'd -trips all over- checkSlow(Count>0);
		checkSlow(ElementSize>0);
		// --- gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		INT Index = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = ArrayNum + 3*ArrayNum/8 + 32;
			Realloc( ElementSize );
		}

		return Index;
		unguardSlow;
	}
	INT AddZeroed( INT ElementSize, INT n=1 )
	{
		guardSlow(FArray::AddZeroed);
		// gam ---
		checkSlow(ElementSize>0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		// --- gam

		INT Index = Add( n, ElementSize );
		appMemzero( (BYTE*)Data+Index*ElementSize, n*ElementSize );
		return Index;
		unguardSlow;
	}
	void Shrink( INT ElementSize )
	{
		guardSlow(FArray::Shrink);
		checkSlow(ElementSize>0); // gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if( ArrayMax != ArrayNum )
		{
			ArrayMax = ArrayNum;
			Realloc( ElementSize );
		}
		unguardSlow;
	}
	// sjs ---
	void SetSize( INT Count, INT ElementSize )
	{
		guardSlow(FArray::SetSize);
		// gam ---
		checkSlow(Count>=0);
		checkSlow(ElementSize>0);
		// --- gam
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		ArrayNum = ArrayMax = Count;
		Realloc( ElementSize );
		unguardSlow;
	}
	// --- sjs
	void Empty( INT ElementSize, INT Slack=0 )
	{
		guardSlow(FArray::Empty);
		// gam ---
		checkSlow(ElementSize>0);
		checkSlow(Slack>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		// --- gam
		ArrayNum = 0;
		ArrayMax = Slack;
		Realloc( ElementSize );
		unguardSlow;
	}
	FArray()
	:	Data	( NULL )
	,	ArrayNum( 0 )
	,	ArrayMax( 0 )
	{}
	FArray( ENoInit )
	{}
	~FArray()
	{
		guard(FArray::~FArray);
		EmptyFArray(TEXT("~FArray"));
		unguard;
	}
	void EmptyFArray(const TCHAR* Tag)
	{
		guard(FArray::EmptyFArray);
		if (GExitPurge)
			return;
		if (Data)
		{
			void* TData = Data;
			Data = NULL;
			appFree(TData);
		}
		ArrayNum = ArrayMax = 0;
		unguardf((TEXT("(%ls %p)"), Tag, this));
	}
	void CountBytes( FArchive& Ar, INT ElementSize )
	{
		guardSlow(FArray::CountBytes);
		Ar.CountBytes( ArrayNum*ElementSize, ArrayMax*ElementSize );
		unguardSlow;
	}
	void Remove( INT Index, INT Count, INT ElementSize );
protected:
	void Realloc( INT ElementSize );
	FArray( INT InNum, INT ElementSize )
	:	Data    ( NULL  )
	,	ArrayNum( InNum )
	,	ArrayMax( InNum )
	{
		if(InNum)
			Realloc( ElementSize );
	}
	void* Data;
	INT	  ArrayNum;
	INT	  ArrayMax;
};

//
// Templated dynamic array.
//
template< class T > class TArray : public FArray
{
public:
	typedef T ElementType;
	TArray()
	:	FArray()
	{}
	TArray( INT InNum )
	:	FArray( InNum, sizeof(T) )
	{}
	TArray( const TArray& Other )
	:	FArray( Other.ArrayNum, sizeof(T) )
	{
		guardSlow(TArray::copyctor);
		if( TTypeInfo<T>::NeedsDestructor() )
		{
			ArrayNum=0;
			for( INT i=0; i<Other.ArrayNum; i++ )
				new(*this)T(Other(i));
		}
		else if( sizeof(T)!=1 )
		{
			for( INT i=0; i<ArrayNum; i++ )
				(*this)(i) = Other(i);
		}
		else if ( ArrayNum>0 )
		{
            guardSlow(DoCopy);
			appMemcpy( &(*this)(0), &Other(0), ArrayNum * sizeof(T) );
			unguardSlow;
		}
		unguardSlow;
	}
	TArray( ENoInit )
	: FArray( E_NoInit )
	{}
	~TArray()
	{
		guard(~TArray);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if (TTypeInfo<T>::NeedsDestructor())
			for (INT i = 0; i < ArrayNum; i++)
				(&(*this)(i))->~T();
		EmptyFArray(TEXT("~TArray"));
		unguard;
	}
	T& operator()( INT i )
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		//checkSlow(ArrayMax>0);
        //checkSlow(Data)
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(ArrayMax>0);
        checkSlow(Data)
		return ((T*)Data)[i];
		unguardSlow;
	}
	T Pop()
	{
		guardSlow(TArray::Pop);
		check(ArrayNum>0);
		checkSlow(ArrayMax>=ArrayNum);
		T Result = ((T*)Data)[ArrayNum-1];
		Remove( ArrayNum-1 );
		return Result;
		unguardSlow;
	}
	T& Top()
	{
		guardSlow(TArray::Top);
		check(ArrayNum > 0);
		checkSlow(ArrayMax >= ArrayNum);
		return ((T*)Data)[ArrayNum - 1];
		unguardSlow;
	}
	const T& Top() const
	{
		guardSlow(TArray::Top);
		check(ArrayNum > 0);
		checkSlow(ArrayMax >= ArrayNum);
		return ((T*)Data)[ArrayNum - 1];
		unguardSlow;
	}
	T& Last( INT c=0 )
	{
		guardSlow(TArray::Last);
		check(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
		unguardSlow;
	}
	const T& Last( INT c=0 ) const
	{
		guardSlow(TArray::Last);
		checkSlow(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
		unguardSlow;
	}
	void Shrink()
	{
		guardSlow(TArray::Shrink);
		FArray::Shrink( sizeof(T) );
		unguardSlow;
	}
	UBOOL FindItem( const T& Item, INT& Index ) const
	{
		guardSlow(TArray::FindItem);
		for( Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return 1;
		return 0;
		unguardSlow;
	}
	INT FindItemIndex( const T& Item ) const
	{
		guardSlow(TArray::FindItemIndex);
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return INDEX_NONE;
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TArray& A )
	{
		guard(TArray<<);
		A.CountBytes( Ar );
		if( sizeof(T)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << A.ArrayNum;
			if( Ar.IsLoading() )
			{
				A.ArrayMax = A.ArrayNum;
				A.Realloc( sizeof(T) );
			}
            // gam ---
			if( A.Num() > 0 )
				Ar.Serialize( &A(0), A.Num() );
			else
				Ar.Serialize( NULL, 0 );
            // --- gam
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum = 0; // gam
			Ar << NewNum;
			A.Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
				Ar << *new(A)T;
		}
		else
		{
			// Save array.
			Ar << A.ArrayNum;
			for( INT i=0; i<A.ArrayNum; i++ )
				Ar << A( i );
		}
		return Ar;
		unguard;
	}
	void CountBytes( FArchive& Ar )
	{
		guardSlow(TArray::CountBytes);
		FArray::CountBytes( Ar, sizeof(T) );
		unguardSlow;
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT n=1 )
	{
		guardSlow(TArray::Add);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		return FArray::Add( n, sizeof(T) );
		unguardSlow;
	}
	// stijn: used in Render - only to be used if you've preallocated enough bytes (e.g., with Reserve)
	INT AddNoCheck (INT n=1)
	{
		return FArray::AddNoCheck(n);
	}
	// sjs ---
	void SetSize( INT n=1 )
	{
		guardSlow(TArray::SetSize);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::SetSize( n, sizeof(T) );
		unguardSlow;
	}
	// --- sjs
	void Insert( INT Index, INT Count=1 )
	{
		guardSlow(TArray::Insert);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::Insert( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void InsertZeroed( INT Index, INT Count=1 )
	{
		guardSlow(TArray::InsertZeroed);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		FArray::InsertZeroed( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void Remove( INT Index, INT Count=1 )
	{
		guardSlow(TArray::Remove);
		check(Index>=0);
		check(Index<=ArrayNum);
		check(Index+Count<=ArrayNum);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=Index; i<Index+Count; i++ )
				(&(*this)(i))->~T();
		FArray::Remove( Index, Count, sizeof(T) );
		unguardSlow;
	}
	void Empty( INT Slack=0 )
	{
		guardSlow(TArray::Empty);
		if( TTypeInfo<T>::NeedsDestructor() )
			for( INT i=0; i<ArrayNum; i++ )
				(&(*this)(i))->~T();
		FArray::Empty( sizeof(T), Slack );
		unguardSlow;
	}

	// Functions dependent on Add, Remove.
	TArray& operator+( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			for( INT i=0; i<Other.ArrayNum; i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	TArray& operator+=( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			*this = *this + Other;
		}
		return *this;
		unguardSlow;
	}
	TArray& operator=( const TArray& Other )
	{
		guardSlow(TArray::operator=);
		if( this != &Other )
		{
			Empty( Other.ArrayNum );
			for( INT i=0; i<Other.ArrayNum; i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	INT AddItem( const T& Item )
	{
		guardSlow(TArray::AddItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		new(*this) T(Item);
		return Num() - 1;
		unguardSlow;
	}
	INT AddZeroed( INT n=1 )
	{
		guardSlow(TArray::AddZeroed);
		return FArray::AddZeroed( sizeof(T), n );
		unguardSlow;
	}
	INT AddUniqueItem( const T& Item )
	{
		guardSlow(TArray::AddUniqueItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
		unguardSlow;
	}
	INT RemoveItem( const T& Item )
	{
		guardSlow(TArray::RemoveItem);
		INT OriginalNum=ArrayNum;
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - ArrayNum;
		unguardSlow;
	}
	void InsertItem(INT Index, const T& Item)
	{
		guardSlow(TArray::InsertItem);
		Insert(Index);
		(*this)(Index) = Item;
		unguardSlow;
	}
	// stijn: used in Render
	void EmptyNoRealloc()
	{
		ArrayNum = 0;
	}
	// stijn: used in Render
	void Reserve(INT NewArrayMax)
	{
		if (NewArrayMax > ArrayMax)
		{
			ArrayMax = NewArrayMax;
			Realloc(sizeof(T));
		}
	}

	// Iterator.
	class TIterator
	{
	public:
		TIterator( TArray<T>& InArray ) : Array(InArray), Index(-1) { ++*this;      }
		void operator++()      { ++Index;                                           }
		void RemoveCurrent()   { Array.Remove(Index--); }
		INT GetIndex()   const { return Index;                                      }
		operator UBOOL() const { return Index < Array.Num();                        }
		T& operator*()   const { return Array(Index);                               }
		T* operator->()  const { return &Array(Index);                              }
		T& GetCurrent()  const { return Array( Index );                             }
		T& GetPrev()     const { return Array( Index ? Index-1 : Array.Num()-1 );   }
		T& GetNext()     const { return Array( Index<Array.Num()-1 ? Index+1 : 0 ); }
	private:
		TArray<T>& Array;
		INT Index;
	};
};
template< class T > class TArrayNoInit : public TArray<T>
{
public:
	TArrayNoInit()
	: TArray<T>(E_NoInit)
	{}
	TArrayNoInit& operator=(const TArrayNoInit& Other)
	{
		TArray<T>::operator=(Other);
		return *this;
	}
};
//
// Array operator news.
//
template <class T> void* operator new( size_t Size, TArray<T>& Array )
{
	guardSlow(TArray::operator new);
	INT Index = Array.FArray::Add(1,sizeof(T));
	return &Array(Index);
	unguardSlow;
}
template <class T> void* operator new( size_t Size, TArray<T>& Array, INT Index )
{
	guardSlow(TArray::operator new);
	Array.FArray::Insert(Index,1,sizeof(T));
	return &Array(Index);
	unguardSlow;
}

//
// Array exchanger.
//
template <class T> inline void ExchangeArray( TArray<T>& A, TArray<T>& B )
{
	guardSlow(ExchangeTArray);
	appMemswap( &A, &B, sizeof(FArray) );
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Transactional array.
-----------------------------------------------------------------------------*/

template< class T > class TTransArray : public TArray<T>
{
public:
	// Constructors.
	TTransArray( UObject* InOwner, INT InNum=0 )
	:	TArray<T>( InNum )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray( UObject* InOwner, const TArray<T>& Other )
	:	TArray<T>( Other )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray& operator=( const TTransArray& Other )
	{
		operator=( (const TArray<T>&)Other );
		return *this;
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT Count=1 )
	{
		guardSlow(TTransArray::Add);
		INT Index = TArray<T>::Add( Count );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		return Index;
		unguardSlow;
	}
	void Insert( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::InsertZeroed);
		FArray::Insert( Index, Count, sizeof(T) );
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void Remove( INT Index, INT Count=1 )
	{
		guardSlow(TTransArray::Remove);
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, Count, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::Remove( Index, Count );
		unguardSlow;
	}
	void Empty( INT Slack=0 )
	{
		guardSlow(TTransArray::Empty);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, this->ArrayNum, -1, sizeof(T), SerializeItem, DestructItem );
		TArray<T>::Empty( Slack );
		unguardSlow;
	}

	// Functions dependent on Add, Remove.
	TTransArray& operator=( const TArray<T>& Other )
	{
		guardSlow(TTransArray::operator=);
		if( this != &Other )
		{
			Empty( Other.Num() );
			for( INT i=0; i<Other.Num(); i++ )
				new( *this )T( Other(i) );
		}
		return *this;
		unguardSlow;
	}
	INT AddItem( const T& Item )
	{
		guardSlow(TTransArray::AddItem);
		checkSlow(!TTypeInfo<T>::DefinitelyNeedsDestructor());
		new(*this) T(Item);
		return TArray<T>::Num() - 1;
		unguardSlow;
	}
	INT AddZeroed( INT n=1 )
	{
		guardSlow(TTransArray::AddZeroed);
		INT Index = Add(n);
		appMemzero( &(*this)(Index), n*sizeof(T) );
		return Index;
		unguardSlow;
	}
	INT AddUniqueItem( const T& Item )
	{
		guardSlow(TTransArray::AddUniqueItem);
		for( INT Index=0; Index<this->ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
		unguardSlow;
	}
	INT RemoveItem( const T& Item )
	{
		guardSlow(TTransArray::RemoveItem);
		INT OriginalNum=this->ArrayNum;
		for( INT Index=0; Index<this->ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - this->ArrayNum;
		unguardSlow;
	}

	// TTransArray interface.
	UObject* GetOwner()
	{
		return Owner;
	}
	void ModifyItem( INT Index )
	{
		guardSlow(TTransArray::ModifyItem);
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, 1, 0, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	void ModifyAllItems()
	{
		guardSlow(TTransArray::ModifyAllItems);
		if( GUndo )
			GUndo->SaveArray( Owner, this, 0, this->Num(), 0, sizeof(T), SerializeItem, DestructItem );
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TTransArray& A )
	{
		guard(TTransArray<<);
		if( !Ar.IsTrans() )
			Ar << (TArray<T>&)A;
		return Ar;
		unguard;
	}
protected:
	static void SerializeItem( FArchive& Ar, void* TPtr )
	{
		guardSlow(TArray::SerializeItem);
		Ar << *(T*)TPtr;
		unguardSlow;
	}
	static void DestructItem( void* TPtr )
	{
		guardSlow(TArray::SerializeItem);
		((T*)TPtr)->~T();
		unguardSlow;
	}
	UObject* Owner;
private:

	// Disallow the copy constructor.
	TTransArray( const TArray<T>& Other )
	{}
};

//
// Transactional array operator news.
//
template <class T> void* operator new( size_t Size, TTransArray<T>& Array )
{
	guardSlow(TArray::operator new);
	INT Index = Array.Add();
	return &Array(Index);
	unguardSlow;
}
template <class T> void* operator new( size_t Size, TTransArray<T>& Array, INT Index )
{
	guardSlow(TArray::operator new);
	Array.Insert(Index);
	return &Array(Index);
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Lazy loading.
-----------------------------------------------------------------------------*/

//
// Lazy loader base class.
//
class FLazyLoader
{
	friend class ULinkerLoad;
	friend class UFontExporter;
protected:
	FArchive*	 SavedAr;
	INT          SavedPos;
public:
	FLazyLoader()
	: SavedAr( NULL )
	, SavedPos( 0 )
	{}
	virtual void Load() _VF_BASE;
	virtual void Unload() _VF_BASE;
};

//
// Lazy-loadable dynamic array.
//
template <class T> class TLazyArray : public TArray<T>, public FLazyLoader
{
public:
	TLazyArray( INT InNum=0 )
	: TArray<T>( InNum )
	, FLazyLoader()
	{}
	~TLazyArray() noexcept(false)
	{
		guard(TLazyArray::~TLazyArray);
		if( SavedAr )
			SavedAr->DetachLazyLoader( this );
		unguard;
	}
#if LOAD_ON_DEMAND /* Breaks because of untimely accesses of operator() !! */
    T& operator()( INT i )
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( SavedPos>0 )
			Load();
		return ((T*)Data)[i];
		unguardSlow;
	}
	const T& operator()( INT i ) const
	{
		guardSlow(TArray::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		if( SavedPos>0 )
			Load();
		return ((T*)Data)[i];
		unguardSlow;
	}
#endif
	void Load()
	{
		// Make sure this array is loaded.
		guard(TLazyArray::Load);
		if( SavedPos>0 )
		{
			// Lazy load it now.
			INT PushedPos = SavedAr->Tell();
			SavedAr->Seek( SavedPos );
			*SavedAr << (TArray<T>&)*this;
			SavedPos *= -1;
			SavedAr->Seek( PushedPos );
		}
		unguard;
	}
	void DetachLazyLoader()
	{
		if( SavedAr )
			SavedAr->DetachLazyLoader( this );
		SavedPos = 0;
		SavedAr = NULL;
	}
	void Unload()
#if __GNUG__
		;	// Function declaration.
#else
	{
		// Make sure this array is unloaded.
		guard(TLazyArray::Unload);
		if( SavedPos<0 )
		{
			// Unload it now.
			TArray<T>::Empty();
			SavedPos *= -1;
		}
		unguard;
	}
#endif
	friend FArchive& operator<<( FArchive& Ar, TLazyArray& This )
	{
		guard(TLazyArray<<);
		if( Ar.IsLoading() )
		{
			INT SeekPos=0;
			if( Ar.Ver() <= 61 )
			{
				//oldver: Handles dynamic arrays of fixed-length serialized items only.
				Ar.AttachLazyLoader( &This );
				INT SkipCount = 0; // gam
				Ar << AR_INDEX(SkipCount);
				SeekPos = Ar.Tell() + SkipCount*sizeof(T);

			}
			else
			{
				Ar << SeekPos;
				Ar.AttachLazyLoader( &This );
			}
			if( !GLazyLoad )
				This.Load();
			Ar.Seek( SeekPos );
		}
		else if( Ar.IsSaving() && Ar.Ver()>61 )
		{
			// Save out count for skipping over this data.
			INT CountPos = Ar.Tell();
			Ar << CountPos << (TArray<T>&)This;
			INT EndPos = Ar.Tell();
			Ar.Seek( CountPos );
			Ar << EndPos;
			Ar.Seek( EndPos );
		}
		else Ar << (TArray<T>&)This;
		return Ar;
		unguard;
	}
};
#if __GNUG__
template <class T> void TLazyArray<T>::Unload()
{
	// Make sure this array is unloaded.
	 guard(TLazyArray::Unload);
	 if( SavedPos<0 && GLazyLoad )
	 {
		// Unload it now.
		this->Empty();
		SavedPos *= -1;
	 }
	 unguard;
}
#endif

/*-----------------------------------------------------------------------------
	Dynamic strings.
-----------------------------------------------------------------------------*/

//
// A dynamically sizeable string.
//
class FString : protected TArray<TCHAR>
{
public:
	FString()
	: TArray<TCHAR>()
	{}
	FString( const FString& Other )
	: TArray<TCHAR>( Other.ArrayNum )
	{
		if( ArrayNum )
			appMemcpy( &(*this)(0), &Other(0), ArrayNum*sizeof(TCHAR) );
	}
	FString( const TCHAR* In )
	: TArray<TCHAR>((In && *In) ? (appStrlen(In) + 1) : 0)
	{
		if( ArrayNum )
			appMemcpy( &(*this)(0), In, ArrayNum*sizeof(TCHAR) );
	}
	FString(const TCHAR* Start, const TCHAR* End)
		: TArray<TCHAR>((End > Start) ? ((End - Start) + 1) : 0)
	{
		if (ArrayNum)
		{
			appMemcpy(&(*this)(0), Start, (ArrayNum - 1) * sizeof(TCHAR));
			(*this)(ArrayNum - 1) = 0;
		}
	}
	FString ( const TCHAR* In, INT MaxBytes )
	: TArray<TCHAR>( MaxBytes + 1 )
	{
		if ( ArrayNum && In && appStrlen(In) > MaxBytes)
		{
			appMemcpy(&(*this)(0), In, MaxBytes * sizeof(TCHAR));
			(*this)(MaxBytes) = 0;
		}
	}
	explicit FString(INT InCount, const TCHAR* InSrc)
		: TArray<TCHAR>(InCount ? InCount + 1 : 0)
	{
		if (ArrayNum)
		{
			appStrncpy(&(*this)(0), InSrc, InCount + 1);
		}
	}
	FString( ENoInit )
	: TArray<TCHAR>( E_NoInit )
	{}
	explicit FString( BYTE   Arg, INT Digits=1 );
	explicit FString( SBYTE  Arg, INT Digits=1 );
	explicit FString( _WORD  Arg, INT Digits=1 );
	explicit FString( SWORD  Arg, INT Digits=1 );
	explicit FString( INT    Arg, INT Digits=1 );
	explicit FString( DWORD  Arg, INT Digits=1 );
	explicit FString( FLOAT  Arg, INT Digits=1, INT RightDigits=0, UBOOL LeadZero=1 );
	explicit FString( DOUBLE Arg, INT Digits=1, INT RightDigits=0, UBOOL LeadZero=1 );
	FString& operator=( const TCHAR* Other )
	{
	    guardSlow(FString:: operator=(const TCHAR*));
		if( (Data == NULL) || (&(*this)(0)!=Other) ) // gam
		{
			// Added check for Other beeing NULL before derencing. --han
			ArrayNum = ArrayMax = (Other && *Other) ? appStrlen(Other) + 1 : 0;
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
		unguardSlow;
	}
	FString& operator=( const FString& Other )
	{
		guardSlow(FString:: operator=(const FString&));
		if( Data==NULL || this!=&Other )
		{
			checkSlow(Other.Num()>=0);
			//checkSlow(Other.GetData()!=NULL);
			ArrayNum = ArrayMax = Other.Num();
			guardSlow(DoRealloc);
			Realloc( sizeof(TCHAR) );
			unguardSlow;
			if( ArrayNum )
			{
				guardSlow(DoCopy);
				checkSlow(GetData() != NULL);
				appMemcpy( GetData(), Other.GetData(), ArrayNum*sizeof(TCHAR) );
				unguardSlow;
			}
		}
		return *this;
		unguardSlow;
	}
    TCHAR& operator[]( INT i )
	{
		guardSlow(FString::operator());
		checkSlow(i>=0);
		checkSlow(i<=ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((TCHAR*)Data)[i];
		unguardSlow;
	}
	const TCHAR& operator[]( INT i ) const
	{
		guardSlow(FString::operator());
		checkSlow(i>=0);
		checkSlow(i<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((TCHAR*)Data)[i];
		unguardSlow;
	}

	~FString()
	{
		EmptyFArray(TEXT("~FString"));
	}
	void Empty()
	{
		TArray<TCHAR>::Empty();
	}
	void Shrink()
	{
		TArray<TCHAR>::Shrink();
	}
	const TCHAR* operator*() const
	{
		return Num() ? &(*this)(0) : TEXT("");
	}
	operator UBOOL() const
	{
		return Num()!=0;
	}
	TArray<TCHAR>& GetCharArray()
	{
		//warning: Operations on the TArray<CHAR> can be unsafe, such as adding
		// non-terminating 0's or removing the terminating zero.
		return (TArray<TCHAR>&)*this;
	}
	FString& operator+=( const TCHAR* Str )
	{
		if( Str && *Str != '\0' ) // gam
		{
			if( ArrayNum )
			{
				INT Index = ArrayNum-1;
				Add( appStrlen(Str) );
				appStrcpy( &(*this)(Index), Str ); // stijn: mem safety ok
			}
			else if( *Str )
			{
				Add( appStrlen(Str)+1 );
				appStrcpy( &(*this)(0), Str ); // stijn: mem safety ok
			}
		}
		return *this;
	}
	FString& operator+=(const TCHAR Ch)
	{
		if (Ch)
		{
			if (ArrayNum)
			{
				INT Index = ArrayNum - 1;
				Add(1);
				(*this)(Index) = Ch;
				(*this)(Index + 1) = 0;
			}
			else
			{
				Add(2);
				(*this)(0) = Ch;
				(*this)(1) = 0;
			}
		}
		return *this;
	}
	FString& operator+=( const FString& Str )
	{
		return operator+=( *Str );
	}
	FString operator+( const TCHAR* Str )
	{
		return FString( *this ) += Str;
	}
	FString operator+( const FString& Str )
	{
		return operator+( *Str );
	}
	FString& operator*=( const TCHAR* Str )
	{
		if( ArrayNum>1 && (*this)(ArrayNum-2)!=PATH_SEPARATOR[0] )
			*this += PATH_SEPARATOR;
		return *this += Str;
	}
	FString& operator*=( const FString& Str )
	{
		return operator*=( *Str );
	}
	FString operator*( const TCHAR* Str ) const
	{
		return FString( *this ) *= Str;
	}
	FString operator*( const FString& Str ) const
	{
		return operator*( *Str );
	}
	UBOOL operator<=( const TCHAR* Other ) const
	{
		return !(appStricmp( **this, Other ) > 0);
	}
	UBOOL operator<( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other ) < 0;
	}
	UBOOL operator>=( const TCHAR* Other ) const
	{
		return !(appStricmp( **this, Other ) < 0);
	}
	UBOOL operator>( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other ) > 0;
	}
	UBOOL operator==( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other )==0;
	}
	UBOOL operator==( const FString& Other ) const
	{
		return appStricmp( **this, *Other )==0;
	}
	UBOOL operator!=( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other )!=0;
	}
	UBOOL operator!=( const FString& Other ) const
	{
		return appStricmp( **this, *Other )!=0;
	}
	INT Len() const
	{
		return Num() ? Num()-1 : 0;
	}
	FString Left( INT Count ) const
	{
		return FString( Clamp(Count,0,Len()), **this );
	}
	FString LeftChop( INT Count ) const
	{
		return FString( Clamp(Len()-Count,0,Len()), **this );
	}
	FString Right( INT Count ) const
	{
		return FString( **this + Len()-Clamp(Count,0,Len()) );
	}
	FString Mid( INT Start, INT Count=MAXINT ) const
	{
		DWORD End = (DWORD)Start+(DWORD)Count;
		Start    = Clamp( (DWORD)Start, (DWORD)0,     (DWORD)Len() );
		End      = Clamp( (DWORD)End,   (DWORD)Start, (DWORD)Len() );
		return FString( End-Start, **this + Start );
	}
	FString FStringReplace( FString Replace, FString With ) const
	{
	   guard(FStringReplace);
       int i;
       FString InputT;             FString Text( **this );

       InputT = Text;
       Text = TEXT("");
       i = InputT.InStr(Replace);
       while(i != -1)
       {
           Text = Text + InputT.Left(i) + With;
           InputT = InputT.Mid(i + Replace.Len());
           i = InputT.InStr(Replace);
       }
       Text = Text+ InputT;
	   return Text;

	   unguard;
	}
	FString GetFilenameOnly() const
	{
		guard(GetFilenameOnly);
		const TCHAR* Start = **this;
		const TCHAR* S = Start;
		while (*S)
		{
			if (*S == '/' || *S == '\\')
				Start = S + 1;
			++S;
		}
		const TCHAR* End = Start;
		S = Start;
		while (*S)
		{
			if (*S == '.')
				End = S;
			++S;
		}
		return FString(Start, End);
		unguard;
	}
	inline UBOOL IsNumeric() const
	{
		if (!Len())
			return FALSE;
		const TCHAR* S = **this;
		while (*S)
		{
			if (!appIsDigit(*S))
				return FALSE;
			++S;
		}
		return TRUE;
	}
	UBOOL Divide(FString Src, FString Divider, FString& LeftPart, FString& RightPart)
	{
		guard(UParse::FStringDivide);
		int div_pos = Src.InStr(Divider);

		if (div_pos != -1)
		{
			LeftPart = Src.Left(div_pos);
			RightPart = Src.Mid(LeftPart.Len() + Divider.Len(), Src.Len() - LeftPart.Len() - Divider.Len());
			return 1;
		}
		else
			return 0;

		unguard;
	}
	INT InStr( const TCHAR* SubStr, UBOOL Right=0 ) const
	{
		if( !Right )
		{
			const TCHAR* Tmp = appStrstr(**this,SubStr); //Unicode changes for Win32 - Smirftsch
			//TCHAR* Tmp = appStrstr(**this,SubStr); oldver.

			return Tmp ? (Tmp-**this) : -1;
		}
		else
		{
			for( INT i=Len()-1; i>=0; i-- )
			{
				INT j;
				for( j=0; SubStr[j]; j++ )
					if( (*this)(i+j)!=SubStr[j] )
						break;
				if( !SubStr[j] )
					return i;
			}
			return -1;
		}
	}
	INT InStr( const FString& SubStr, UBOOL Right=0 ) const
	{
		return InStr( *SubStr, Right );
	}
	INT InStr2(const FString &SubStr, INT Start = 0) const
	{
		if (Start < 0)
			Start = 0;
		if (SubStr.Len() > Len() - Start)
			return -1;
		const TCHAR* Tmp = appStrstr(**this + Start, *SubStr);
		return Tmp ? Tmp - **this : -1;
	}
	/*
	INT InStr2( FString SubStr, INT Start=0 )
    {
       FString Src( **this );
       if(Start <= 0 || Start >= Src.Len()) return Src.InStr(SubStr);
             FString tmp = Src.Left(Start);
		return Src.FStringReplace(tmp,FString(TEXT(""))).InStr(SubStr)+tmp.Len();
    }
	*/
	UBOOL StartsWith(const TCHAR* Str) const
	{
		const TCHAR* S = **this;
		while (*Str)
		{
			if (appToLower(*S) != appToLower(*Str))
				return FALSE;
			++S;
			++Str;
		}
		return TRUE;
	}
	UBOOL Split( const FString& InS, FString* LeftS, FString* RightS, UBOOL Right=0 ) const
	{
		INT InPos = InStr(InS,Right);
		if( InPos<0 )
			return 0;
		if( LeftS )
			*LeftS = Left(InPos);
		if( RightS )
			*RightS = Mid(InPos+InS.Len());
		return 1;
	}
	FString Caps() const
	{
		FString New( **this );
		for( INT i=0; i<ArrayNum; i++ )
			New(i) = appToUpper(New(i));
		return New;
	}
	FString Locs() const
	{
		FString New( **this );
		for( INT i=0; i<ArrayNum; i++ )
			New(i) = appToLower(New(i));
		return New;
	}
	FString Reverse() const
	{
		FString New(*this);
		New.ReverseString();
		return New;
	}
	FString ConvertTabsToSpaces(INT NumSpaces) const
	{
		TCHAR* Tabs = appStaticString1024();
		for (INT i = 0; i < NumSpaces; ++i)
			Tabs[i] = ' ';
		Tabs[NumSpaces] = 0;
		return Replace(TEXT("\t"), Tabs);
	}
	void ReverseString()
	{
		if (Len() > 0)
		{
			TCHAR* StartChar = &(*this)(0);
			TCHAR* EndChar = &(*this)(Len() - 1);
			TCHAR TempChar;
			do
			{
				TempChar = *StartChar;	// store the current value of StartChar
				*StartChar = *EndChar;	// change the value of StartChar to the value of EndChar
				*EndChar = TempChar;	// change the value of EndChar to the character that was previously at StartChar

				StartChar++;
				EndChar--;

			} while (StartChar < EndChar);	// repeat until we've reached the midpoint of the string
		}
	}

	FString Replace(const TCHAR* Find, const TCHAR* ReplaceWith) const
	{
		FString Result = *this;
		INT FromLen = appStrlen(Find);
		INT i = Result.InStr(Find);
		while (i != -1)
		{
			Result = Result.Left(i) + ReplaceWith + Result.Mid(i + FromLen);
			i = Result.InStr(Find);
		}
		return Result;
	}
	INT Int() const
	{
		guardSlow(FString::Int);
		return appAtoi(**this);
		unguardSlow;
	}
	FLOAT Float() const
	{
		guardSlow(FString::Float);
		return appAtof(**this);
		unguardSlow;
	}
	UBOOL IsAlpha()
	{
		guard(FString::IsAlpha);
		for (INT Pos = 0; Pos < Len(); Pos++)
			if (!appIsAlpha((*this)(Pos)))
				return 0;
		return 1;
		unguard;
	}
	UBOOL IsName(UBOOL AllowDigitStart = 0)
	{
		guard(FString::IsName);
		if (Len() == 0)
			return 1;
		if (!AllowDigitStart && appIsDigit((*this)(0)))
			return 0;
		for (INT Pos = 0; Pos < Len(); Pos++)
			if (!appIsAlnum((*this)(Pos)) && (*this)(Pos) != '_')
				return 0;
		return 1;
		unguard;
	}
	FString TrimQuotes(UBOOL* bQuotesRemoved = NULL) const
	{
		UBOOL bQuotesWereRemoved = FALSE;
		INT Start = 0, Count = Len();
		if (Count > 0)
		{
			if ((*this)[0] == TCHAR('"'))
			{
				Start++;
				Count--;
				bQuotesWereRemoved = TRUE;
			}

			if (Len() > 1 && (*this)[Len() - 1] == TCHAR('"'))
			{
				Count--;
				bQuotesWereRemoved = TRUE;
			}
		}

		if (bQuotesRemoved != NULL)
			*bQuotesRemoved = bQuotesWereRemoved;
		return Mid(Start, Count);
	}

	// Tries to preserve the dot and a zero afterwards, but ditches otherwise needless zeros.
	static FString NiceFloat(FLOAT Value)
	{
		guard(FString::NiceFloat);
		FString Text = FString::Printf(TEXT("%g"), Value);
		if (Text.InStr(TEXT(".")) == INDEX_NONE)
			Text += TEXT(".0");
		return Text;
		unguard;
	}
	FString LeftPad( INT ChCount );
	FString RightPad( INT ChCount );
	static FString Printf( const TCHAR* Fmt, ... );
	static FString Printf(const TCHAR* Fmt, va_list Args);
	static FString Chr( TCHAR Ch );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FString& S );
	friend struct FStringNoInit;

	INT ParseIntoArray(TArray<FString>* InArray, const TCHAR* pchDelim, UBOOL InCullEmpty) const
	{
		InArray->Empty();
		const TCHAR* Start = &(*this)(0);
		INT DelimLength = appStrlen(pchDelim);
		if (Start && DelimLength)
		{
			while (const TCHAR* At = appStrstr(Start, pchDelim))
			{
				if (!InCullEmpty || At - Start)
				{
					new (*InArray) FString(At - Start, Start);
				}
				Start += DelimLength + (At - Start);
			}
			if (!InCullEmpty || *Start)
			{
				new(*InArray) FString(Start);
			}

		}
		return InArray->Num();
	}
	FString Trim()
	{
		INT Pos = 0;
		while (Pos < Len())
		{
			if (appIsWhitespace((*this)[Pos]))
				Pos++;
			else break;
		}
		*this = Right(Len() - Pos);
		return *this;
	}
	FString TrimTrailing()
	{
		INT Pos = Len() - 1;
		while (Pos >= 0)
		{
			if (!appIsWhitespace((*this)[Pos]))
				break;
			Pos--;
		}
		*this = Left(Pos + 1);
		return(*this);
	}
};

#define FSTRING(str) FString(TEXT(str))

struct CORE_API FStringNoInit : public FString
{
	FStringNoInit()
	: FString( E_NoInit )
	{}
	FStringNoInit& operator=( const TCHAR* Other )
	{
	    guardSlow(FString:: operator=(const TCHAR*));
		if( (Data == NULL) || (&(*this)(0)!=Other) ) // gam
		{
			ArrayNum = ArrayMax = (Other && *Other) ? appStrlen(Other) + 1 : 0;
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
		unguardSlow;
	}
	FStringNoInit& operator=( const FString& Other )
	{
		if (Data == NULL || this != &Other)
		{
			ArrayNum = ArrayMax = Other.Num();
			Realloc( sizeof(TCHAR) );
			if( ArrayNum )
				appMemcpy( &(*this)(0), *Other, ArrayNum*sizeof(TCHAR) );
		}
		return *this;
	}
};
inline DWORD GetTypeHash( const FString& S )
{
	return appStrihash(*S);
}
template <> struct TTypeInfo<FString> : public TTypeInfoBase<FString>
{
	typedef const TCHAR* ConstInitType;
	static const TCHAR* ToInit( const FString& In ) {return *In;}
	static UBOOL DefinitelyNeedsDestructor() {return 0;}
};

//
// String exchanger.
//
inline void ExchangeString( FString& A, FString& B )
{
	guardSlow(ExchangeTArray);
	appMemswap( &A, &B, sizeof(FString) );
	unguardSlow;
}

/*----------------------------------------------------------------------------
	Special archivers.
----------------------------------------------------------------------------*/

//
// String output device.
//
class FStringOutputDevice : public FString, public FOutputDevice
{
public:
	FStringOutputDevice( const TCHAR* InStr=TEXT("") )
	: FString( InStr )
	{}
	void Serialize( const TCHAR* Data, EName Event )
	{
		*this += (TCHAR*)Data;
	}
};

//
// Buffer writer.
//
class FBufferWriter : public FArchive
{
public:
	FBufferWriter( TArray<BYTE>& InBytes )
	: Bytes( InBytes )
	, Pos( 0 )
	{
		ArIsSaving = 1;
	}
	void Serialize( void* InData, INT Length )
	{
		if( Pos+Length>Bytes.Num() )
			Bytes.Add( Pos+Length-Bytes.Num() );
		if( Length == 1 )
			Bytes(Pos) = ((BYTE*)InData)[0];
		else if (Length > 0)
			appMemcpy( &Bytes(Pos), InData, Length );
		Pos += Length;
	}
	INT Tell()
	{
		return Pos;
	}
	void Seek( INT InPos )
	{
		Pos = InPos;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}
private:
	TArray<BYTE>& Bytes;
	INT Pos;
};

//
// Buffer archiver.
//
class FBufferArchive : public FBufferWriter, public TArray<BYTE>
{
public:
	FBufferArchive()
	: FBufferWriter( (TArray<BYTE>&)*this )
	{}
};

//
// Buffer reader.
//
class CORE_API FBufferReader : public FArchive
{
public:
	FBufferReader( const TArray<BYTE>& InBytes )
	:	Bytes	( InBytes )
	,	Pos 	( 0 )
	{
		ArIsLoading = ArIsTrans = 1;
	}
	void Serialize( void* Data, INT Num )
	{
		checkSlow(Pos>=0);
		checkSlow(Pos+Num<=Bytes.Num());
		if( Num == 1 )
			((BYTE*)Data)[0] = Bytes(Pos);
		else if ( Num > 0)
			appMemcpy( Data, &Bytes(Pos), Num );
		Pos += Num;
	}
	INT Tell()
	{
		return Pos;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}
	void Seek( INT InPos )
	{
		check(InPos>=0);
		check(InPos<=Bytes.Num());
		Pos = InPos;
	}
	UBOOL AtEnd()
	{
		return Pos>=Bytes.Num();
	}
private:
	const TArray<BYTE>& Bytes;
	INT Pos;
};

/*----------------------------------------------------------------------------
	TMap.
----------------------------------------------------------------------------*/

// TMapBase but for UnrealScript codebase. Written by Marco
class FScriptMapBase
{
public:
	struct USPair
	{
		INT HashNext, HashValue;
		BYTE* Key;
		BYTE* Value;

		USPair(BYTE* InKey, BYTE* InValue, INT iHash)
			: HashValue(iHash), Key(InKey), Value(InValue)
		{}
		~USPair()
		{
			appFree(Key);
			appFree(Value);
		}
	};
	TArray<USPair> Pairs;
	INT* Hash;
	INT HashCount;

	inline void RehashRecommended()
	{
		if (Pairs.Num() == 0)
		{
			if (Hash)
				delete[] Hash;
			Hash = NULL;
			HashCount = 0;
		}
		else
		{
			HashCount = 1 << appCeilLogTwo((Pairs.Num() >> 1) + 8);
			Rehash();
		}
	}
	inline void Rehash()
	{
		guardSlow(FScriptMapBase::Rehash);
		HashCount = Max(HashCount, 8);
		INT i, HashMask = (HashCount - 1);
		INT* NewHash = new INT[HashCount];
		appMemzero(NewHash, HashCount*sizeof(INT));
		for (i = 0; i < Pairs.Num(); i++)
		{
			INT iHash = (Pairs(i).HashValue & HashMask);
			Pairs(i).HashNext = NewHash[iHash];
			NewHash[iHash] = i + 1;
		}
		if (Hash)
			delete[] Hash;
		Hash = NewHash;
		unguardSlow;
	}
	inline void Relax()
	{
		guardSlow(FScriptMapBase::Relax);
		while (HashCount > Pairs.Num() * 2 + 8)
			HashCount = HashCount >> 1;
		Rehash();
		unguardSlow;
	}
	inline void* Add(BYTE* InKey, BYTE* InValue, INT iHash)
	{
		guardSlow(FScriptMapBase::Add);
		USPair& Pair = *new(Pairs) USPair(InKey, InValue, iHash);
		if (Hash)
		{
			iHash = (iHash & (HashCount - 1));
			Pair.HashNext = Hash[iHash];
			Hash[iHash] = Pairs.Num();
			if (HashCount * 2 + 8 < Pairs.Num())
			{
				HashCount = HashCount << 1;
				Rehash();
			}
		}
		else Rehash();
		return InValue;
		unguardSlow;
	}

	FScriptMapBase()
		: Hash(NULL), HashCount(0)
	{}
	FScriptMapBase(ENoInit)
		: Pairs(E_NoInit)
	{}
	~FScriptMapBase()
	{
		guardSlow(FScriptMapBase::~TMapBase);
		if (Hash)
			delete[] Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	inline INT Num() const
	{
		return Pairs.Num();
	}
	inline INT FindHash(INT iHash) const
	{
		return Hash ? Hash[iHash & (HashCount - 1)] : 0;
	}
	inline void Empty()
	{
		guardSlow(FScriptMapBase::Empty);
		Pairs.Empty();
		if (Hash)
			delete[] Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
};

template< class TK, class TI > class TScriptMap : protected FScriptMapBase
{
	friend class TIterator;
public:
	TScriptMap()
		: FScriptMapBase()
	{}
	TScriptMap(ENoInit)
		: FScriptMapBase(E_NoInit)
	{}
	~TScriptMap()
	{
		Empty();
	}
	TScriptMap& operator=(const TScriptMap& Other)
	{
		guardSlow(TScriptMap::operator=);
		Empty();
		for (INT i = 0; i < Other.Pairs.Num(); ++i)
		{
			const USPair& P = Other.Pairs(i);
			BYTE* KV = (BYTE*)appMalloc(sizeof(TK), TEXT("SKey"));
			BYTE* VV = (BYTE*)appMalloc(sizeof(TI), TEXT("SValue"));
			new (KV) TK(*((TK*)P.Key));
			new (VV) TI(*((TI*)P.Value));
			new(Pairs) USPair(KV, VV, P.HashValue);
		}
		RehashRecommended();
		return *this;
		unguardSlow;
	}
	inline void Empty()
	{
		guardSlow(TScriptMap::Empty);
		if (Pairs.Num())
		{
			for (INT i = 0; i < Pairs.Num(); i++)
			{
				((TK*)(Pairs(i).Key))->~TK();
				((TI*)(Pairs(i).Value))->~TI();
			}
			Pairs.Empty();
		}
		if (Hash)
			delete[] Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	inline TI& Set(const TK& InKey, const TI& InValue)
	{
		guardSlow(TScriptMap::Set);
		INT iHash = GetTypeHash(InKey);
		if (Hash)
		{
			USPair* P = &Pairs(-1);
			for (INT i = Hash[iHash & (HashCount - 1)]; i; i = P[i].HashNext)
				if (*((TK*)P[i].Key) == InKey)
				{
					*((TI*)P[i].Value) = InValue;
					return *(TI*)P[i].Value;
				}
		}

		BYTE* KV = (BYTE*)appMalloc(sizeof(TK), TEXT("SKey"));
		BYTE* VV = (BYTE*)appMalloc(sizeof(TI), TEXT("SValue"));
		new (KV) TK(InKey);
		new (VV) TI(InValue);
		return *((TI*)Add(KV, VV, iHash));
		unguardSlow;
	}
	inline BYTE Remove(const TK& InKey)
	{
		guardSlow(TScriptMap::Remove);
		if (Hash)
		{
			USPair* P = &Pairs(-1);
			for (INT i = Hash[GetTypeHash(InKey) & (HashCount - 1)]; i; i = P[i].HashNext)
			{
				if (*((TK*)P[i].Key) == InKey)
				{
					((TK*)(Pairs(i).Key))->~TK();
					((TI*)(Pairs(i).Value))->~TI();
					Pairs.Remove(i);
					Relax();
					return 1;
				}
			}
		}
		return 0;
		unguardSlow;
	}
	inline TI* Find(const TK& Key)
	{
		guardSlow(TScriptMap::Find);
		if (Hash)
		{
			USPair* P = &Pairs(-1);
			for (INT i = Hash[GetTypeHash(Key) & (HashCount - 1)]; i; i = P[i].HashNext)
				if (*((TK*)P[i].Key) == Key)
					return (TI*)P[i].Value;
		}
		return NULL;
		unguardSlow;
	}
	inline const TI* Find(const TK& Key) const
	{
		guardSlow(TScriptMap::Find);
		if (Hash)
		{
			const USPair* P = &Pairs(-1);
			for (INT i = Hash[GetTypeHash(Key) & (HashCount - 1)]; i; i = P[i].HashNext)
				if (*((TK*)P[i].Key) == Key)
					return (TI*)P[i].Value;
		}
		return NULL;
		unguardSlow;
	}
	inline TI FindRef(const TK& Key)
	{
		guardSlow(TScriptMap::Find);
		if (Hash)
		{
			USPair* P = &Pairs(-1);
			for (INT i = Hash[GetTypeHash(Key) & (HashCount - 1)]; i; i = P[i].HashNext)
				if (*((TK*)P[i].Key) == Key)
					return *(TI*)P[i].Value;
		}
		return NULL;
		unguardSlow;
	}
	inline const TI FindRef(const TK& Key) const
	{
		guardSlow(TScriptMap::Find);
		if (Hash)
		{
			const USPair* P = &Pairs(-1);
			for (INT i = Hash[GetTypeHash(Key) & (HashCount - 1)]; i; i = P[i].HashNext)
				if (*((TK*)P[i].Key) == Key)
					return *(TI*)P[i].Value;
		}
		return NULL;
		unguardSlow;
	}
	friend FArchive& operator<<(FArchive& Ar, TScriptMap& M)
	{
		guardSlow(TScriptMap::Serialize);
		INT Count = M.Pairs.Num();
		Ar << AR_INDEX(Count);
		if (Ar.IsLoading())
		{
			TK* SerKey;
			TI* SerValue;
			if (M.Pairs.Num())
			{
				for (INT i = 0; i < M.Pairs.Num(); i++)
				{
					((TK*)(M.Pairs(i).Key))->~TK();
					((TI*)(M.Pairs(i).Value))->~TI();
				}
			}
			M.Pairs.Empty(Count);
			for (INT i = 0; i < Count; ++i)
			{
				SerKey = new TK;
				SerValue = new TI;
				Ar << *SerKey;
				Ar << *SerValue;
				new(M.Pairs) USPair((BYTE*)SerKey, (BYTE*)SerValue, GetTypeHash(*SerKey));
			}
			M.RehashRecommended();
		}
		else
		{
			for (INT i = 0; i < M.Pairs.Num(); ++i)
			{
				Ar << *((TK*)M.Pairs(i).Key);
				Ar << *((TI*)M.Pairs(i).Value);
			}
		}
		return Ar;
		unguardSlow;
	}
	inline INT Num() const
	{
		return Pairs.Num();
	}
	void Dump() const
	{
		INT i;
		debugf(TEXT("HashCount %i Pairs %i"), HashCount, Pairs.Num());
		for (i = 0; i < HashCount; ++i)
		{
			debugf(TEXT("Hash[%i] = %i"), i, Hash[i]);
		}
		for (i = 0; i < Pairs.Num(); ++i)
		{
			debugf(TEXT("Pairs[%i] = (Hash %i Next %i)"), i, Pairs(i).HashValue, Pairs(i).HashNext);
		}
	}

	class TIterator
	{
	public:
		TIterator(TScriptMap<TK, TI>& InMap) : Pairs(InMap.Pairs), Index(0) {}
		inline void operator++() { ++Index; }
		inline void RemoveCurrent() { Pairs.Remove(Index--); }
		inline operator UBOOL() const { return Index < Pairs.Num(); }
		inline TK& Key() const { return *((TK*)Pairs(Index).Key); }
		inline TI& Value() const { return *((TI*)Pairs(Index).Value); }
	private:
		TArray<USPair>& Pairs;
		INT Index;
	};
};

template< class TK, class TI > class TScriptMapNoInit : public TScriptMap<TK, TI>
{
public:
	TScriptMapNoInit()
		: TScriptMap<TK,TI>(E_NoInit)
	{}
};

//
// Maps unique keys to values.
//
template< class TK, class TI > class TMapBase
{
protected:
	class TPair
	{
	public:
		INT HashNext;
		TK Key;
		TI Value;
		TPair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
		: Key( InKey ), Value( InValue )
		{}
		TPair()
		{}
		friend FArchive& operator<<( FArchive& Ar, TPair& F )
		{
			guardSlow(TMapBase::TPair<<);
			return Ar << F.Key << F.Value;
			unguardSlow;
		}
	};
	void Rehash()
	{
		guardSlow(TMapBase::Rehash);
		if (HashCount)
		{
			INT i;
			INT* NewHash = NewTagged(TEXT("HashMapHash"))INT[HashCount];
			for (i = 0; i < HashCount; i++)
			{
				NewHash[i] = INDEX_NONE;
			}
			for (i = 0; i < Pairs.Num(); i++)
			{
				TPair& Pair = Pairs(i);
				INT    iHash = (GetTypeHash(Pair.Key) & (HashCount - 1));
				Pair.HashNext = NewHash[iHash];
				NewHash[iHash] = i;
			}
			if (Hash)
				delete Hash;
			Hash = NewHash;
		}
		else Hash = NULL;
		unguardSlow;
	}
	void Relax()
	{
		guardSlow(TMapBase::Relax);
		while( HashCount>Pairs.Num()*2+8 )
			HashCount /= 2;
		Rehash();
		unguardSlow;
	}
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		guardSlow(TMapBase::Add);
		TPair& Pair   = *new(Pairs)TPair( InKey, InValue );
		INT    iHash  = (GetTypeHash(Pair.Key) & (HashCount-1));
		Pair.HashNext = Hash[iHash];
		Hash[iHash]   = Pairs.Num()-1;
		if( HashCount*2+8 < Pairs.Num() )
		{
			HashCount *= 2;
			Rehash();
		}
		return Pair.Value;
		unguardSlow;
	}
	TArray<TPair> Pairs;
	INT* Hash;
	INT HashCount;
public:
	TMapBase()
	:	Hash( NULL )
	,	HashCount( 0 )
	{
		guardSlow(TMapBase::TMapBase);
		unguardSlow;
	}
	TMapBase( const TMapBase& Other )
	:	Pairs( Other.Pairs )
	,	Hash( NULL )
	,	HashCount( Other.HashCount )
	{
		guardSlow(TMapBase::TMapBase copy);
		Rehash();
		unguardSlow;
	}
	~TMapBase()
	{
		guardSlow(TMapBase::~TMapBase);
		if( Hash )
			delete Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	inline void InitMap()
	{
		if (!HashCount)
		{
			HashCount = 8;
			Rehash();
		}
	}
	TMapBase& operator=( const TMapBase& Other )
	{
		guardSlow(TMapBase::operator=);
		Pairs     = Other.Pairs;
		HashCount = Other.HashCount;
		Rehash();
		return *this;
		unguardSlow;
	}
	void Empty()
	{
		guardSlow(TMapBase::Empty);
		checkSlow(!(HashCount&(HashCount-1)));
		Pairs.Empty();
		HashCount = 8;
		Rehash();
		unguardSlow;
	}
	TI& Set( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		guardSlow(TMap::Set);
		InitMap();
		for( INT i=Hash[(GetTypeHash(InKey) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==InKey )
				{Pairs(i).Value=InValue; return Pairs(i).Value;}
		return Add( InKey, InValue );
		unguardSlow;
	}
	INT Remove( typename TTypeInfo<TK>::ConstInitType InKey )
	{
		guardSlow(TMapBase::Remove);
		INT Count=0;
		for( INT i=Pairs.Num()-1; i>=0; i-- )
			if( Pairs(i).Key==InKey )
				{Pairs.Remove(i); Count++;}
		if( Count )
			Relax();
		return Count;
		unguardSlow;
	}
	TI* Find( const TK& Key )
	{
		guardSlow(TMapBase::Find);
		if (!HashCount)
			return NULL;
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	TI FindRef( const TK& Key )
	{
		guardSlow(TMapBase::Find);
		if (!HashCount)
			return NULL;
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	TK FindKey(const TI& Value)
	{
		guardSlow(TMapBase::FindKey);
		for (INT i = 0; i<Pairs.Num(); ++i)
			if (Pairs(i).Value == Value)
				return Pairs(i).Key;
		return NULL;
		unguardSlow;
	}
	const TI FindRef(const TK& Key) const
	{
		guardSlow(TMapBase::Find);
		if (!HashCount)
			return NULL;
		for (INT i = Hash[(GetTypeHash(Key) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == Key)
				return Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	const TI* Find( const TK& Key ) const
	{
		guardSlow(TMapBase::Find);
		if (!HashCount)
			return NULL;
		for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			if( Pairs(i).Key==Key )
				return &Pairs(i).Value;
		return NULL;
		unguardSlow;
	}
	friend FArchive& operator<<( FArchive& Ar, TMapBase& M )
	{
		guardSlow(TMapBase<<);
		Ar << M.Pairs;
		if( Ar.IsLoading() )
			M.Rehash();
		return Ar;
		unguardSlow;
	}
	void Dump( FOutputDevice& Ar )
	{
#if 1
		// Give some more useful statistics. --han
		INT NonEmpty = 0, Worst = 0;
		for( INT i=0; i<HashCount; i++ )
		{
			INT c=0;
			for( INT j=Hash[i]; j!=INDEX_NONE; j=Pairs(j).HashNext )
				c++;
			if ( c>Worst )
				Worst = c;
			if ( c>0 )
			{
				NonEmpty++;
				Ar.Logf( TEXT("   Hash[%i] = %i"), i, c );
			}
		}
		Ar.Logf( TEXT("TMapBase: %i items, worst %i, %i/%i hash slots used."), Pairs.Num(), Worst, NonEmpty, HashCount );
#else
		// oldver.
		Ar.Logf( TEXT("TMapBase: %i items, %i hash slots"), Pairs.Num(), HashCount );
		for( INT i=0; i<HashCount; i++ )
		{
			INT c=0;
			for( INT j=Hash[i]; j!=INDEX_NONE; j=Pairs(j).HashNext )
				c++;
			Ar.Logf( TEXT("   Hash[%i] = %i"), i, c );
		}
#endif
	}
	class TIterator
	{
	public:
		TIterator(TMapBase<TK, TI>& InMap) : Pairs(InMap.Pairs), Index(0) {}
		void operator++() { ++Index; }
		void Increment() { ++Index; }
		void RemoveCurrent() { Pairs.Remove(Index--); }
		operator UBOOL() const { return Index < Pairs.Num(); }
		TK& Key() const { return Pairs(Index).Key; }
		TI& Value() const { return Pairs(Index).Value; }
	private:
		TArray<TPair>& Pairs;
		INT Index;
		friend class TUniqueKeyIterator;
	};
	class TUniqueKeyIterator : public TMapBase<TK, TI>::TIterator
	{
	public:
		TUniqueKeyIterator(TMapBase<TK, TI>& InMap)
			: TMapBase<TK, TI>::TIterator(InMap)
		{
			if (*this)
				VisitedKeys.AddUniqueItem(TMapBase<TK, TI>::TIterator::Key());
		}
		void operator++()
		{
			while (1)
			{
				TMapBase<TK, TI>::TIterator::Increment();

				if (!*this)
					return;

				INT OldNum = VisitedKeys.Num();
				VisitedKeys.AddUniqueItem(TMapBase<TK, TI>::TIterator::Key());
				if (OldNum != VisitedKeys.Num())
					return;
			}
		}
	private:
		TArray<TK> VisitedKeys;
	};
	friend class TUniqueKeyIterator;
	friend class TIterator;
};
template< class TK, class TI > class TMap : public TMapBase<TK,TI>
{
public:
	TMap& operator=( const TMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	inline int Num() const
	{
		guardSlow(TMap::Num);
		return Pairs.Num();
		unguardSlow;
	}
};
template< class TK, class TI > class TMultiMap : public TMapBase<TK,TI>
{
public:
	TMultiMap& operator=( const TMultiMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	int Num()
	{
		guardSlow(TMap::Num);
		return this->Pairs.Num();
		unguardSlow;
	}

	int Num(TArray<TK>& Keys)
	{
		guardSlow(TMultiMap::Num);
		Keys.Empty();

		for( INT i=0;i < this->Pairs.Num();i++ )
		{
			INT j=Keys.FindItemIndex(this->Pairs(i).Key);
			if (j==INDEX_NONE)
			{
				j=Keys.AddZeroed();
				Keys(j)=this->Pairs(i).Key;
			}
		}
		return Keys.Num();
		unguardSlow;
	}

	inline INT Count(const TK& Key)
	{
		guardSlow(TMultiMap::Count);
		if (!HashCount)
			return 0;
		INT Result = 0;
		for (INT i = this->Hash[(GetTypeHash(Key) & (this->HashCount - 1))]; i != INDEX_NONE; i = this->Pairs(i).HashNext)
			if (this->Pairs(i).Key == Key)
				++Result;
		return Result;
		unguardSlow;
	}

	void MultiFind( const TK& Key, TArray<TI>& Values )
	{
		guardSlow(TMap::MultiFind);
		if (HashCount)
		{
			for (INT i = this->Hash[(GetTypeHash(Key) & (this->HashCount - 1))]; i != INDEX_NONE; i = this->Pairs(i).HashNext)
				if (this->Pairs(i).Key == Key)
					new(Values)TI(this->Pairs(i).Value);
		}
		unguardSlow;
	}
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		InitMap();
		return TMapBase<TK,TI>::Add( InKey, InValue );
	}
	TI& AddUnique( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		InitMap();
		for( INT i=this->Hash[(GetTypeHash(InKey) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			if( this->Pairs(i).Key==InKey && this->Pairs(i).Value==InValue )
				return this->Pairs(i).Value;
		return Add( InKey, InValue );
	}
	INT MultiRemove( const TK& InKey )
	{
		guardSlow(TMap::RemoveMulti);
		INT Count = 0;
		for ( INT i=this->Pairs.Num()-1; i>=0; i-- )
			if ( this->Pairs(i).Key == InKey )
				{this->Pairs.Remove(i); Count++;}
		if ( Count )
			this->Relax();
		return Count;
		unguardSlow;
	}
	INT RemovePair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		guardSlow(TMap::Remove);
		INT Count=0;
		for( INT i=this->Pairs.Num()-1; i>=0; i-- )
			if( this->Pairs(i).Key==InKey && this->Pairs(i).Value==InValue )
				{this->Pairs.Remove(i); Count++;}
		if( Count )
			this->Relax();
		return Count;
		unguardSlow;
	}
	TI* FindPair( const TK& Key, const TK& Value )
	{
		guardSlow(TMap::Find);
		if (HashCount)
		{
			for (INT i = this->Hash[(GetTypeHash(Key) & (this->HashCount - 1))]; i != INDEX_NONE; i = this->Pairs(i).HashNext)
				if (this->Pairs(i).Key == Key && this->Pairs(i).Value == Value)
					return &this->Pairs(i).Value;
		}
		return NULL;
		unguardSlow;
	}
};

// Single key map, written by Marco
template< class TK > class TSingleMap
{
protected:
	class TValue
	{
	public:
		INT HashNext;
		TK Key;
		TValue(typename TTypeInfo<TK>::ConstInitType InKey)
			: Key(InKey)
		{}
		TValue()
		{}
		friend FArchive& operator<<(FArchive& Ar, TValue& F)
		{
			guardSlow(TSingleMap::TValue << );
			return Ar << F.Key;
			unguardSlow;
		}
	};
	void Rehash()
	{
		guardSlow(TSingleMap::Rehash);
		checkSlow(!(HashCount & (HashCount - 1)));
		checkSlow(HashCount >= 8);
		INT* NewHash = NewTagged(TEXT("SHashMapHash"))INT[HashCount];
		{for (INT i = 0; i < HashCount; i++)
		{
			NewHash[i] = INDEX_NONE;
		}}
		{for (INT i = 0; i < Pairs.Num(); i++)
		{
			TValue& Pair = Pairs(i);
			INT    iHash = (GetTypeHash(Pair.Key) & (HashCount - 1));
			Pair.HashNext = NewHash[iHash];
			NewHash[iHash] = i;
		}}
		if (Hash)
			delete[] Hash;
		Hash = NewHash;
		unguardSlow;
	}
	void Relax()
	{
		guardSlow(TSingleMap::Relax);
		while (HashCount > Pairs.Num() * 2 + 8)
			HashCount /= 2;
		Rehash();
		unguardSlow;
	}
	void Add(typename TTypeInfo<TK>::ConstInitType InKey)
	{
		guardSlow(TSingleMap::Add);
		TValue& Pair = *new(Pairs)TValue(InKey);
		INT    iHash = (GetTypeHash(Pair.Key) & (HashCount - 1));
		Pair.HashNext = Hash[iHash];
		Hash[iHash] = Pairs.Num() - 1;
		if (HashCount * 2 + 8 < Pairs.Num())
		{
			HashCount *= 2;
			Rehash();
		}
		unguardSlow;
	}
	TArray<TValue> Pairs;
	INT* Hash;
	INT HashCount;
public:
	TSingleMap()
		: Hash(NULL)
		, HashCount(8)
	{
		guardSlow(TSingleMap::TMapBase);
		Rehash();
		unguardSlow;
	}
	TSingleMap(const TSingleMap& Other)
		: Pairs(Other.Pairs)
		, Hash(NULL)
		, HashCount(Other.HashCount)
	{
		guardSlow(TSingleMap::TMapBase copy);
		Rehash();
		unguardSlow;
	}
	~TSingleMap()
	{
		guardSlow(TSingleMap::~TMapBase);
		if (Hash)
			delete[] Hash;
		Hash = NULL;
		HashCount = 0;
		unguardSlow;
	}
	TSingleMap& operator=(const TSingleMap& Other)
	{
		guardSlow(TSingleMap::operator=);
		Pairs = Other.Pairs;
		HashCount = Other.HashCount;
		Rehash();
		return *this;
		unguardSlow;
	}
	void Empty()
	{
		guardSlow(TSingleMap::Empty);
		checkSlow(!(HashCount & (HashCount - 1)));
		Pairs.Empty();
		HashCount = 8;
		Rehash();
		unguardSlow;
	}
	UBOOL Set(typename TTypeInfo<TK>::ConstInitType InKey)
	{
		guardSlow(TSingleMap::Set);
		for (INT i = Hash[(GetTypeHash(InKey) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == InKey)
				return 0;
		Add(InKey);
		return 1;
		unguardSlow;
	}
	UBOOL Remove(typename TTypeInfo<TK>::ConstInitType InKey)
	{
		guardSlow(TSingleMap::Remove);
		for (INT i = Hash[(GetTypeHash(InKey) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == InKey)
			{
				Pairs.Remove(i);
				Relax();
				return 1;
			}
		return 0;
		unguardSlow;
	}
	UBOOL Find(const TK& Key) const
	{
		guardSlow(TSingleMap::Find);
		for (INT i = Hash[(GetTypeHash(Key) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == Key)
				return 1;
		return 0;
		unguardSlow;
	}
	INT FindIndex(const TK& Key) const
	{
		guardSlow(TSingleMap::FindIndex);
		for (INT i = Hash[(GetTypeHash(Key) & (HashCount - 1))]; i != INDEX_NONE; i = Pairs(i).HashNext)
			if (Pairs(i).Key == Key)
				return i;
		return INDEX_NONE;
		unguardSlow;
	}
	inline INT Num() const
	{
		guardSlow(TSingleMap::Remove);
		return Pairs.Num();
		unguardSlow;
	}
	friend FArchive& operator<<(FArchive& Ar, TSingleMap& M)
	{
		guardSlow(TSingleMap << );
		Ar << M.Pairs;
		if (Ar.IsLoading())
			M.Rehash();
		return Ar;
		unguardSlow;
	}
	class TIterator
	{
	public:
		TIterator(TSingleMap& InMap) : MP(InMap), Index(0), bDeleted(FALSE) {}
		~TIterator() { if (bDeleted) MP.Relax(); }
		void operator++() { ++Index; }
		void RemoveCurrent() { MP.Pairs.Remove(Index--); bDeleted = TRUE; }
		operator UBOOL() const { return Index < MP.Pairs.Num(); }
		TK& Key() const { return MP.Pairs(Index).Key; }
	private:
		TSingleMap& MP;
		INT Index;
		UBOOL bDeleted;
	};
	friend class TIterator;
};

/*----------------------------------------------------------------------------
	Sorting template.
----------------------------------------------------------------------------*/

//
// Sort elements. The sort is unstable, meaning that the ordering of equal
// items is not necessarily preserved.
//
template<class T> struct TStack
{
	T* Min;
	T* Max;
};
template<class T> void Sort( T* First, INT Num )
{
	guard(Sort);
	if( Num<2 )
		return;
	TStack<T> RecursionStack[32]={{First,First+Num-1}}, Current, Inner;
	for( TStack<T>* StackTop=RecursionStack; StackTop>=RecursionStack; --StackTop )
	{
		Current = *StackTop;
	Loop:
		INT Count = Current.Max - Current.Min + 1;
		if( Count <= 8 )
		{
			// Use simple bubble-sort.
			while( Current.Max > Current.Min )
			{
				T *Max, *Item;
				for( Max=Current.Min, Item=Current.Min+1; Item<=Current.Max; Item++ )
					if( Compare(*Item, *Max) > 0 )
						Max = Item;
				Exchange( *Max, *Current.Max-- );
			}
		}
		else
		{
			// Grab middle element so sort doesn't exhibit worst-cast behaviour with presorted lists.
			Exchange( Current.Min[Count/2], Current.Min[0] );

			// Divide list into two halves, one with items <=Current.Min, the other with items >Current.Max.
			Inner.Min = Current.Min;
			Inner.Max = Current.Max+1;
			for( ; ; )
			{
				while( ++Inner.Min<=Current.Max && Compare(*Inner.Min, *Current.Min) <= 0 );
				while( --Inner.Max> Current.Min && Compare(*Inner.Max, *Current.Min) >= 0 );
				if( Inner.Min>Inner.Max )
					break;
				Exchange( *Inner.Min, *Inner.Max );
			}
			Exchange( *Current.Min, *Inner.Max );

			// Save big half and recurse with small half.
			if( Inner.Max-1-Current.Min >= Current.Max-Inner.Min )
			{
				if( Current.Min+1 < Inner.Max )
				{
					StackTop->Min = Current.Min;
					StackTop->Max = Inner.Max - 1;
					StackTop++;
				}
				if( Current.Max>Inner.Min )
				{
					Current.Min = Inner.Min;
					goto Loop;
				}
			}
			else
			{
				if( Current.Max>Inner.Min )
				{
					StackTop->Min = Inner  .Min;
					StackTop->Max = Current.Max;
					StackTop++;
				}
				if( Current.Min+1<Inner.Max )
				{
					Current.Max = Inner.Max - 1;
					goto Loop;
				}
			}
		}
	}
	unguard;
}

/*----------------------------------------------------------------------------
	TDoubleLinkedList.
----------------------------------------------------------------------------*/

//
// Simple double-linked list template.
//
template<class ElementType> class TDoubleLinkedList
{
public:

	class TIterator;
	class TDoubleLinkedListNode
	{
	public:
		friend class TDoubleLinkedList;
		friend class TIterator;

		/** Constructor */
		TDoubleLinkedListNode(const ElementType& InValue)
			: Value(InValue), NextNode(NULL), PrevNode(NULL)
		{
		}

		const ElementType& GetValue() const
		{
			return Value;
		}

		ElementType& GetValue()
		{
			return Value;
		}

		TDoubleLinkedListNode* GetNextNode()
		{
			return NextNode;
		}

		TDoubleLinkedListNode* GetPrevNode()
		{
			return PrevNode;
		}

	protected:
		ElementType				Value;
		TDoubleLinkedListNode* NextNode;
		TDoubleLinkedListNode* PrevNode;
	};

	/**
	 * Used to iterate over the elements of a linked list.
	 */
	class TIterator
	{
	private:
		// private class for safe bool conversion
		struct PrivateBooleanHelper { INT Value; };

	public:

		TIterator(TDoubleLinkedListNode* StartingNode)
			: CurrentNode(StartingNode)
		{}

		/** conversion to "bool" returning TRUE if the iterator is valid. */
		typedef bool PrivateBooleanType;
		operator PrivateBooleanType() const { return CurrentNode != NULL ? &PrivateBooleanHelper::Value : NULL; }
		bool operator !() const { return !operator PrivateBooleanType(); }

		TIterator& operator++()
		{
			checkSlow(CurrentNode);
			CurrentNode = CurrentNode->NextNode;
			return *this;
		}

		TIterator operator++(int)
		{
			TIterator Tmp(*this);
			++(*this);
			return Tmp;
		}

		TIterator& operator--()
		{
			checkSlow(CurrentNode);
			CurrentNode = CurrentNode->PrevNode;
			return *this;
		}

		TIterator operator--(int)
		{
			TIterator Tmp(*this);
			--(*this);
			return Tmp;
		}

		// Accessors.
		ElementType& operator->() const
		{
			checkSlow(CurrentNode);
			return CurrentNode->GetValue();
		}
		ElementType& operator*() const
		{
			checkSlow(CurrentNode);
			return CurrentNode->GetValue();
		}
		TDoubleLinkedListNode* GetNode() const
		{
			checkSlow(CurrentNode);
			return CurrentNode;
		}

	private:

		TDoubleLinkedListNode* CurrentNode;
	};

	/** Constructors. */
	TDoubleLinkedList() : HeadNode(NULL), TailNode(NULL), ListSize(0)
	{}

	/** Destructor */
	virtual ~TDoubleLinkedList()
	{
		Clear();
	}

	// Adding/Removing methods
	/**
	 * Add the specified value to the beginning of the list, making that value the new head of the list.
	 *
	 * @param	InElement	the value to add to the list.
	 *
	 * @return	whether the node was successfully added into the list
	 */
	UBOOL AddHead(const ElementType& InElement)
	{
		TDoubleLinkedListNode* NewNode = new TDoubleLinkedListNode(InElement);
		if (NewNode == NULL)
		{
			return FALSE;
		}

		// have an existing head node - change the head node to point to this one
		if (HeadNode != NULL)
		{
			NewNode->NextNode = HeadNode;
			HeadNode->PrevNode = NewNode;
			HeadNode = NewNode;
		}
		else
		{
			HeadNode = TailNode = NewNode;
		}

		SetListSize(ListSize + 1);
		return TRUE;
	}

	/**
	 * Append the specified value to the end of the list
	 *
	 * @param	InElement	the value to add to the list.
	 *
	 * @return	whether the node was successfully added into the list
	 */
	UBOOL AddTail(const ElementType& InElement)
	{
		TDoubleLinkedListNode* NewNode = new TDoubleLinkedListNode(InElement);
		if (NewNode == NULL)
		{
			return FALSE;
		}

		if (TailNode != NULL)
		{
			TailNode->NextNode = NewNode;
			NewNode->PrevNode = TailNode;
			TailNode = NewNode;
		}
		else
		{
			HeadNode = TailNode = NewNode;
		}

		SetListSize(ListSize + 1);
		return TRUE;
	}

	/**
	 * Insert the specified value into the list at an arbitrary point.
	 *
	 * @param	InElement			the value to insert into the list
	 * @param	NodeToInsertBefore	the new node will be inserted into the list at the current location of this node
	 *								if NULL, the new node will become the new head of the list
	 *
	 * @return	whether the node was successfully added into the list
	 */
	UBOOL InsertNode(const ElementType& InElement, TDoubleLinkedListNode* NodeToInsertBefore = NULL)
	{
		if (NodeToInsertBefore == NULL || NodeToInsertBefore == HeadNode)
		{
			return AddHead(InElement);
		}

		TDoubleLinkedListNode* NewNode = new TDoubleLinkedListNode(InElement);
		if (NewNode == NULL)
		{
			return FALSE;
		}

		NewNode->PrevNode = NodeToInsertBefore->PrevNode;
		NewNode->NextNode = NodeToInsertBefore;

		NodeToInsertBefore->PrevNode->NextNode = NewNode;
		NodeToInsertBefore->PrevNode = NewNode;

		SetListSize(ListSize + 1);
		return TRUE;
	}

	/**
	 * Remove the node corresponding to InElement
	 *
	 * @param	InElement	the value to remove from the list
	 */
	void RemoveNode(const ElementType& InElement)
	{
		TDoubleLinkedListNode* ExistingNode = FindNode(InElement);
		RemoveNode(ExistingNode);
	}

	/**
	 * Removes the node specified.
	 */
	void RemoveNode(TDoubleLinkedListNode* NodeToRemove)
	{
		if (NodeToRemove != NULL)
		{
			// if we only have one node, just call Clear() so that we don't have to do lots of extra checks in the code below
			if (Num() == 1)
			{
				checkSlow(NodeToRemove == HeadNode);
				Clear();
				return;
			}

			if (NodeToRemove == HeadNode)
			{
				HeadNode = HeadNode->NextNode;
				HeadNode->PrevNode = NULL;
			}

			else if (NodeToRemove == TailNode)
			{
				TailNode = TailNode->PrevNode;
				TailNode->NextNode = NULL;
			}
			else
			{
				NodeToRemove->NextNode->PrevNode = NodeToRemove->PrevNode;
				NodeToRemove->PrevNode->NextNode = NodeToRemove->NextNode;
			}

			delete NodeToRemove;
			NodeToRemove = NULL;
			SetListSize(ListSize - 1);
		}
	}

	/**
	 * Removes all nodes from the list.
	 */
	void Clear()
	{
		TDoubleLinkedListNode* pNode;
		while (HeadNode != NULL)
		{
			pNode = HeadNode->NextNode;
			delete HeadNode;
			HeadNode = pNode;
		}

		HeadNode = TailNode = NULL;
		SetListSize(0);
	}

	// Accessors.

	/**
	 * Returns the node at the head of the list
	 */
	TDoubleLinkedListNode* GetHead() const
	{
		return HeadNode;
	}

	/**
	 * Returns the node at the end of the list
	 */
	TDoubleLinkedListNode* GetTail() const
	{
		return TailNode;
	}

	/**
	 * Finds the node corresponding to the value specified
	 *
	 * @param	InElement	the value to find
	 *
	 * @return	a pointer to the node that contains the value specified, or NULL of the value couldn't be found
	 */
	TDoubleLinkedListNode* FindNode(const ElementType& InElement)
	{
		TDoubleLinkedListNode* pNode = HeadNode;
		while (pNode != NULL)
		{
			if (pNode->GetValue() == InElement)
			{
				break;
			}

			pNode = pNode->NextNode;
		}

		return pNode;
	}

	/**
	 * Returns the size of the list.
	 */
	INT Num()
	{
		return ListSize;
	}

protected:

	/**
	 * Updates the size reported by Num().  Child classes can use this function to conveniently
	 * hook into list additions/removals.
	 *
	 * @param	NewListSize		the new size for this list
	 */
	virtual void SetListSize(INT NewListSize)
	{
		ListSize = NewListSize;
	}

private:
	TDoubleLinkedListNode* HeadNode;
	TDoubleLinkedListNode* TailNode;
	INT ListSize;
};

/*----------------------------------------------------------------------------
	TList.
----------------------------------------------------------------------------*/

//
// Simple single-linked list template.
//
template <class ElementType> class TList
{
public:

	ElementType			Element;
	TList<ElementType>*	Next;

	// Constructor.

	TList(ElementType InElement,TList<ElementType>* InNext = NULL)
	{
		Element = InElement;
		Next = InNext;
	}
};

/**
 * Wrapper around a raw pointer that destroys it automatically.
 * Calls operator delete on the object, so it must have been allocated with
 * operator new. Modeled after boost::scoped_ptr.
 *
 * If a custom deallocator is needed, this class will have to
 * expanded with a deletion policy.
 */
template<typename ReferencedType>
class TScopedPointer
{
private:

	ReferencedType* Reference;
	typedef TScopedPointer<ReferencedType> SelfType;

public:

	/** Initialization constructor. */
	TScopedPointer(ReferencedType* InReference = NULL)
		: Reference(InReference)
	{}

	/** Copy constructor. */
	TScopedPointer(const TScopedPointer& InCopy)
	{
		Reference = InCopy.Reference ?
			new ReferencedType(*InCopy.Reference) :
			NULL;
	}

	/** Destructor. */
	~TScopedPointer()
	{
		delete Reference;
		Reference = NULL;
	}

	/** Assignment operator. */
	TScopedPointer& operator=(const TScopedPointer& InCopy)
	{
		if (&InCopy != this)
		{
			delete Reference;
			Reference = InCopy.Reference ?
				new ReferencedType(*InCopy.Reference) :
				NULL;
		}
		return *this;
	}

	// Dereferencing operators.
	ReferencedType& operator*() const
	{
		check(Reference != 0);
		return *Reference;
	}

	ReferencedType* operator->() const
	{
		check(Reference != 0);
		return Reference;
	}

	ReferencedType* GetOwnedPointer() const
	{
		return Reference;
	}

	/** Returns true if the pointer is valid */
	UBOOL IsValid() const
	{
		return (Reference != 0);
	}

	// implicit conversion to the reference type.
	operator ReferencedType* () const
	{
		return Reference;
	}

	void Swap(SelfType& b)
	{
		ReferencedType* Tmp = b.Reference;
		b.Reference = Reference;
		Reference = Tmp;
	}

	/** Deletes the current pointer and sets it to a new value. */
	void Reset(ReferencedType* NewReference = NULL)
	{
		check(!Reference || Reference != NewReference);
		delete Reference;
		Reference = NewReference;
	}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar, SelfType& P)
	{
		if (Ar.IsLoading())
		{
			// When loading, allocate a new value.
			ReferencedType* OldReference = P.Reference;
			P.Reference = new ReferencedType;

			// Delete the old value.
			delete OldReference;
		}

		// Serialize the value.  The caller of this serializer is responsible to only serialize for saving non-NULL pointers. */
		check(P.Reference);
		Ar << *P.Reference;

		return Ar;
	}
};

/** specialize container traits */
/*template<typename ReferencedType>
struct TContainerTraits<TScopedPointer<ReferencedType> > : public TContainerTraitsBase<TScopedPointer<ReferencedType> >
{
	typedef ReferencedType* ConstInitType;
	typedef ReferencedType* ConstPointerType;
};*/

/** Implement movement of a scoped pointer to avoid copying the referenced value. */
template<typename ReferencedType> void Move(TScopedPointer<ReferencedType>& A, ReferencedType* B)
{
	A.Reset(B);
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
