#pragma once

class FRenderCommandFence
{
public:
    /**
     * Minimal initialization constructor.
     */
    FRenderCommandFence() :
        NumPendingFences(0)
    {}
private:
    volatile UINT NumPendingFences;
};

struct FTimerData
{
    BITFIELD bLoop : 1;
    BITFIELD bPaused : 1;
    FName FuncName;
    FLOAT Rate;
    FLOAT Count;
    FLOAT TimerTimeDilation;
    class UObject* TimerObj;
};

class AActor : public UObject
{
public:
	DECLARE_CLASS(AActor, UObject, (CLASS_NativeReplication | CLASS_Abstract), Engine);
	NO_DEFAULT_CONSTRUCTOR(AActor);
};

class UFracturedStaticMesh : public UObject
{
	void StaticConstructor();

	DECLARE_NOINST_CLASS(UFracturedStaticMesh, UObject, 0, Engine);
};

class UPlayer : public UObject
{
	DECLARE_NOINST_CLASS(UPlayer, UObject, CLASS_Transient | CLASS_Config, Engine);
};

class UNetConnection : public UPlayer
{
	void StaticConstructor();

	DECLARE_NOINST_CLASS(UNetConnection, UPlayer, 0, Engine);
};
class UChildConnection : public UNetConnection
{
	void StaticConstructor();

	DECLARE_NOINST_CLASS(UChildConnection, UNetConnection, 0, Engine);
};
class UClient : public UObject
{
	DECLARE_NOINST_CLASS(UClient, UObject, 0, Engine);
};
class ULevel : public UObject
{
	DECLARE_NOINST_CLASS(ULevel, UObject, 0, Engine);
};
class UPendingLevel : public ULevel
{
	DECLARE_NOINST_CLASS(UPendingLevel, ULevel, 0, Engine);
};
class UModel : public UObject
{
	DECLARE_NOINST_CLASS(UModel, UObject, 0, Engine);
};
class UNetDriver : public UObject
{
	DECLARE_NOINST_CLASS(UNetDriver, UObject, 0, Engine);
};
class UStaticMesh : public UObject
{
	DECLARE_NOINST_CLASS(UStaticMesh, UObject, 0, Engine);
};
class UShadowMap1D : public UObject
{
	DECLARE_NOINST_CLASS(UShadowMap1D, UObject, 0, Engine);
};

/*class UAkEvent : public UObject
{
	DECLARE_CLASS(UAkEvent, UObject, 0, AkAudio);
	IMPLEMENT_EMPTY_OBJ(UAkEvent);
};*/

class UActorComponent : public UComponent
{
public:
	DECLARE_CLASS(UActorComponent, UComponent, CLASS_Abstract, Engine);
	NO_DEFAULT_CONSTRUCTOR(UActorComponent);
};

struct FLightingChannelContainer
{
	union
	{
		struct
		{
			/** Whether the lighting channel has been initialized. Used to determine whether UPrimitveComponent::Attach should set defaults. */
			BITFIELD bInitialized : 1;

			// User settable channels that are auto set and also default to true for lights.
			BITFIELD BSP : 1;
			BITFIELD Static : 1;
			BITFIELD Dynamic : 1;
			// User set channels.
			BITFIELD CompositeDynamic : 1;
			BITFIELD Skybox : 1;
			BITFIELD Unnamed_1 : 1;
			BITFIELD Unnamed_2 : 1;
			BITFIELD Unnamed_3 : 1;
			BITFIELD Unnamed_4 : 1;
			BITFIELD Unnamed_5 : 1;
			BITFIELD Unnamed_6 : 1;
			BITFIELD Cinematic_1 : 1;
			BITFIELD Cinematic_2 : 1;
			BITFIELD Cinematic_3 : 1;
			BITFIELD Cinematic_4 : 1;
			BITFIELD Cinematic_5 : 1;
			BITFIELD Cinematic_6 : 1;
			BITFIELD Cinematic_7 : 1;
			BITFIELD Cinematic_8 : 1;
			BITFIELD Cinematic_9 : 1;
			BITFIELD Cinematic_10 : 1;
			BITFIELD Gameplay_1 : 1;
			BITFIELD Gameplay_2 : 1;
			BITFIELD Gameplay_3 : 1;
			BITFIELD Gameplay_4 : 1;
			BITFIELD Crowd : 1;
		};
		DWORD Bitfield;
	};
};
struct FLightmassLightSettings
{
	FLOAT IndirectLightingScale;
	FLOAT IndirectLightingSaturation;
	FLOAT ShadowExponent;
};
struct FLightmassDirectionalLightSettings : public FLightmassLightSettings
{
	FLOAT LightSourceAngle;
};
struct FLinearColor
{
	FLOAT R, G, B, A;
};

class ULightComponent : public UActorComponent
{
public:
	void Serialize(FArchive& Ar);

	DECLARE_CLASS(ULightComponent, UActorComponent, CLASS_Abstract, Engine);
	NO_DEFAULT_CONSTRUCTOR(ULightComponent);
};

class UDirectionalLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UDirectionalLightComponent);
};

