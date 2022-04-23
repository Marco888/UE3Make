/*=============================================================================
	UnObjBas.h: Unreal object base class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Core enumerations.
-----------------------------------------------------------------------------*/

#define DEBUG_OBJECTS(tg) // UObject::DEBUG_VerifyObjects(tg)

//
// Flags for loading objects.
//
enum ELoadFlags
{
	LOAD_None			= 0x0000,	// No flags.
	LOAD_NoFail			= 0x0001,	// Critical error if load fails.
	LOAD_NoWarn			= 0x0002,	// Don't display warning if load fails.
	LOAD_Throw			= 0x0008,	// Throw exceptions upon failure.
	LOAD_Verify			= 0x0010,	// Only verify existance; don't actually load.
	LOAD_AllowDll		= 0x0020,	// Allow plain DLLs.
	LOAD_DisallowFiles  = 0x0040,	// Don't load from file.
	LOAD_NoVerify       = 0x0080,   // Don't verify imports yet.
	LOAD_Forgiving      = 0x1000,   // Forgive missing imports (set them to NULL).
	LOAD_Quiet			= 0x2000,   // No log warnings.
	LOAD_NoRemap        = 0x4000,   // No remapping of packages.
	LOAD_NoPrivate		= 0x8000,   // Load package without throwing private object errors.
	LOAD_Propagate      = (LOAD_NoPrivate | LOAD_Forgiving),
};

//
// Package flags.
//
enum EPackageFlags
{
	PKG_AllowDownload = 0x00000001,	// Allow downloading package.
	PKG_ClientOptional = 0x00000002,	// Purely optional for clients.
	PKG_ServerSideOnly = 0x00000004,   // Only needed on the server side.
	PKG_Cooked = 0x00000008,	// Whether this package has been cooked for the target platform.
	PKG_Unsecure = 0x00000010,   // Not trusted.
	PKG_SavedWithNewerVersion = 0x00000020,	// Package was saved with newer version.
	PKG_Need = 0x00008000,	// Client needs to download this package.
	PKG_Compiling = 0x00010000,	// package is currently being compiled
	PKG_ContainsMap = 0x00020000,	// Set if the package contains a ULevel/ UWorld object
	PKG_Trash = 0x00040000,	// Set if the package was loaded from the trashcan
	PKG_DisallowLazyLoading = 0x00080000,	// Set if the archive serializing this package cannot use lazy loading
	PKG_PlayInEditor = 0x00100000,	// Set if the package was created for the purpose of PIE
	PKG_ContainsScript = 0x00200000,	// Package is allowed to contain UClasses and unrealscript
	PKG_ContainsDebugInfo = 0x00400000,	// Package contains debug info (for UDebugger)
	PKG_RequireImportsAlreadyLoaded = 0x00800000,	// Package requires all its imports to already have been loaded
	PKG_SelfContainedLighting = 0x01000000,	// All lighting in this package should be self contained
	PKG_StoreCompressed = 0x02000000,	// Package is being stored compressed, requires archive support for compression
	PKG_StoreFullyCompressed = 0x04000000,	// Package is serialized normally, and then fully compressed after (must be decompressed before LoadPackage is called)
	PKG_ContainsInlinedShaders = 0x08000000,	// Package was cooked allowing materials to inline their FMaterials (and hence shaders)
	PKG_ContainsFaceFXData = 0x10000000,	// Package contains FaceFX assets and/or animsets
	PKG_NoExportAllowed = 0x20000000,	// Package was NOT created by a modder.  Internal data not for export
	PKG_StrippedSource = 0x40000000,	// Source has been removed to compress the package size
};

//
// Internal enums.
//
enum ENativeConstructor    {EC_NativeConstructor};
enum EStaticConstructor    {EC_StaticConstructor};
enum EInternal             {EC_Internal};
enum ECppProperty          {EC_CppProperty};
enum EInPlaceConstructor   {EC_InPlaceConstructor};

//
// Result of GotoState.
//
enum EGotoState
{
	GOTOSTATE_NotFound		= 0,
	GOTOSTATE_Success		= 1,
	GOTOSTATE_Preempted		= 2,
};

//
// Flags describing a class.
//
enum EClassFlags : DWORD
{
	/** @name Base flags */
	//@{
	CLASS_None				  = 0x00000000,
	/** Class is abstract and can't be instantiated directly. */
	CLASS_Abstract            = 0x00000001,
	/** Script has been compiled successfully. */
	CLASS_Compiled			  = 0x00000002,
	/** Load object configuration at construction time. */
	CLASS_Config			  = 0x00000004,
	/** This object type can't be saved; null it out at save time. */
	CLASS_Transient			  = 0x00000008,
	/** Successfully parsed. */
	CLASS_Parsed              = 0x00000010,
	/** Class contains localized text. */
	CLASS_Localized           = 0x00000020,
	/** Objects of this class can be safely replaced with default or NULL. */
	CLASS_SafeReplace         = 0x00000040,
	/** Class is a native class - native interfaces will have CLASS_Native set, but not RF_Native */
	CLASS_Native			  = 0x00000080,
	/** Don't export to C++ header. */
	CLASS_NoExport            = 0x00000100,
	/** Allow users to create in the editor. */
	CLASS_Placeable           = 0x00000200,
	/** Handle object configuration on a per-object basis, rather than per-class. */
	CLASS_PerObjectConfig     = 0x00000400,
	/** Replication handled in C++. */
	CLASS_NativeReplication   = 0x00000800,
	/** Class can be constructed from editinline New button. */
	CLASS_EditInlineNew		  = 0x00001000,
	/** Display properties in the editor without using categories. */
	CLASS_CollapseCategories  = 0x00002000,
	/** Class is an interface **/
	CLASS_Interface           = 0x00004000,
	/**
	 * Indicates that this class contains object properties which are marked 'instanced' (or editinline export).  Set by the script compiler after all properties in the
	 * class have been parsed.  Used by the loading code as an optimization to attempt to instance newly added properties only for relevant classes
	 */
	CLASS_HasInstancedProps   = 0x00200000,
	/** Class needs its defaultproperties imported */
	CLASS_NeedsDefProps		  = 0x00400000,
	/** Class has component properties. */
	CLASS_HasComponents		  = 0x00800000,
	/** Don't show this class in the editor class browser or edit inline new menus. */
	CLASS_Hidden			  = 0x01000000,
	/** Don't save objects of this class when serializing */
	CLASS_Deprecated		  = 0x02000000,
	/** Class not shown in editor drop down for class selection */
	CLASS_HideDropDown		  = 0x04000000,
	/** Class has been exported to a header file */
	CLASS_Exported			  = 0x08000000,
	/** Class has no unrealscript counter-part */
	CLASS_Intrinsic			  = 0x10000000,
	/** Properties in this class can only be accessed from native code */
	CLASS_NativeOnly		  = 0x20000000,
	/** Handle object localization on a per-object basis, rather than per-class. */
	CLASS_PerObjectLocalized  = 0x40000000,
	/** This class has properties that are marked with CPF_CrossLevel */
	CLASS_HasCrossLevelRefs   = 0x80000000,

	// deprecated - these values now match the values of the EClassCastFlags enum
	/** IsA UProperty */
	CLASS_NoScriptProps			= 0x00008000,
	/** IsA UObjectProperty */
	CLASS_IsAUObjectProperty  = 0x00010000,
	/** IsA UBoolProperty */
	CLASS_IsAUBoolProperty    = 0x00020000,
	/** IsA UState */
	CLASS_IsAUState           = 0x00040000,
	/** IsA UFunction */
	CLASS_IsAUFunction        = 0x00080000,
	/** IsA UStructProperty */
	CLASS_IsAUStructProperty  = 0x00100000,

	//@}


