/*=============================================================================
	UnMath.cpp: Unreal math routines, implementation of FGlobalMath class
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
		* FMatrix (4x4 matrices) and FQuat (quaternions) classes added - Erik de Neve
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FGlobalMath constructor.
-----------------------------------------------------------------------------*/

// Constructor.
FGlobalMath::FGlobalMath()
:	WorldMin			(-32700.f,-32700.f,-32700.f),
	WorldMax			(32700.f,32700.f,32700.f),
	UnitCoords			(FVector(0,0,0),FVector(1,0,0),FVector(0,1,0),FVector(0,0,1)),
	UnitScale			(FVector(1,1,1),0.f,SHEER_ZX),
	ViewCoords			(FVector(0,0,0),FVector(0,1,0),FVector(0,0,-1),FVector(1,0,0))
{
	// Init base angle table.
	{for( INT i=0; i<NUM_ANGLES; i++ )
		TrigFLOAT[i] = appSin((FLOAT)i * 2.f * PI / (FLOAT)NUM_ANGLES);}

	// Init square root table.
	{for( INT i=0; i<NUM_SQRTS; i++ )
		SqrtFLOAT[i] = appSqrt((FLOAT)i / 16384.f);}
}

/*-----------------------------------------------------------------------------
	Conversion functions.
-----------------------------------------------------------------------------*/

//
// Find good arbitrary axis vectors to represent U and V axes of a plane
// given just the normal.
//
void FVector::FindBestAxisVectors( FVector& Axis1, FVector& Axis2 )
{
	guard(FindBestAxisVectors);

	FLOAT NX = Abs(X);
	FLOAT NY = Abs(Y);
	FLOAT NZ = Abs(Z);

	// Find best basis vectors.
	if( NZ>NX && NZ>NY )	Axis1 = FVector(1,0,0);
	else					Axis1 = FVector(0,0,1);

	Axis1 = (Axis1 - *this * (Axis1 | *this)).SafeNormal();
	Axis2 = Axis1 ^ *this;

	unguard;
}

/*-----------------------------------------------------------------------------
	Matrix inversion.
-----------------------------------------------------------------------------*/

//
// Coordinate system inverse.
//
FCoords FCoords::Inverse() const
{
	FLOAT RDet = 1.f / FTriple( XAxis, YAxis, ZAxis );
	return FCoords
	(	-Origin.TransformVectorBy(*this)
	,	RDet * FVector
		(	(YAxis.Y * ZAxis.Z - YAxis.Z * ZAxis.Y)
		,	(ZAxis.Y * XAxis.Z - ZAxis.Z * XAxis.Y)
		,	(XAxis.Y * YAxis.Z - XAxis.Z * YAxis.Y) )
	,	RDet * FVector
		(	(YAxis.Z * ZAxis.X - ZAxis.Z * YAxis.X)
		,	(ZAxis.Z * XAxis.X - XAxis.Z * ZAxis.X)
		,	(XAxis.Z * YAxis.X - XAxis.X * YAxis.Z))
	,	RDet * FVector
		(	(YAxis.X * ZAxis.Y - YAxis.Y * ZAxis.X)
		,	(ZAxis.X * XAxis.Y - ZAxis.Y * XAxis.X)
		,	(XAxis.X * YAxis.Y - XAxis.Y * YAxis.X) )
	);
}


//
// Combine two 'pivot' transforms.
//
FCoords FCoords::ApplyPivot(const FCoords& CoordsB) const
{
	// Fast solution assuming orthogonal coordinate system
	FCoords Temp;
	Temp.Origin = CoordsB.Origin + Origin.TransformVectorBy(CoordsB);
	Temp.XAxis = CoordsB.XAxis.TransformVectorBy( *this );
	Temp.YAxis = CoordsB.YAxis.TransformVectorBy( *this );
	Temp.ZAxis = CoordsB.ZAxis.TransformVectorBy( *this );
	return Temp;
}

//
// Invert pivot transformation
//
FCoords FCoords::PivotInverse() const
{
	FCoords Temp;
	FLOAT RDet = 1.f / FTriple( XAxis, YAxis, ZAxis );

	Temp.XAxis = RDet * FVector
		(	(YAxis.Y * ZAxis.Z - YAxis.Z * ZAxis.Y)
		,	(ZAxis.Y * XAxis.Z - ZAxis.Z * XAxis.Y)
		,	(XAxis.Y * YAxis.Z - XAxis.Z * YAxis.Y));

	Temp.YAxis = RDet * FVector
		(	(YAxis.Z * ZAxis.X - ZAxis.Z * YAxis.X)
		,	(ZAxis.Z * XAxis.X - XAxis.Z * ZAxis.X)
		,	(XAxis.Z * YAxis.X - XAxis.X * YAxis.Z));

	Temp.ZAxis = RDet * FVector
		(	(YAxis.X * ZAxis.Y - YAxis.Y * ZAxis.X)
		,	(ZAxis.X * XAxis.Y - ZAxis.Y * XAxis.X)
		,	(XAxis.X * YAxis.Y - XAxis.Y * YAxis.X));

	FVector TOrigin = -Origin;
	Temp.Origin = TOrigin.TransformVectorBy(Temp);
	return Temp;
}

//
// Convert this orthogonal coordinate system to a rotation.
//
FRotator FCoords::OrthoRotation() const
{
	FRotator R
	(
		(INT)(appAtan2( XAxis.Z, appSqrt(Square(XAxis.X)+Square(XAxis.Y)) ) * 32768.f / PI),
		(INT)(appAtan2( XAxis.Y, XAxis.X                                  ) * 32768.f / PI),
		0
	);
	FCoords S = GMath.UnitCoords / R;
	R.Roll = (INT)(appAtan2( ZAxis | S.YAxis, YAxis | S.YAxis ) * 32768.f / PI);
	return R;
}

