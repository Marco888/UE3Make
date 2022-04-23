#pragma once

#include "Core.h"
#include "UnAssetCache.h"

void MakeMain();

const TCHAR* ImportObjectProperties(
	BYTE* DestData,
	const TCHAR* SourceText,
	UStruct* ObjectStruct,
	UObject* SubobjectRoot,
	UObject* SubobjectOuter,
	FFeedbackContext* Warn,
	INT					Depth,
	INT					LineNumber = INDEX_NONE
);
const TCHAR* ImportDefaultProperties(
	UClass* Class,
	const TCHAR* Text,
	FFeedbackContext* Warn,
	INT					Depth,
	INT					LineNumber = INDEX_NONE
);

class UEditor
{
public:
	FString EditPackagesOutPath,EditPackagesInPath,DumpClass;
	TArray<FString> EditPackages,LoadPackages;
	INT Bootstrapping;
	UObject* ParentContext;
	INT iFunctionOffset;
	UBOOL bShouldObfuscate;
	TMap<FName, BYTE> EnumLookup;

	UEditor();

	static void InitEditor();

	static const TCHAR* UEditor::GetObscName(INT Index);
	static const TCHAR* UEditor::GetObscNameUnique(UObject* Outer);

	void ExportHeader(UStruct* S);
	void ObfuscatePck(UPackage* P);

	UBOOL SafeExec(const TCHAR* Cmd, FOutputDevice& Out);

	UBOOL MakeScripts(FFeedbackContext* Warn, UPackage* MyPackage, TArray<UClass*>& AllClasses);

	BYTE FindEnumValue(FName ValueName);
	BYTE FindEnumValue(const TCHAR* ValueName);

	static void InitDebugger();

	static UBOOL ImportDefaultProps(UClass* Class);
};

inline void SkipWhitespace(const TCHAR*& Str)
{
	while (*Str == ' ' || *Str == '\t')
		Str++;
}

extern UEditor* GEditor;
