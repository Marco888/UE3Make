
#include "UnEditor.h"
#include "UnLinker.h"

constexpr INT CacheVersion = 1;

static UBOOL ObjectIsAssetNamed(UObject* Obj)
{
	static TSingleMap<FName>* LookupMap = NULL;
	if (!LookupMap)
	{
		LookupMap = new TSingleMap<FName>;
		LookupMap->Set(TEXT("Font"));
		LookupMap->Set(TEXT("AnimSet"));
		LookupMap->Set(TEXT("ParticleSystem"));
		LookupMap->Set(TEXT("SwfMovie"));
		LookupMap->Set(TEXT("AkEvent"));
		LookupMap->Set(TEXT("KFPawnAnimInfo"));
		LookupMap->Set(TEXT("CameraAnim"));
		LookupMap->Set(TEXT("CameraShake"));
		LookupMap->Set(TEXT("AnimTree"));
		LookupMap->Set(TEXT("KFMuzzleFlash"));
		LookupMap->Set(TEXT("KFMusicTrackInfo"));
		LookupMap->Set(TEXT("KFImpactEffectInfo"));
		LookupMap->Set(TEXT("MaterialExpression"));
		LookupMap->Set(TEXT("PostProcessChain"));
		LookupMap->Set(TEXT("PostProcessEffect"));
		LookupMap->Set(TEXT("AnimationCompressionAlgorithm"));
		LookupMap->Set(TEXT("AkBank"));
		LookupMap->Set(TEXT("SkeletalMeshSocket"));
		LookupMap->Set(TEXT("ParticleModule"));
		LookupMap->Set(TEXT("KMeshProps"));
		LookupMap->Set(TEXT("PhysicalMaterial"));
		LookupMap->Set(TEXT("DistributionFloat"));
		LookupMap->Set(TEXT("DistributionVector"));
		LookupMap->Set(TEXT("AnimNotify"));
		LookupMap->Set(TEXT("InterpCurveEdSetup"));
		LookupMap->Set(TEXT("InterpGroup"));
		LookupMap->Set(TEXT("ParticleEmitter"));
		LookupMap->Set(TEXT("InterpTrack"));
		LookupMap->Set(TEXT("ParticleLODLevel"));
		LookupMap->Set(TEXT("ArrowComponent"));
		LookupMap->Set(TEXT("PhysicsAsset"));
		LookupMap->Set(TEXT("KFWeaponAttachment"));
		LookupMap->Set(TEXT("PhysXParticleSystem"));
		LookupMap->Set(TEXT("AnimMetaData"));
		LookupMap->Set(TEXT("MaterialFunction"));
		LookupMap->Set(TEXT("PhysicalMaterialPropertyBase"));
		LookupMap->Set(TEXT("FlexAsset"));
		LookupMap->Set(TEXT("FlexContainer"));
		LookupMap->Set(TEXT("LensFlare"));
		LookupMap->Set(TEXT("PhysicsAssetInstance"));
		LookupMap->Set(TEXT("FracturedStaticMesh"));
		LookupMap->Set(TEXT("NxForceFieldComponent"));
		LookupMap->Set(TEXT("FlexForceFieldComponent"));
		LookupMap->Set(TEXT("StaticMeshActor"));
		LookupMap->Set(TEXT("RB_BodyInstance")); 
		LookupMap->Set(TEXT("ForceFieldShape"));
		LookupMap->Set(TEXT("KFExplosionLight"));
		LookupMap->Set(TEXT("ForceFeedbackWaveform"));
		LookupMap->Set(TEXT("AkBaseSoundObject"));
		LookupMap->Set(TEXT("KFSkinTypeEffects"));
		LookupMap->Set(TEXT("Prefab"));
		LookupMap->Set(TEXT("KFLaserSightAttachment"));
		LookupMap->Set(TEXT("KFPawnSoundGroup"));
		LookupMap->Set(TEXT("UISoundTheme"));
		LookupMap->Set(TEXT("KFSprayActor"));
		LookupMap->Set(TEXT("StaticMeshComponent"));
		LookupMap->Set(TEXT("RB_ConstraintInstance"));
		LookupMap->Set(TEXT("DrawSphereComponent"));
		LookupMap->Set(TEXT("AkComponent"));
		LookupMap->Set(TEXT("SoundNodeWave"));
		LookupMap->Set(TEXT("RB_ConstraintSetup"));
		LookupMap->Set(TEXT("KFParticleSystemComponent"));
	}
	for (UClass* C = Obj->GetClass(); C; C = C->GetSuperClass())
		if (LookupMap->Find(C->GetFName()))
			return TRUE;
	return FALSE;
}
static UBOOL IsAssetObject(UObject* Obj)
{
	if (Obj->IsA(UField::StaticClass()))
		return FALSE;
	return (Obj->GetClass()==UPackage::StaticClass() || Obj->IsA(USurface::StaticClass()) || Obj->IsA(UStaticMesh::StaticClass()) || Obj->IsA(UAnimSequence::StaticClass()) || Obj->IsA(USkeletalMesh::StaticClass()) || Obj->IsA(UAnimObject::StaticClass())
		|| ObjectIsAssetNamed(Obj));
}