	/** @name Flags to inherit from base class */
	//@{
	CLASS_Inherit           = CLASS_Transient | CLASS_Config | CLASS_Localized | CLASS_SafeReplace | CLASS_PerObjectConfig | CLASS_PerObjectLocalized | CLASS_Placeable
							| CLASS_HasComponents | CLASS_Deprecated | CLASS_Intrinsic | CLASS_HasInstancedProps | CLASS_HasCrossLevelRefs,

	/** these flags will be cleared by the compiler when the class is parsed during script compilation */
	CLASS_RecompilerClear   = CLASS_Inherit | CLASS_Abstract | CLASS_NoExport | CLASS_NativeReplication | CLASS_Native,

	/** these flags will be inherited from the base class only for non-intrinsic classes */
	CLASS_ScriptInherit		= CLASS_Inherit | CLASS_EditInlineNew | CLASS_CollapseCategories,
	//@}
	CLASS_SerializeFlags	= CLASS_NoScriptProps,

	CLASS_AllFlags			= 0xFFFFFFFF,
};

//
// Flags associated with each property in a class, overriding the
// property's default behavior.
//

// For compilers that don't support 64 bit enums.
#define	CPF_Edit					DECLARE_UINT64(0x0000000000000001)		// Property is user-settable in the editor.
#define	CPF_Const					DECLARE_UINT64(0x0000000000000002)		// Actor's property always matches class's default actor property.
#define CPF_Input					DECLARE_UINT64(0x0000000000000004)		// Variable is writable by the input system.
#define CPF_ExportObject			DECLARE_UINT64(0x0000000000000008)		// Object can be exported with actor.
#define CPF_OptionalParm			DECLARE_UINT64(0x0000000000000010)		// Optional parameter (if CPF_Param is set).
#define CPF_Net						DECLARE_UINT64(0x0000000000000020)		// Property is relevant to network replication.
#define CPF_EditFixedSize			DECLARE_UINT64(0x0000000000000040)		// Indicates that elements of an array can be modified, but its size cannot be changed.
#define CPF_Parm					DECLARE_UINT64(0x0000000000000080)		// Function/When call parameter.
#define CPF_OutParm					DECLARE_UINT64(0x0000000000000100)		// Value is copied out after function call.
#define CPF_SkipParm				DECLARE_UINT64(0x0000000000000200)		// Property is a short-circuitable evaluation function parm.
#define CPF_ReturnParm				DECLARE_UINT64(0x0000000000000400)		// Return value.
#define CPF_CoerceParm				DECLARE_UINT64(0x0000000000000800)		// Coerce args into this function parameter.
#define CPF_Native      			DECLARE_UINT64(0x0000000000001000)		// Property is native: C++ code is responsible for serializing it.
#define CPF_Transient   			DECLARE_UINT64(0x0000000000002000)		// Property is transient: shouldn't be saved, zero-filled at load time.
#define CPF_Config      			DECLARE_UINT64(0x0000000000004000)		// Property should be loaded/saved as permanent profile.
#define CPF_Localized   			DECLARE_UINT64(0x0000000000008000)		// Property should be loaded as localizable text.

#define CPF_EditConst   			DECLARE_UINT64(0x0000000000020000)		// Property is uneditable in the editor.
#define CPF_GlobalConfig			DECLARE_UINT64(0x0000000000040000)		// Load config from base class, not subclass.
#define CPF_Component				DECLARE_UINT64(0x0000000000080000)		// Property containts component references.
#define CPF_AlwaysInit				DECLARE_UINT64(0x0000000000100000)		// Property should never be exported as NoInit	(@todo - this doesn't need to be a property flag...only used during make)
#define CPF_DuplicateTransient		DECLARE_UINT64(0x0000000000200000)		// Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.)
#define CPF_NeedCtorLink			DECLARE_UINT64(0x0000000000400000)		// Fields need construction/destruction.
#define CPF_NoExport    			DECLARE_UINT64(0x0000000000800000)		// Property should not be exported to the native class header file.
#define	CPF_NoImport				DECLARE_UINT64(0x0000000001000000)		// Property should not be imported when creating an object from text (copy/paste)
#define CPF_NoClear					DECLARE_UINT64(0x0000000002000000)		// Hide clear (and browse) button.
#define CPF_EditInline				DECLARE_UINT64(0x0000000004000000)		// Edit this object reference inline.

#define CPF_EditInlineUse			DECLARE_UINT64(0x0000000010000000)		// EditInline with Use button.
#define CPF_Deprecated  			DECLARE_UINT64(0x0000000020000000)		// Property is deprecated.  Read it from an archive, but don't save it.
#define CPF_DataBinding				DECLARE_UINT64(0x0000000040000000)		// Indicates that this property should be exposed to data stores
#define CPF_SerializeText			DECLARE_UINT64(0x0000000080000000)		// Native property should be serialized as text (ImportText, ExportText)

#define CPF_RepNotify				DECLARE_UINT64(0x0000000100000000)		// Notify actors when a property is replicated
#define CPF_Interp					DECLARE_UINT64(0x0000000200000000)		// interpolatable property for use with matinee
#define CPF_NonTransactional		DECLARE_UINT64(0x0000000400000000)		// Property isn't transacted

#define CPF_EditorOnly				DECLARE_UINT64(0x0000000800000000)		// Property should only be loaded in the editor
#define CPF_NotForConsole			DECLARE_UINT64(0x0000001000000000)		// Property should not be loaded on console (or be a console cooker commandlet)
#define CPF_RepRetry				DECLARE_UINT64(0x0000002000000000)		// retry replication of this property if it fails to be fully sent (e.g. object references not yet available to serialize over the network)
#define CPF_PrivateWrite			DECLARE_UINT64(0x0000004000000000)		// property is const outside of the class it was declared in
#define CPF_ProtectedWrite			DECLARE_UINT64(0x0000008000000000)		// property is const outside of the class it was declared in and subclasses

//@todo ronp - temporary until we add UArchetypeProperty
#define CPF_ArchetypeProperty		DECLARE_UINT64(0x0000010000000000)		// property should be ignored by archives which have ArIgnoreArchetypeRef set

#define CPF_EditHide				DECLARE_UINT64(0x0000020000000000)		// property should never be shown in a properties window.
#define CPF_EditTextBox				DECLARE_UINT64(0x0000040000000000)		// property can be edited using a text dialog box.

#define CPF_CrossLevelPassive		DECLARE_UINT64(0x0000100000000000)		// property can point across levels, and will be serialized properly, but assumes it's target exists in-game (non-editor)
#define CPF_CrossLevelActive		DECLARE_UINT64(0x0000200000000000)		// property can point across levels, and will be serialized properly, and will be updated when the target is streamed in/out

/** @name Combinations flags */
//@{
#define	CPF_ParmFlags				(CPF_OptionalParm | CPF_Parm | CPF_OutParm | CPF_SkipParm | CPF_ReturnParm | CPF_CoerceParm)
#define	CPF_PropagateFromStruct		(CPF_Const | CPF_Native | CPF_Transient)
#define	CPF_PropagateToArrayInner	(CPF_ExportObject | CPF_EditInline | CPF_EditInlineUse | CPF_Localized | CPF_Component | CPF_Config | CPF_AlwaysInit | CPF_EditConst | CPF_Deprecated | CPF_SerializeText | CPF_CrossLevel )

/** the flags that should never be set on interface properties */
#define CPF_InterfaceClearMask		(CPF_NeedCtorLink|CPF_ExportObject)

/** a combination of both cross level types */
#define CPF_CrossLevel				(CPF_CrossLevelPassive | CPF_CrossLevelActive)	

#define CPF_AllFlags				DECLARE_UINT64(0xFFFFFFFFFFFFFFFF)