struct FDominantShadowInfo
{
	FMatrix WorldToLight;
	FMatrix LightToWorld;
	FBox LightSpaceImportanceBounds;
	INT ShadowMapSizeX;
	INT ShadowMapSizeY;
};
class UDominantDirectionalLightComponent : public UDirectionalLightComponent
{
public:
	void Serialize(FArchive& Ar);

	DECLARE_CLASS(UDominantDirectionalLightComponent, UDirectionalLightComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UDominantDirectionalLightComponent);
};

struct FLightmassPointLightSettings : public FLightmassLightSettings
{
	FLOAT LightSourceRadius;
};
class UPointLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UPointLightComponent, ULightComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UPointLightComponent);
};
class USpotLightComponent : public UPointLightComponent
{
public:
	DECLARE_CLASS(USpotLightComponent, UPointLightComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(USpotLightComponent);
};
class UDominantSpotLightComponent : public USpotLightComponent
{
public:
	void Serialize(FArchive& Ar);

	DECLARE_CLASS(UDominantSpotLightComponent, USpotLightComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UDominantSpotLightComponent);
};

class UPrimitiveComponent : public UActorComponent
{
public:
	DECLARE_CLASS(UPrimitiveComponent, UActorComponent, CLASS_Abstract, Engine);
	NO_DEFAULT_CONSTRUCTOR(UPrimitiveComponent);
};
class UMeshComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent, CLASS_Abstract, Engine);
	NO_DEFAULT_CONSTRUCTOR(UMeshComponent);
};

class UStaticMeshComponent : public UMeshComponent
{
public:
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UStaticMeshComponent);
};

class UFlexComponent : public UStaticMeshComponent
{
public:
	//DISABLE_SERIALIZATION;

	DECLARE_CLASS(UFlexComponent, UStaticMeshComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UFlexComponent);
};

class UFlexForceFieldComponent : public UPrimitiveComponent
{
public:
	DISABLE_SERIALIZATION;

	DECLARE_CLASS(UFlexForceFieldComponent, UPrimitiveComponent, 0, Engine);
	NO_DEFAULT_CONSTRUCTOR(UFlexForceFieldComponent);
};

class UAnimSequence : public UObject
{
	DECLARE_NOINST_CLASS(UAnimSequence, UObject, 0, Engine);
};

class USkeletalMesh : public UObject
{
	DECLARE_NOINST_CLASS(USkeletalMesh, UObject, 0, Engine);
};

class UAnimObject : public UObject
{
	DECLARE_NOINST_CLASS(UAnimObject, UObject, CLASS_Abstract, Engine);
};

class USoundNode : public UObject
{
	DECLARE_NOINST_CLASS(USoundNode, UObject, 0, Engine);
};

class UCodecMovie : public UObject
{
	DECLARE_NOINST_CLASS(UCodecMovie, UObject, CLASS_Transient, Engine);
};
class UCodecMovieBink : public UCodecMovie
{
	DECLARE_NOINST_CLASS(UCodecMovieBink, UCodecMovie, CLASS_Transient, Engine);
};

class USurface : public UObject
{
	DECLARE_NOINST_CLASS(USurface, UObject, CLASS_Abstract, Engine);
};
class UTexture : public USurface
{
	DECLARE_NOINST_CLASS(UTexture, USurface, CLASS_Abstract, Engine);
};
class UTexture2D : public UTexture
{
	DECLARE_NOINST_CLASS(UTexture2D, UTexture, 0, Engine);
};
class UTexture2DComposite : public UTexture
{
	DECLARE_NOINST_CLASS(UTexture2DComposite, UTexture, 0, Engine);
};
class UTexture2DDynamic : public UTexture
{
	DECLARE_NOINST_CLASS(UTexture2DDynamic, UTexture, 0, Engine);
};
class UTextureFlipBook : public UTexture2D
{
	DECLARE_NOINST_CLASS(UTextureFlipBook, UTexture2D, 0, Engine);
};
class UTextureCube : public UTexture
{
	DECLARE_NOINST_CLASS(UTextureCube, UTexture, 0, Engine);
};
class UTextureMovie : public UTexture
{
	DECLARE_NOINST_CLASS(UTextureMovie, UTexture, 0, Engine);
};
class UTextureRenderTarget : public UTexture
{
	DECLARE_NOINST_CLASS(UTextureRenderTarget, UTexture, 0, Engine);
};
class UTextureRenderTargetCube : public UTextureRenderTarget
{
	DECLARE_NOINST_CLASS(UTextureRenderTargetCube, UTextureRenderTarget, 0, Engine);
};
class UTextureRenderTarget2D : public UTextureRenderTarget
{
	DECLARE_NOINST_CLASS(UTextureRenderTarget2D, UTextureRenderTarget, 0, Engine);
};
class UMaterialInterface : public USurface
{
	DECLARE_NOINST_CLASS(UMaterialInterface, USurface, CLASS_Abstract, Engine);
};
class UMaterial : public UMaterialInterface
{
	DECLARE_NOINST_CLASS(UMaterial, UMaterialInterface, 0, Engine);
};