UBOOL PrepareAssetCache()
{
	guard(PrepareAssetCache);
	FArchive* Ar = GFileManager->CreateFileReader(TEXT("AssetCache.dat"));
	if (!Ar)
	{
		CreateAssetCache();
		return FALSE;
	}
	INT iVer;
	*Ar << iVer;
	debugf(TEXT("Loading AssetCache file (ver %i)"), iVer);
	if (iVer != CacheVersion)
	{
		delete Ar;
		GWarn->Logf(TEXT("Asset cache file version mismatch (%i vs %i)"), iVer, CacheVersion);
		return TRUE;
	}
	INT sz;
	*Ar << sz;
	for (INT i = 0; i < sz; ++i)
	{
		FString Name;
		*Ar << Name;
		UObject::CreatePackage(NULL, *Name)->bIsAssetPackage = TRUE;
	}

	delete Ar;
	return TRUE;
	unguard;
}
void CreateAssetCache()
{
	guard(CreateAssetCache);
	GWarn->Logf(NAME_Title, TEXT("Creating Asset Cache..."));
	
	TArray<FString> StockPck;
	GConfig->GetArray(TEXT("Make"), TEXT("Stock"), &StockPck);
	for (INT i = (StockPck.Num() - 1); i >= 0; --i)
	{
		GWarn->Logf(NAME_Heading, TEXT("Loading %s"), *StockPck(i));
		GUglyHackFlags |= 2;
		UObject* P = UObject::LoadPackage(NULL, *StockPck(i), LOAD_Forgiving);
		GUglyHackFlags &= ~2;
		if (!P)
			GWarn->Logf(TEXT("Couldn't load package %s"), *StockPck(i));
	}

	GWarn->Log(NAME_Heading, TEXT("Loading every reference package"));
	for (TObjectIterator<ULinkerLoad> It; It; ++It)
	{
		It->LoadAllObjects();
	}

	TArray<UObject*> WatchPck;
	for (TObjectIterator<ULinkerLoad> It; It; ++It)
	{
		if (It->LinkerRoot)
			WatchPck.AddUniqueItem(It->LinkerRoot);
	}

	GWarn->Log(NAME_Heading, TEXT("Processing packages"));
	TArray<FName> AssetPackages;
	for (INT i = 0; i < WatchPck.Num(); ++i)
	{
		UBOOL bIsAsset = TRUE;
		for (FPackageObjectIterator It(WatchPck(i)); It; ++It)
		{
			if (!IsAssetObject(*It))
			{
				GWarn->Logf(TEXT("Skipping non-asset package %ls"), WatchPck(i)->GetName());
				debugf(TEXT(" -> %ls"), It->GetFullName());
				bIsAsset = FALSE;
				break;
			}
		}
		if (bIsAsset)
			AssetPackages.AddItem(WatchPck(i)->GetFName());
	}

	GWarn->Logf(TEXT("Got %i asset packages, saving..."), AssetPackages.Num());

	FArchive* Ar = GFileManager->CreateFileWriter(TEXT("AssetCache.dat"));
	if (!Ar)
	{
		GWarn->Logf(TEXT("Failed to create AssetCache.dat file!"));
		return;
	}
	INT iVer = CacheVersion;
	*Ar << iVer;
	INT sz = AssetPackages.Num();
	*Ar << sz;
	for (INT i = 0; i < sz; ++i)
	{
		FString S(*AssetPackages(i));
		*Ar << S;
	}
	delete Ar;
	GWarn->Log(TEXT("Created AssetCache.dat!"));
	unguard;
}