// 227j: Compile-time property flags.
enum ETempPropertyFlags
{
	TCPF_NoWarn		= 0x00000001, // Don't throw 'X' obscures 'Y' defined in base class 'Z' warnings.
	TCPF_Protected	= 0x00000002, // Variable is constant, but accessable from main class itself.
};

#define DECLARE_UINT64(x)	x

//
// Flags describing an object instance.
//
typedef QWORD EObjectFlags;													/** @warning: mirrored in UnName.h */
// For compilers that don't support 64 bit enums.
// unused							DECLARE_UINT64(0x0000000000000001)
#define RF_InSingularFunc			DECLARE_UINT64(0x0000000000000002)		// In a singular function.
#define RF_StateChanged				DECLARE_UINT64(0x0000000000000004)		// Object did a state change.
#define RF_DebugPostLoad			DECLARE_UINT64(0x0000000000000008)		// For debugging PostLoad calls.
#define RF_DebugSerialize			DECLARE_UINT64(0x0000000000000010)		// For debugging Serialize calls.
#define RF_DebugFinishDestroyed		DECLARE_UINT64(0x0000000000000020)		// For debugging FinishDestroy calls.
#define RF_EdSelected				DECLARE_UINT64(0x0000000000000040)		// Object is selected in one of the editors browser windows.
#define RF_ZombieComponent			DECLARE_UINT64(0x0000000000000080)		// This component's template was deleted, so should not be used.
#define RF_Protected				DECLARE_UINT64(0x0000000000000100)		// Property is protected (may only be accessed from its owner class or subclasses)
#define RF_ClassDefaultObject		DECLARE_UINT64(0x0000000000000200)		// this object is its class's default object
#define RF_ArchetypeObject			DECLARE_UINT64(0x0000000000000400)		// this object is a template for another object - treat like a class default object
#define RF_ForceTagExp				DECLARE_UINT64(0x0000000000000800)		// Forces this object to be put into the export table when saving a package regardless of outer
#define RF_TokenStreamAssembled		DECLARE_UINT64(0x0000000000001000)		// Set if reference token stream has already been assembled
#define RF_MisalignedObject			DECLARE_UINT64(0x0000000000002000)		// Object's size no longer matches the size of its C++ class (only used during make, for native classes whose properties have changed)
#define RF_RootSet					DECLARE_UINT64(0x0000000000004000)		// Object will not be garbage collected, even if unreferenced.
#define RF_BeginDestroyed			DECLARE_UINT64(0x0000000000008000)		// BeginDestroy has been called on the object.
#define RF_FinishDestroyed			DECLARE_UINT64(0x0000000000010000)		// FinishDestroy has been called on the object.
#define RF_DebugBeginDestroyed		DECLARE_UINT64(0x0000000000020000)		// Whether object is rooted as being part of the root set (garbage collection)
#define RF_MarkedByCooker			DECLARE_UINT64(0x0000000000040000)		// Marked by content cooker.
#define RF_LocalizedResource		DECLARE_UINT64(0x0000000000080000)		// Whether resource object is localized.
#define RF_InitializedProps			DECLARE_UINT64(0x0000000000100000)		// whether InitProperties has been called on this object
#define RF_PendingFieldPatches		DECLARE_UINT64(0x0000000000200000)		//@script patcher: indicates that this struct will receive additional member properties from the script patcher
#define RF_IsCrossLevelReferenced	DECLARE_UINT64(0x0000000000400000)		// This object has been pointed to by a cross-level reference, and therefore requires additional cleanup upon deletion
// unused							DECLARE_UINT64(0x0000000000800000)
// unused							DECLARE_UINT64(0x0000000001000000)
// unused							DECLARE_UINT64(0x0000000002000000)
// unused							DECLARE_UINT64(0x0000000004000000)
// unused							DECLARE_UINT64(0x0000000008000000)
// unused							DECLARE_UINT64(0x0000000010000000)
// unused							DECLARE_UINT64(0x0000000020000000)
// unused							DECLARE_UINT64(0x0000000040000000)
#define RF_Saved					DECLARE_UINT64(0x0000000080000000)		// Object has been saved via SavePackage. Temporary.
#define	RF_Transactional			DECLARE_UINT64(0x0000000100000000)		// Object is transactional.
#define	RF_Unreachable				DECLARE_UINT64(0x0000000200000000)		// Object is not reachable on the object graph.
#define RF_Public					DECLARE_UINT64(0x0000000400000000)		// Object is visible outside its package.
#define	RF_TagImp					DECLARE_UINT64(0x0000000800000000)		// Temporary import tag in load/save.
#define RF_TagExp					DECLARE_UINT64(0x0000001000000000)		// Temporary export tag in load/save.
#define	RF_Obsolete					DECLARE_UINT64(0x0000002000000000)		// Object marked as obsolete and should be replaced.
#define	RF_TagGarbage				DECLARE_UINT64(0x0000004000000000)		// Check during garbage collection.
#define RF_DisregardForGC			DECLARE_UINT64(0x0000008000000000)		// Object is being disregard for GC as its static and itself and all references are always loaded.
#define	RF_PerObjectLocalized		DECLARE_UINT64(0x0000010000000000)		// Object is localized by instance name, not by class.
#define RF_NeedLoad					DECLARE_UINT64(0x0000020000000000)		// During load, indicates object needs loading.
#define RF_AsyncLoading				DECLARE_UINT64(0x0000040000000000)		// Object is being asynchronously loaded.
#define	RF_NeedPostLoadSubobjects	DECLARE_UINT64(0x0000080000000000)		// During load, indicates that the object still needs to instance subobjects and fixup serialized component references
#define RF_Suppress					DECLARE_UINT64(0x0000100000000000)		// @warning: Mirrored in UnName.h. Suppressed log name.
#define RF_InEndState				DECLARE_UINT64(0x0000200000000000)		// Within an EndState call.
#define RF_Transient				DECLARE_UINT64(0x0000400000000000)		// Don't save object.
#define RF_Cooked					DECLARE_UINT64(0x0000800000000000)		// Whether the object has already been cooked
#define RF_LoadForClient			DECLARE_UINT64(0x0001000000000000)		// In-file load for client.
#define RF_LoadForServer			DECLARE_UINT64(0x0002000000000000)		// In-file load for client.
#define RF_LoadForEdit				DECLARE_UINT64(0x0004000000000000)		// In-file load for client.
#define RF_Standalone				DECLARE_UINT64(0x0008000000000000)		// Keep object around for editing even if unreferenced.
#define RF_NotForClient				DECLARE_UINT64(0x0010000000000000)		// Don't load this object for the game client.
#define RF_NotForServer				DECLARE_UINT64(0x0020000000000000)		// Don't load this object for the game server.
#define RF_NotForEdit				DECLARE_UINT64(0x0040000000000000)		// Don't load this object for the editor.
// unused							DECLARE_UINT64(0x0080000000000000)
#define RF_NeedPostLoad				DECLARE_UINT64(0x0100000000000000)		// Object needs to be postloaded.
#define RF_HasStack					DECLARE_UINT64(0x0200000000000000)		// Has execution stack.
#define RF_Native					DECLARE_UINT64(0x0400000000000000)		// Native (UClass only).
#define RF_Marked					DECLARE_UINT64(0x0800000000000000)		// Marked (for debugging).
#define RF_ErrorShutdown			DECLARE_UINT64(0x1000000000000000)		// ShutdownAfterError called.
#define RF_PendingKill				DECLARE_UINT64(0x2000000000000000)		// Objects that are pending destruction (invalid for gameplay but valid objects)
// unused							DECLARE_UINT64(0x4000000000000000)
// unused							DECLARE_UINT64(0x8000000000000000)