/*-----------------------------------------------------------------------------
	FSphere implementation.
-----------------------------------------------------------------------------*/

//
// Compute a bounding sphere from an array of points.
//
FSphere::FSphere( const FVector* Pts, INT Count )
: FPlane(0,0,0,0)
{
	guard(FSphere::FSphere);
	if( Count )
	{
		FVector Center = FVector(0.0,0.0,0.0);
		INT i;
		for (i = 0; i < Count; i++)
		{
			Center += Pts[i] / Count;
		}
		for (i = 0; i < Count; i++)
		{
			W = Max(W, FDistSquared(Pts[i], Center));
		}
		*this = FSphere(Center, appSqrt(W) * 1.001f);
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	FBox implementation.
-----------------------------------------------------------------------------*/

FBox::FBox( const FVector* Points, INT Count )
: Min(0,0,0), Max(0,0,0), IsValid(0)
{
	guard(FBox::FBox);
	for( INT i=0; i<Count; i++ )
		*this += Points[i];
	unguard;
}


// SafeNormal
FVector FVector::SafeNormal() const
{
	FLOAT SquareSum = X*X + Y*Y + Z*Z;
	if( SquareSum < SMALL_NUMBER )
		return FVector( 0.f, 0.f, 0.f );

	FLOAT Size = appSqrt(SquareSum);
	FLOAT Scale = 1.f/Size;
	return FVector( X*Scale, Y*Scale, Z*Scale );
}


/*-----------------------------------------------------------------------------
	FQuaternion and FMatrix support functions
-----------------------------------------------------------------------------*/

// Todo: make faster by assuming last row zero and ortho(normal) translation...
FMatrix CombineTransforms( const FMatrix& M, const FMatrix& N )
{
	FMatrix Q;
	for( int i=0; i<4; i++ ) // row
		for( int j=0; j<3; j++ ) // column
			Q.M(i,j) = M.M(i,0)*N.M(0,j) + M.M(i,1)*N.M(1,j) + M.M(i,2)*N.M(2,j) + M.M(i,3)*N.M(3,j);

	return (Q);
}

// Convert quaternion to transformation (rotation) matrix.
// Optimized code based on the Feb 98 GDMAG article by Nick Bobick.
FMatrix FQuat::FQuatToFMatrix()
{
	FMatrix M;
	FLOAT wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = X + X;  y2 = Y + Y;  z2 = Z + Z;
	xx = X * x2;   xy = X * y2;   xz = X * z2;
	yy = Y * y2;   yz = Y * z2;   zz = Z * z2;
	wx = W * x2;   wy = W * y2;   wz = W * z2;

	M.XPlane.X = 1.0f - (yy + zz);
	M.XPlane.Y = xy - wz;
	M.XPlane.Z = xz + wy;
	M.XPlane.W = 0.0f;

	M.YPlane.X = xy + wz;
	M.YPlane.Y = 1.0f - (xx + zz);
	M.YPlane.Z = yz - wx;
	M.YPlane.W = 0.0f;

	M.ZPlane.X = xz - wy;
	M.ZPlane.Y = yz + wx;
	M.ZPlane.Z = 1.0f - (xx + yy);
	M.ZPlane.W = 0.0f;

	M.WPlane.X = 0.0f;
	M.WPlane.Y = 0.0f;
	M.WPlane.Z = 0.0f;
	M.WPlane.W = 1.0f;

	return M;
};

// Matrix-to-Quaternion code.
FQuat FMatrix::FMatrixToFQuat()
{
	FQuat Q;

	INT nxt[3] = {1, 2, 0};

	FLOAT tr = M(0,0) + M(1,1) + M(2,2);

	// Check the diagonal

	if (tr > 0.f)
	{
		FLOAT s = appSqrt (tr + 1.0f);
		Q.W = s / 2.0f;
  		s = 0.5f / s;

	    Q.X = (M(1,2) - M(2,1)) * s;
	    Q.Y = (M(2,0) - M(0,2)) * s;
	    Q.Z = (M(0,1) - M(1,0)) * s;
	}
	else
	{
		// diagonal is negative
		FLOAT  q[4];

		INT i = 0;

		if (M(1,1) > M(0,0)) i = 1;
		if (M(2,2) > M(i,i)) i = 2;

		INT j = nxt[i];
		INT k = nxt[j];

		FLOAT s = appSqrt ((M(i,i) - (M(j,j) + M(k,k))) + 1.0f);

		q[i] = s * 0.5f;

		//if (s != 0.0f) s = 0.5f / s;
		if (s > DELTA) s = 0.5f / s;

		q[3] = (M(j,k) - M(k,j)) * s;
		q[j] = (M(i,j) + M(j,i)) * s;
		q[k] = (M(i,k) + M(k,i)) * s;

		Q.X = q[0];
		Q.Y = q[1];
		Q.Z = q[2];
		Q.W = q[3];
	}

	return Q;
};

inline FRotator GetRandOrientation( FVector Dir )
{
	FRotator R;
	FCoords C;

    R.Roll = appRandRange(0,65535);
	FCoords Coords = GMath.UnitCoords / R;

	C.XAxis = Coords.XAxis;
	C.YAxis = Coords.YAxis;
	C.YAxis = Coords.ZAxis;

	C /= Dir.Rotation();

	FCoords NCoords( FVector(0,0,0), C.XAxis, C.XAxis, C.XAxis );

	return NCoords.OrthoRotation();
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
