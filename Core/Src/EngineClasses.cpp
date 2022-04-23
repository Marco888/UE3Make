
#include "CorePrivate.h"

IMPLEMENT_CLASS(AActor);
IMPLEMENT_CLASS(UFracturedStaticMesh);
IMPLEMENT_CLASS(UPlayer);
IMPLEMENT_CLASS(UNetConnection);
IMPLEMENT_CLASS(UChildConnection);
IMPLEMENT_CLASS(UClient);
IMPLEMENT_CLASS(ULevel);
IMPLEMENT_CLASS(UPendingLevel);
IMPLEMENT_CLASS(UModel);
IMPLEMENT_CLASS(UNetDriver);
IMPLEMENT_CLASS(UStaticMesh);
IMPLEMENT_CLASS(UShadowMap1D);
IMPLEMENT_CLASS(UActorComponent);
IMPLEMENT_CLASS(ULightComponent);
IMPLEMENT_CLASS(UDirectionalLightComponent);
IMPLEMENT_CLASS(UDominantDirectionalLightComponent);
IMPLEMENT_CLASS(UPointLightComponent);
IMPLEMENT_CLASS(USpotLightComponent);
IMPLEMENT_CLASS(UDominantSpotLightComponent);
IMPLEMENT_CLASS(UPrimitiveComponent);
IMPLEMENT_CLASS(UMeshComponent);
IMPLEMENT_CLASS(UStaticMeshComponent);
IMPLEMENT_CLASS(UFlexComponent);
IMPLEMENT_CLASS(UFlexForceFieldComponent);
IMPLEMENT_CLASS(UAnimSequence);
IMPLEMENT_CLASS(USkeletalMesh);
IMPLEMENT_CLASS(UAnimObject);
IMPLEMENT_CLASS(USoundNode);
IMPLEMENT_CLASS(UCodecMovie);
IMPLEMENT_CLASS(UCodecMovieBink);
IMPLEMENT_CLASS(USurface);
IMPLEMENT_CLASS(UTexture);
IMPLEMENT_CLASS(UTexture2D);
IMPLEMENT_CLASS(UTexture2DComposite);
IMPLEMENT_CLASS(UTexture2DDynamic);
IMPLEMENT_CLASS(UTextureFlipBook);
IMPLEMENT_CLASS(UTextureCube);
IMPLEMENT_CLASS(UTextureMovie);
IMPLEMENT_CLASS(UTextureRenderTarget);
IMPLEMENT_CLASS(UTextureRenderTargetCube);
IMPLEMENT_CLASS(UTextureRenderTarget2D);
IMPLEMENT_CLASS(UMaterialInterface);
IMPLEMENT_CLASS(UMaterial);
IMPLEMENT_CLASS(UObjectRedirector);

void UFracturedStaticMesh::StaticConstructor()
{
	guard(UFracturedStaticMesh::StaticConstructor);
	UArrayProperty* Arr = new(GetClass(), TEXT("FragmentDestroyEffects"), 0)UArrayProperty(EC_CppProperty, 0, TEXT(""), 0);
	Arr->Inner = new(Arr, TEXT("ObjectProperty0"), 0)UObjectProperty(EC_CppProperty, 0, TEXT(""), 0, UObject::StaticClass());
	new(GetClass(), TEXT("bAlwaysBreakOffIsolatedIslands"), 0)UBoolProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("bFixIsolatedChunks"), 0)UBoolProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("bSpawnPhysicsChunks"), 0)UBoolProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("ChanceOfPhysicsChunk"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("ChunkAngVel"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("ChunkLinHorizontalScale"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("ChunkLinVel"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("ExplosionPhysicsChunkScaleMax"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("ExplosionPhysicsChunkScaleMin"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("FragmentDestroyEffectScale"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("NormalPhysicsChunkScaleMax"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("NormalPhysicsChunkScaleMin"), 0)UFloatProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("OutsideMaterialIndex"), 0)UIntProperty(EC_CppProperty, 0, TEXT(""), 0);
	new(GetClass(), TEXT("LoseChunkOutsideMaterial"), 0)UObjectProperty(EC_CppProperty, 0, TEXT(""), 0, UObject::StaticClass());
	unguard;
}
void UNetConnection::StaticConstructor()
{
	guard(UNetConnection::StaticConstructor);
	UArrayProperty* Arr = new(GetClass(), TEXT("Children"), 0)UArrayProperty(EC_CppProperty, 0, TEXT(""), 0);
	Arr->Inner = new(Arr, TEXT("ObjectProperty0"), 0)UObjectProperty(EC_CppProperty, 0, TEXT(""), 0, UObject::StaticClass());
	unguard;
}
void UChildConnection::StaticConstructor()
{
	guard(UChildConnection::StaticConstructor);
	new(GetClass(), TEXT("Parent"), 0)UObjectProperty(EC_CppProperty, 0, TEXT(""), 0, UNetConnection::StaticClass());
	unguard;
}
void ULightComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}
void UDominantDirectionalLightComponent::Serialize(FArchive& Ar)
{
	if (Ar.Ver() >= VER_DOMINANTLIGHT_NORMALSHADOWS)
	{
		// Hack, to load and save zero shadowmaps.
		TArray<WORD> DominantLightShadowMap;
		Ar << DominantLightShadowMap;
	}
	Super::Serialize(Ar);
}
void UDominantSpotLightComponent::Serialize(FArchive& Ar)
{
	if (Ar.Ver() >= VER_SPOTLIGHT_DOMINANTSHADOW_TRANSITION)
	{
		// Hack, to load and save zero shadowmaps.
		TArray<WORD> DominantLightShadowMap;
		Ar << DominantLightShadowMap;
	}
	Super::Serialize(Ar);
}

// Editor:

class UPolys : public UObject
{
	DECLARE_NOINST_CLASS(UPolys, UObject, CLASS_Transient, Engine);
};
class UWorld : public UObject
{
	DECLARE_NOINST_CLASS(UWorld, UObject, CLASS_Transient, Engine);
};
class UByteCodeSerializer : public UObject
{
	DECLARE_NOINST_CLASS(UByteCodeSerializer, UObject, CLASS_Transient, UnrealEd);
};
class UTransBuffer : public UObject
{
	DECLARE_NOINST_CLASS(UTransBuffer, UObject, CLASS_Transient, UnrealEd);
};

IMPLEMENT_CLASS(UPolys);
IMPLEMENT_CLASS(UWorld);
IMPLEMENT_CLASS(UByteCodeSerializer);
IMPLEMENT_CLASS(UTransBuffer);