#define RF_ContextFlags				(RF_NotForClient | RF_NotForServer | RF_NotForEdit) // All context flags.
#define RF_LoadContextFlags			(RF_LoadForClient | RF_LoadForServer | RF_LoadForEdit) // Flags affecting loading.
#define RF_Load  					(RF_ContextFlags | RF_LoadContextFlags | RF_Public | RF_Standalone | RF_Native | RF_Obsolete| RF_Protected | RF_Transactional | RF_HasStack | RF_PerObjectLocalized | RF_ClassDefaultObject | RF_ArchetypeObject | RF_LocalizedResource ) // Flags to load from Unrealfiles.
#define RF_Keep						(RF_Native | RF_Marked | RF_PerObjectLocalized | RF_MisalignedObject | RF_DisregardForGC | RF_RootSet | RF_LocalizedResource ) // Flags to persist across loads.
#define RF_ScriptMask				(RF_Transactional | RF_Public | RF_Transient | RF_NotForClient | RF_NotForServer | RF_NotForEdit | RF_Standalone) // Script-accessible flags.
#define RF_UndoRedoMask				(RF_PendingKill)									// Undo/ redo will store/ restore these
#define RF_PropagateToSubObjects	(RF_Public|RF_ArchetypeObject|RF_Transactional)		// Sub-objects will inherit these flags from their SuperObject.

#define RF_AllFlags					DECLARE_UINT64(0xFFFFFFFFFFFFFFFF)
//@}

//
// Advanced animation enum.
//
enum eAnimNotifyEval
{
    ANE_Equal               =0,
    ANE_Greater             =1,
    ANE_Less                =2,
    ANE_GreaterOrEqual      =3,
    ANE_LessOrEqual         =4,
    ANE_MAX                 =5,
};

/*----------------------------------------------------------------------------
	Core types.
----------------------------------------------------------------------------*/

//
// Globally unique identifier.
//
class FGuid
{
public:
	DWORD A,B,C,D;
	FGuid()
	{}
	FGuid( DWORD InA, DWORD InB, DWORD InC, DWORD InD )
	: A(InA), B(InB), C(InC), D(InD)
	{}
	friend UBOOL operator==(const FGuid& X, const FGuid& Y)
		{return X.A==Y.A && X.B==Y.B && X.C==Y.C && X.D==Y.D;}
	friend UBOOL operator!=(const FGuid& X, const FGuid& Y)
		{return X.A!=Y.A || X.B!=Y.B || X.C!=Y.C || X.D!=Y.D;}
	friend FArchive& operator<<( FArchive& Ar, FGuid& G )
	{
		guard(FGuid<<);
		return Ar << G.A << G.B << G.C << G.D;
		unguard;
	}
	TCHAR* String() const
	{
		TCHAR* Result = appStaticString1024();
		appSprintf( Result, TEXT("%08X%08X%08X%08X"), A, B, C, D );
		return Result;
	}
};

inline INT CompareGuids( FGuid* A, FGuid* B )
{
	INT Diff;
	Diff = A->A-B->A; if( Diff ) return Diff;
	Diff = A->B-B->B; if( Diff ) return Diff;
	Diff = A->C-B->C; if( Diff ) return Diff;
	Diff = A->D-B->D; if( Diff ) return Diff;
	return 0;
}

//
// Information about a driver class.
//
class CORE_API FRegistryObjectInfo
{
public:
	FString Object;
	FString Class;
	FString MetaClass;
	FString Description;
	FString Autodetect;
	FRegistryObjectInfo()
	: Object(), Class(), MetaClass(), Description(), Autodetect()
	{}
};

//
// Information about a preferences menu item.
//
class CORE_API FPreferencesInfo
{
public:
	FString Caption;
	FString ParentCaption;
	FString Class;
	FName Category;
	UBOOL Immediate;
	FPreferencesInfo()
	: Caption(), ParentCaption(), Class(), Category(NAME_None), Immediate(0)
	{}
};

/*----------------------------------------------------------------------------
	Core macros.
----------------------------------------------------------------------------*/

// Special canonical package for FindObject, ParseObject.
#define ANY_PACKAGE ((UPackage*)-1)
#define INVALID_OBJECT ((UObject*)-1)

// Define private default constructor.
#define NO_DEFAULT_CONSTRUCTOR(cls) \
	protected: cls() {} public:

// Guard macros.
#define unguardobjSlow		unguardfSlow(( TEXT("(%ls)"), GetFullName() ))
#define unguardobj			unguardf(( TEXT("(%ls)"), GetFullName() ))

// Verify the a class definition and C++ definition match up.
#define VERIFY_CLASS_OFFSET(Pre,ClassName,Member) \
	{for( TFieldIterator<UProperty> It( FindObjectChecked<UClass>( Pre##ClassName::StaticClass()->GetOuter(), TEXT(#ClassName) ) ); It; ++It ) \
		if( appStricmp(It->GetName(),TEXT(#Member))==0 ) \
			if( It->Offset != STRUCT_OFFSET(Pre##ClassName,Member) ) \
				appErrorf(TEXT("Class %ls Member %ls problem: Script=%i C++=%i"), TEXT(#ClassName), TEXT(#Member), It->Offset, STRUCT_OFFSET(Pre##ClassName,Member) );}

// Verify that C++ and script code agree on the size of a class.
#define VERIFY_CLASS_SIZE(ClassName) \
	check(sizeof(ClassName)==ClassName::StaticClass()->GetPropertiesSize());

// Verify a class definition and C++ definition match up (don't assert).
#define VERIFY_CLASS_OFFSET_NODIE(Pre,ClassName,Member) \
{ \
	bool found = false; \
	for( TFieldIterator<UProperty> It( FindObjectChecked<UClass>( Pre##ClassName::StaticClass()->GetOuter(), TEXT(#ClassName) ) ); It; ++It ) \
	{ \
		if( appStricmp(It->GetName(),TEXT(#Member))==0 ) { \
			found = true; \
			if( It->Offset != STRUCT_OFFSET(Pre##ClassName,Member) ) { \
				debugf(TEXT("VERIFY: Class %ls Member %ls problem: Script=%i C++=%i\n"), TEXT(#ClassName), TEXT(#Member), It->Offset, STRUCT_OFFSET(Pre##ClassName,Member) ); \
				Mismatch = true; \
			} \
		} \
	} \
	if (!found) { \
		debugf(TEXT("VERIFY: Class '%ls' doesn't appear to have a '%ls' member %ls problem: Script=%i C++=%i\n"), TEXT(#ClassName), TEXT(#Member) ); \
		Mismatch = true; \
	} \
}

// Verify that C++ and script code agree on the size of a class (don't assert).
#define VERIFY_CLASS_SIZE_NODIE(ClassName) \
	if (sizeof(ClassName)!=ClassName::StaticClass()->GetPropertiesSize()) { \
		debugf(TEXT("VERIFY: Class %ls size problem; Script=%i C++=%i"), TEXT(#ClassName), (int) ClassName::StaticClass()->GetPropertiesSize(), sizeof(ClassName)); \
		Mismatch = true; \
	}

// Declare the base UObject class.

#define DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, TPackage, bNoInstance ) \
public: \
	/* Identification */ \
	enum {StaticClassFlags=TStaticFlags}; \
	private: static UClass PrivateStaticClass; public: \
	typedef TSuperClass Super;\
	typedef TClass ThisClass;\
	static UBOOL AllowInstancing() { return bNoInstance; } \
	static const TCHAR* GetClassPackage() { return TEXT(#TPackage); } \
	static UClass* StaticClass() \
		{ return &PrivateStaticClass; } \
	void* operator new( size_t Size, UObject* Outer=(UObject*)GetTransientPackage(), FName Name=NAME_None, EObjectFlags SetFlags=0 ) \
		{ return StaticAllocateObject( StaticClass(), Outer, Name, SetFlags ); } \
	void* operator new( size_t Size, EInternal* Mem ) \
		{ return (void*)Mem; }

// Declare a concrete class.
#define DECLARE_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, TPackage, FALSE ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return Ar << *(UObject**)&Res; } \
    virtual ~TClass() noexcept(false) \
		{ ConditionalDestroy(); } \
	static void InternalConstructor( void* X ) \
		{ new( (EInternal*)X )TClass; } \

// Declare an abstract class.
#define DECLARE_ABSTRACT_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_Abstract, TPackage, FALSE ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return Ar << *(UObject**)&Res; } \
	virtual ~TClass() \
		{ ConditionalDestroy(); } \

// Declare that objects of class being defined reside within objects of the specified class.
#define DECLARE_WITHIN( TWithinClass ) \
	typedef TWithinClass WithinClass; \
	TWithinClass* GetOuter##TWithinClass() const { return (TWithinClass*)GetOuter(); }

// Register a class at startup time.
#define IMPLEMENT_CLASS(TClass) \
	UClass TClass::PrivateStaticClass \
	( \
		EC_NativeConstructor, \
		sizeof(TClass), \
		TClass::StaticClassFlags, \
		TClass::Super::StaticClass(), \
		TClass::WithinClass::StaticClass(), \
		FGuid(TClass::GUID1,TClass::GUID2,TClass::GUID3,TClass::GUID4), \
		&TEXT(#TClass)[1], \
		TClass::GetClassPackage(), \
		StaticConfigName(), \
		RF_Public | RF_Standalone | RF_Transient | RF_Native, \
		(void(*)(void*))TClass::InternalConstructor, \
		(void(UObject::*)())&TClass::StaticConstructor, \
		TClass::AllowInstancing() \
	); \
	UClass* autoclass##TClass = TClass::StaticClass();

// Define the package of the current DLL being compiled.
#define IMPLEMENT_PACKAGE(pkg)

#define DISABLE_SERIALIZATION void Serialize( FArchive& Ar ) { if(Ar.IsSaving() && !Ar.IsObjectReferenceCollector()) appErrorf(TEXT("%ls can't currently be saved as instanced object."), GetFullName()); }
#define IMPLEMENT_EMPTY_OBJ(objn) \
public: \
	objn() {} \
	DISABLE_SERIALIZATION

#define DECLARE_NOINST_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_NoScriptProps, TPackage, TRUE ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return Ar << *(UObject**)&Res; } \
    virtual ~TClass() noexcept(false) \
		{ ConditionalDestroy(); } \
	static void InternalConstructor( void* X ) \
		{ new( (EInternal*)X )TClass; } \
public: \
	TClass() {} \
	DISABLE_SERIALIZATION

/*-----------------------------------------------------------------------------
	UObject.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

class UComponent;

//
// The base class of all objects.
//
class UObject
{
	// Declarations.
	DECLARE_BASE_CLASS(UObject,UObject,CLASS_Abstract,Core,FALSE)
	typedef UObject WithinClass;
	enum {GUID1=0,GUID2=0,GUID3=0,GUID4=0};
	static const TCHAR* StaticConfigName() {return TEXT("System");}

	// Friends.
	friend class FObjectIterator;
	friend class FPackageObjectIterator;
	friend class ULinkerLoad;
	friend class ULinkerSave;
	friend class UPackageMap;
	friend class FArchiveTagUsed;
	friend class URipAndTearCommandlet;
	friend struct FObjectImport;
	friend struct FObjectExport;
	friend class UStruct;
	friend class FSlowGCArc;
	friend struct FFrame;

private:
	// Internal per-object variables.
	UObject*				HashNext;			// Next object in this hash bin.
	EObjectFlags			ObjectFlags;		// Private EObjectFlags used by object manager.
	UObject*				HashOuterNext;		/** Next object in the hash bin that includes outers */
	FStateFrame*			StateFrame;			// Main script execution stack.
	ULinkerLoad*			_Linker;			// Linker it came from, or NULL if none.
	INT						_LinkerIndex;
	INT						Index;				// Index of object into table.
	INT						NetIndex;
	UObject*				Outer;

	// Name of the object.
	FName					Name;
	UClass*					Class;	  			// Class the object belongs to.

	UObject*				ObjectArchetype;

	// Private systemwide variables.
	static UBOOL			GObjInitialized;	// Whether initialized.
	static UBOOL            GObjNoRegister;		// Registration disable.
	static INT				GObjBeginLoadCount;	// Count for BeginLoad multiple loads.
	static INT				GObjRegisterCount;  // ProcessRegistrants entry counter.
	static INT				GImportCount;		// Imports for EndLoad optimization.
	static UObject*			GObjHash[4096];		// Object hash.
	static UObject*			GAutoRegister;		// Objects to automatically register.
	static TArray<UObject*> GObjLoaded;			// Objects that might need preloading.
	static TArray<UObject*>	GObjRoot;			// Top of active object graph.
	static TArray<UObject*>	GObjObjects;		// List of all objects.
	static TArray<INT>      GObjAvailable;		// Available object indices.
	static TArray<UObject*>	GObjLoaders;		// Array of loaders.
	static UPackage*		GObjTransientPkg;	// Transient package.
	static TCHAR			GObjCachedLanguage[32]; // Language;
	static FStringNoInit	TraceIndent;
	static TArray<UObject*> GObjRegistrants;		// Registrants during ProcessRegistrants call.
	static TArray<FPreferencesInfo> GObjPreferences; // Preferences cache.
	static TArray<FRegistryObjectInfo> GObjDrivers; // Drivers cache.
	static TMultiMap<FName,FName>* GObjPackageRemap; // Remap table for loading renamed packages.
	static TCHAR GLanguage[256];

	// Private functions.
	void AddObject( INT Index );
	void HashObject();
	void UnhashObject( INT OuterIndex );
	void SetLinker( ULinkerLoad* L, INT I );

	// Private systemwide functions.
	static ULinkerLoad* GetLoader( INT i );
	static FName MakeUniqueObjectName( UObject* Outer, UClass* Class );
	static UBOOL ResolveName( UObject*& Outer, FString& Name, UBOOL Create, UBOOL Throw );
	static void SafeLoadError( DWORD LoadFlags, const TCHAR* Error, const TCHAR* Fmt, ... );
	static void PurgeGarbage();

protected:
	TArray<BYTE>			UnrealScriptData;
	BYTE* GetInternalScriptData(INT DesiredSize);

public:
	// Constructors.
	UObject();
	UObject( const UObject& Src );
	UObject( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );
	UObject( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );
	UObject( EInPlaceConstructor, UClass* InClass, UObject* InOuter, FName InName, EObjectFlags InFlags );
	UObject& operator=( const UObject& );
	void StaticConstructor();
	static void InternalConstructor( void* X )
		{ new( (EInternal*)X )UObject; }

	// Destructors.
	virtual ~UObject() noexcept(false);
	void operator delete( void* Object, size_t Size )
		{guard(UObject::operator delete); appFree( Object ); unguard;}

	// UObject interface.
	virtual void Modify();
	virtual void PostLoad();
	virtual void Destroy();
	void SerializeNetIndex(FArchive& Ar);
	virtual void Serialize( FArchive& Ar );
	inline UBOOL IsPendingKill() const { return (ObjectFlags & RF_PendingKill) != 0; }
	virtual void InitExecution();
	virtual void EndExecution();
	virtual void ShutdownAfterError();
	virtual void PostEditChange();
	virtual void Register();
	virtual void onOuterChanged() {}
	virtual UComponent* FindComponent(FName ComponentName, UBOOL bRecurse = FALSE);
	void SerializeEmpty(FArchive& Ar);

	// Systemwide functions.
	static UObject* StaticFindObject( UClass* Class, UObject* InOuter, const TCHAR* Name, UBOOL ExactClass=0 );
	static UObject* StaticFindObjectChecked( UClass* Class, UObject* InOuter, const TCHAR* Name, UBOOL ExactClass=0 );
	static UObject* StaticLoadObject( UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags );
	static UClass* StaticLoadClass( UClass* BaseClass, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags );
	static UObject* StaticAllocateObject( UClass* Class, UObject* InOuter=(UObject*)GetTransientPackage(), FName Name=NAME_None, EObjectFlags SetFlags=0, UObject* Template=NULL, FOutputDevice* Error=GError, UObject* Ptr=NULL, UObject* SubobjectRoot = NULL);
	static UObject* StaticConstructObject( UClass* Class, UObject* InOuter=(UObject*)GetTransientPackage(), FName Name=NAME_None, EObjectFlags SetFlags=0, UObject* Template=NULL, FOutputDevice* Error=GError, UObject* SubobjectRoot = NULL);
	static void StaticInit();
	static void StaticExit();
	static UObject* LoadPackage( UObject* InOuter, const TCHAR* Filename, DWORD LoadFlags );
	static UBOOL SavePackage( UObject* InOuter, UObject* Base, DWORD TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error=GError, ULinkerLoad* Conform=NULL, UBOOL bAutoSave=0, struct FSavedGameSummary* SaveSummary=NULL);
	static void CollectGarbage( DWORD KeepFlags );
	static void SerializeRootSet( FArchive& Ar, DWORD KeepFlags, DWORD RequiredFlags );
	static UBOOL IsReferenced( UObject*& Res, DWORD KeepFlags, UBOOL IgnoreReference );
	static UBOOL AttemptDelete( UObject*& Res, DWORD KeepFlags, UBOOL IgnoreReference );
	static void BeginLoad();
	static void EndLoad();
	void SafeInitProperties(BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* DefaultData, INT DefaultsCount, UObject* DestObject = NULL, UObject* SubobjectRoot = NULL);
	static void InitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* Defaults, INT DefaultsCount, UObject* RefObj=NULL);
	static void ExitProperties( BYTE* Data, UClass* Class );
	static void ResetLoaders( UObject* InOuter, UBOOL DynamicOnly, UBOOL ForceLazyLoad );
	static UPackage* CreatePackage( UObject* InOuter, const TCHAR* PkgName );
	static ULinkerLoad* GetPackageLinker( UObject* InOuter, const TCHAR* Filename, DWORD LoadFlags, UObject* MainPackage=NULL );
	static void StaticShutdownAfterError();
	static UObject* GetIndexedObject( INT Index );
	static void ExportProperties( FOutputDevice& Out, UClass* ObjectClass, BYTE* Object, INT Indent, UClass* DiffClass, BYTE* Diff );
	static UBOOL GetInitialized();
	static UPackage* GetTransientPackage();
	static void VerifyLinker( ULinkerLoad* Linker );
	static void ProcessRegistrants();
	static inline INT GetObjectHash( FName ObjName, INT Outer )
	{
		return (ObjName.GetIndex() ^ Outer) & (ARRAY_COUNT(GObjHash)-1);
	}
	static void DEBUG_VerifyObjects(const TCHAR* Tag);

	// Functions.
	void AddToRoot();
	void RemoveFromRoot();
	const TCHAR* GetFullName() const;
	const TCHAR* GetPathName( UObject* StopOuter=NULL ) const;
    FString GetFullNameSafe() const; // stijn: binarycompat - improved version
	FString GetPathNameSafe( UObject* StopOuter=NULL ) const; // stijn: binarycompat - improved version
	UBOOL IsValid();
	void ConditionalRegister();
	UBOOL ConditionalDestroy();
	void ConditionalPostLoad();
	void ConditionalShutdownAfterError();
	inline UBOOL IsA( UClass* SomeBaseClass ) const;
	UBOOL IsIn( UObject* SomeOuter ) const;
	UBOOL IsProbing( FName ProbeName );
	void Rename( const TCHAR* NewName=NULL, UObject* NewOuter=((UObject*)-1));
	UField* FindObjectField( FName InName, UBOOL Global=0 );
	UFunction* FindFunction( FName InName, UBOOL Global=0 );
	UFunction* FindFunctionChecked( FName InName, UBOOL Global=0 );
	UState* FindState( FName InName );
	void InitClassDefaultObject( UClass* InClass );
	void ParseParms( const TCHAR* Parms );
	UBOOL ParsePropertyValue(void* Result, const TCHAR** Str, BYTE bIntValue);
	void* GetInterfaceAddress(UClass* InterfaceClass);
	virtual UBOOL IsAComponent() const
	{
		return FALSE;
	}
	virtual BYTE* GetScriptData();
	inline INT GetScriptDataSize()
	{
		return UnrealScriptData.Num();
	}

	// Accessors.
	UClass* GetClass() const
	{
		return Class;
	}
	void SetClass(UClass* NewClass)
	{
		Class = NewClass;
	}
	inline EObjectFlags GetFlags() const
	{
		return ObjectFlags;
	}
	inline EObjectFlags GetMaskedFlags(EObjectFlags Flags) const
	{
		return (ObjectFlags & Flags);
	}
	inline UBOOL HasAnyFlags(EObjectFlags Flags) const
	{
		return (ObjectFlags & Flags) != 0;
	}
	void SetFlags(EObjectFlags NewFlags )
	{
		ObjectFlags |= NewFlags;
		checkSlow(Name!=NAME_None || !(ObjectFlags&RF_Public));
	}
	void ClearFlags(EObjectFlags NewFlags )
	{
		ObjectFlags &= ~NewFlags;
		checkSlow(Name!=NAME_None || !(ObjectFlags&RF_Public));
	}
	const TCHAR* GetName() const
	{
		const TCHAR* Result;
		try
		{
			Result = *Name;
		}
		catch(...)
		{
			Result = TEXT("<Invalid GetName call>");
		}
		return Result;
	}
	const TCHAR* GetSafeName() const
	{
		if (Name.IsValid())
			return GetName();
		return *(const TCHAR**)&Name;
	}
	const FName GetFName() const
	{
		if (Name.IsValid())
			return Name;
		else
			return NAME_Invalid;
	}
	UObject* GetOuter() const
	{
		return Outer;
	}
	DWORD GetIndex() const
	{
		return Index;
	}
	ULinkerLoad* GetLinker()
	{
		return _Linker;
	}
	INT GetLinkerIndex()
	{
		return _LinkerIndex;
	}
	FStateFrame* GetStateFrame()
	{
		return StateFrame;
	}
	inline UObject* TopOuter()
	{
		UObject* Obj = this;
		if (!Obj)
			return NULL;
		while (Obj->Outer)
			Obj = Obj->Outer;
		return Obj;
	}
	UPackage* GetOutermost();

	inline UBOOL IsTemplate(EObjectFlags TemplateTypes = (RF_ArchetypeObject | RF_ClassDefaultObject)) const
	{
		for (const UObject* TestOuter = this; TestOuter; TestOuter = TestOuter->GetOuter())
		{
			if (TestOuter->ObjectFlags & TemplateTypes)
				return TRUE;
		}
		return FALSE;
	}
	inline UObject* GetArchetype() const
	{
		return ObjectArchetype;
	}

	virtual void SetArchetype(UObject* NewArchetype, UBOOL bReinitialize = FALSE);
	void InitializeProperties(UObject* SourceObject = NULL);

	inline UBOOL IsBasedOnArchetype(const UObject* const SomeObject) const
	{
		if (this && SomeObject != this)
		{
			for (UObject* Template = ObjectArchetype; Template; Template = Template->GetArchetype())
			{
				if (SomeObject == Template)
					return TRUE;
			}
		}
		return FALSE;
	}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

inline const TCHAR* ObjectName(UObject* Object)
{
	if (!Object)
		return TEXT("None");
	return Object->GetName();
}
inline const TCHAR* ObjectPathName(UObject* Object)
{
	if (!Object)
		return TEXT("None");
	return Object->GetPathName();
}
inline const TCHAR* ObjectFullName(UObject* Object)
{
	if (!Object)
		return TEXT("None None");
	return Object->GetFullName();
}

/*----------------------------------------------------------------------------
	Core templates.
----------------------------------------------------------------------------*/

// Hash function.
inline DWORD GetTypeHash( const UObject* A )
{
	return A ? A->GetIndex() : 0;
}

// Parse an object name in the input stream.
template< class T > UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, T*& Obj, UObject* Outer )
{
	return ParseObject( Stream, Match, T::StaticClass(), *(UObject **)&Obj, Outer );
}

// Find an optional object.
template< class T > T* FindObject( UObject* Outer, const TCHAR* Name, UBOOL ExactClass=0 )
{
	return (T*)UObject::StaticFindObject( T::StaticClass(), Outer, Name, ExactClass );
}

// Find an object, no failure allowed.
template< class T > T* FindObjectChecked( UObject* Outer, const TCHAR* Name, UBOOL ExactClass=0 )
{
	return (T*)UObject::StaticFindObjectChecked( T::StaticClass(), Outer, Name, ExactClass );
}

// Dynamically cast an object type-safely.
template< class T > T* Cast( UObject* Src )
{
	return Src && Src->IsA(T::StaticClass()) ? (T*)Src : NULL;
}
template< class T, class U > T* CastChecked( U* Src )
{
	if( !Src || !Src->IsA(T::StaticClass()) )
		appErrorf( TEXT("Cast of %ls to %ls failed"), Src ? Src->GetFullName() : TEXT("NULL"), T::StaticClass()->GetName() );
	return (T*)Src;
}

// Construct an object of a particular class.
template< class T > T* ConstructObject( UClass* Class, UObject* Outer=(UObject*)-1, FName Name=NAME_None, DWORD SetFlags=0, UObject* Template = NULL, UObject* SubobjectRoot = NULL)
{
	// check(Class->IsChildOf(T::StaticClass())); // check later, Smirftsch
	if( Outer==(UObject*)-1 )
		Outer = (UObject*)UObject::GetTransientPackage();
	return (T*)UObject::StaticConstructObject( Class, Outer, Name, SetFlags, Template, GError, SubobjectRoot);
}

// Load an object.
template< class T > T* LoadObject( UObject* Outer, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags )
{
	return (T*)UObject::StaticLoadObject( T::StaticClass(), Outer, Name, Filename, LoadFlags );
}

// Load a class object.
template< class T > UClass* LoadClass( UObject* Outer, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags )
{
	return UObject::StaticLoadClass( T::StaticClass(), Outer, Name, Filename, LoadFlags );
}

// Get default object of a class.
template< class T > T* GetDefault()
{
	return (T*)&T::StaticClass()->GetDefaults();
}

/*-----------------------------------------------------------------------------
	FScriptDelegate.
-----------------------------------------------------------------------------*/
enum EEventParm { EC_EventParm };

struct FScriptDelegate
{
	UObject* Object;
	FName FunctionName;

	/** Constructors */
	/** Default ctor - doesn't initialize any members */
	FScriptDelegate() {}

	/** Event parm ctor - zeros all members */
	FScriptDelegate(EEventParm)
		: Object(NULL), FunctionName(NAME_None)
	{}

	inline UBOOL IsCallable(const UObject* OwnerObject) const
	{
		// if Object is NULL, it means that the delegate was assigned to a member function through defaultproperties; in this case, OwnerObject
		// will be the object that contains the function referenced by FunctionName.
		return FunctionName != NAME_None && (Object != NULL ? !Object->IsPendingKill() : (OwnerObject != NULL && !OwnerObject->IsPendingKill()));
	}

	/**
	 * Returns the
	 */
	inline FString ToString(const UObject* OwnerObject) const
	{
		const UObject* DelegateObject = Object;
		if (DelegateObject == NULL)
		{
			DelegateObject = OwnerObject;
		}

		return FString(DelegateObject->GetPathName()) + TEXT(".") + *FunctionName;
	}

	friend FArchive& operator<<(FArchive& Ar, FScriptDelegate& D)
	{
		Ar << D.Object << D.FunctionName;
		return Ar;
	}

	/** Comparison operators */
	FORCEINLINE UBOOL operator==(const FScriptDelegate& Other) const
	{
		return Object == Other.Object && FunctionName == Other.FunctionName;
	}

	FORCEINLINE UBOOL operator!=(const FScriptDelegate& Other) const
	{
		return Object != Other.Object || FunctionName != Other.FunctionName;
	}
};

class FScriptInterface
{
private:
	/**
	 * A pointer to a UObject that implements a native interface.
	 */
	UObject* ObjectPointer;

	/**
	 * Pointer to the location of the interface object within the UObject referenced by ObjectPointer.
	 */
	void* InterfacePointer;

public:
	/**
	 * Default constructor
	 */
	FScriptInterface(UObject* InObjectPointer = NULL, void* InInterfacePointer = NULL)
		: ObjectPointer(InObjectPointer), InterfacePointer(InInterfacePointer)
	{}

	/**
	 * Returns the ObjectPointer contained by this FScriptInterface
	 */
	FORCEINLINE UObject* GetObject() const
	{
		return ObjectPointer;
	}

	/**
	 * Returns the ObjectPointer contained by this FScriptInterface
	 */
	FORCEINLINE UObject*& GetObjectRef()
	{
		return ObjectPointer;
	}

	/**
	 * Returns the pointer to the interface
	 */
	FORCEINLINE void* GetInterface() const
	{
		// only allow access to InterfacePointer if we have a valid ObjectPointer.  This is necessary because the garbage collector will set ObjectPointer to NULL
		// without using the accessor methods
		return ObjectPointer != NULL ? InterfacePointer : NULL;
	}

	/**
	 * Sets the value of the ObjectPointer for this FScriptInterface
	 */
	FORCEINLINE void SetObject(UObject* InObjectPointer)
	{
		ObjectPointer = InObjectPointer;
		if (ObjectPointer == NULL)
		{
			SetInterface(NULL);
		}
	}

	/**
	 * Sets the value of the InterfacePointer for this FScriptInterface
	 */
	FORCEINLINE void SetInterface(void* InInterfacePointer)
	{
		InterfacePointer = InInterfacePointer;
	}

	/**
	 * Comparison operator, taking a reference to another FScriptInterface
	 */
	FORCEINLINE UBOOL operator==(const FScriptInterface& Other) const
	{
		return GetInterface() == Other.GetInterface() && ObjectPointer == Other.GetObject();
	}
	FORCEINLINE UBOOL operator!=(const FScriptInterface& Other) const
	{
		return GetInterface() != Other.GetInterface() || ObjectPointer != Other.GetObject();
	}
};

class UInterface : public UObject
{
public:
	DECLARE_ABSTRACT_CLASS(UInterface, UObject, 0 | CLASS_Interface, Core)
	NO_DEFAULT_CONSTRUCTOR(UInterface)
};

/*----------------------------------------------------------------------------
	Object iterators.
----------------------------------------------------------------------------*/

//
// Class for iterating through all objects.
//
class FObjectIterator
{
public:
	FObjectIterator( UClass* InClass=UObject::StaticClass() )
	:	Class( InClass ), Index( -1 )
	{
		check(Class);
		++*this;
	}
	void operator++()
	{
		while( ++Index<UObject::GObjObjects.Num() && (!UObject::GObjObjects(Index) || !UObject::GObjObjects(Index)->IsA(Class)) );
	}
	UObject* operator*()
	{
		return UObject::GObjObjects(Index);
	}
	UObject* operator->()
	{
		return UObject::GObjObjects(Index);
	}
	operator UBOOL()
	{
		return Index<UObject::GObjObjects.Num();
	}
protected:
	UClass* Class;
	INT Index;
};

class FPackageObjectIterator
{
public:
	FPackageObjectIterator(UObject* InParent)
		: ParentObject(InParent), Index(-1)
	{
		check(ParentObject);
		++(*this);
	}
	void operator++()
	{
		while (++Index < UObject::GObjObjects.Num() && (!UObject::GObjObjects(Index) || !UObject::GObjObjects(Index)->IsIn(ParentObject)));
	}
	UObject* operator*()
	{
		return UObject::GObjObjects(Index);
	}
	UObject* operator->()
	{
		return UObject::GObjObjects(Index);
	}
	operator UBOOL()
	{
		return Index < UObject::GObjObjects.Num();
	}
protected:
	UObject* ParentObject;
	INT Index;
};

//
// Class for iterating through all objects which inherit from a
// specified base class.
//
template< class T > class TObjectIterator : public FObjectIterator
{
public:
	TObjectIterator()
	:	FObjectIterator( T::StaticClass() )
	{}
	T* operator* ()
	{
		return (T*)FObjectIterator::operator*();
	}
	T* operator-> ()
	{
		return (T*)FObjectIterator::operator->();
	}
};

/*----------------------------------------------------------------------------
	FArchiveReplaceObjectRef.
----------------------------------------------------------------------------*/
/**
 * Archive for replacing a reference to an object. This classes uses
 * serialization to replace all references to one object with another.
 * Note that this archive will only traverse objects with an Outer
 * that matches InSearchObject.
 *
 * NOTE: The template type must be a child of UObject or this class will not compile.
 */
template< class T >
class FArchiveReplaceObjectRef : public FArchive
{
public:
	/**
	 * Initializes variables and starts the serialization search
	 *
	 * @param InSearchObject		The object to start the search on
	 * @param ReplacementMap		Map of objects to find -> objects to replace them with (null zeros them)
	 * @param bNullPrivateRefs		Whether references to non-public objects not contained within the SearchObject
	 *								should be set to null
	 * @param bIgnoreOuterRef		Whether we should replace Outer pointers on Objects.
	 * @param bIgnoreArchetypeRef	Whether we should replace the ObjectArchetype reference on Objects.
	 * @param bDelayStart			Specify TRUE to prevent the constructor from starting the process.  Allows child classes' to do initialization stuff in their ctor
	 */
	FArchiveReplaceObjectRef
	(
		UObject* InSearchObject,
		const TMap<T*, T*>& inReplacementMap,
		UBOOL bNullPrivateRefs,
		UBOOL bIgnoreOuterRef,
		UBOOL bIgnoreArchetypeRef,
		UBOOL bDelayStart = FALSE
	)
		: SearchObject(InSearchObject), ReplacementMap(inReplacementMap)
		, Count(0), bNullPrivateReferences(bNullPrivateRefs)
	{
		ArIsObjectReferenceCollector = TRUE;
		if (!bDelayStart)
		{
			SerializeSearchObject();
		}
	}

	/**
	 * Starts the serialization of the root object
	 */
	void SerializeSearchObject()
	{
		if (SearchObject != NULL && !SerializedObjects.Find(SearchObject)
			&& (ReplacementMap.Num() > 0 || bNullPrivateReferences))
		{
			// start the initial serialization
			SerializedObjects.Set(SearchObject);

			// serialization for class default objects must be deterministic (since class 
			// default objects may be serialized during script compilation while the script
			// and C++ versions of a class are not in sync), so use SerializeTaggedProperties()
			// rather than the native Serialize() function
			if (SearchObject->HasAnyFlags(RF_ClassDefaultObject))
			{
				SearchObject->GetClass()->SerializeDefaultObject(SearchObject, *this);
			}
			else
			{
				SearchObject->Serialize(*this);
			}
		}
	}

	/**
	 * Returns the number of times the object was referenced
	 */
	INT GetCount()
	{
		return Count;
	}

	/**
	 * Returns a reference to the object this archive is operating on
	 */
	const UObject* GetSearchObject() const { return SearchObject; }

	/**
	 * Serializes the reference to the object
	 */
	FArchive& operator<<(UObject*& Obj)
	{
		if (Obj != NULL)
		{
			// If these match, replace the reference
			T* const* ReplaceWith = (T* const*)((const TMap<UObject*, UObject*>*) & ReplacementMap)->Find(Obj);
			if (ReplaceWith != NULL)
			{
				Obj = *ReplaceWith;
				Count++;
			}
			// A->IsIn(A) returns FALSE, but we don't want to NULL that reference out, so extra check here.
			else if (Obj == SearchObject || Obj->IsIn(SearchObject))
			{
#if 0
				// DEBUG: Log when we are using the A->IsIn(A) path here.
				if (Obj == SearchObject)
				{
					FString ObjName = Obj->GetPathName();
					debugf(TEXT("FArchiveReplaceObjectRef: Obj == SearchObject : '%s'"), *ObjName);
				}
#endif

				if (!SerializedObjects.Find(Obj))
				{
					// otherwise recurse down into the object if it is contained within the initial search object
					SerializedObjects.Set(Obj);

					// serialization for class default objects must be deterministic (since class 
					// default objects may be serialized during script compilation while the script
					// and C++ versions of a class are not in sync), so use SerializeTaggedProperties()
					// rather than the native Serialize() function
					if (Obj->HasAnyFlags(RF_ClassDefaultObject))
					{
						Obj->GetClass()->SerializeDefaultObject(Obj, *this);
					}
					else
					{
						Obj->Serialize(*this);
					}
				}
			}
			else if (bNullPrivateReferences && !Obj->HasAnyFlags(RF_Public))
			{
				Obj = NULL;
			}
		}
		return *this;
	}

protected:
	/** Initial object to start the reference search from */
	UObject* SearchObject;

	/** Map of objects to find references to -> object to replace references with */
	const TMap<T*, T*>& ReplacementMap;

	/** The number of times encountered */
	INT Count;

	/** List of objects that have already been serialized */
	TSingleMap<UObject*> SerializedObjects;

	/**
	 * Whether references to non-public objects not contained within the SearchObject
	 * should be set to null
	 */
	UBOOL bNullPrivateReferences;
};

#define AUTO_INITIALIZE_REGISTRANTS \
	UObject::StaticClass(); \
	UClass::StaticClass(); \
	USubsystem::StaticClass(); \
	USystem::StaticClass(); \
	UProperty::StaticClass(); \
	UStructProperty::StaticClass(); \
	UField::StaticClass(); \
	UMapProperty::StaticClass(); \
	UArrayProperty::StaticClass(); \
	UFixedArrayProperty::StaticClass(); \
	UStrProperty::StaticClass(); \
	UNameProperty::StaticClass(); \
	UClassProperty::StaticClass(); \
	UObjectProperty::StaticClass(); \
	UFloatProperty::StaticClass(); \
	UBoolProperty::StaticClass(); \
	UIntProperty::StaticClass(); \
	UByteProperty::StaticClass(); \
	ULanguage::StaticClass(); \
	UTextBufferFactory::StaticClass(); \
	UFactory::StaticClass(); \
	UPackage::StaticClass(); \
	UCommandlet::StaticClass(); \
	ULinkerSave::StaticClass(); \
	ULinker::StaticClass(); \
	ULinkerLoad::StaticClass(); \
	UEnum::StaticClass(); \
	UTextBuffer::StaticClass(); \
	UPackageMap::StaticClass(); \
	UConst::StaticClass(); \
	UFunction::StaticClass(); \
	UStruct::StaticClass();

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
