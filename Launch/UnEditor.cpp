
#include "UnEditor.h"

UEditor* GEditor=NULL;

void UEditor::InitEditor()
{
	GEditor = new UEditor();
}
UEditor::UEditor()
	: Bootstrapping(1), ParentContext(NULL), iFunctionOffset(41), bShouldObfuscate(FALSE)
{
	GConfig->GetString(TEXT("Make"), TEXT("OutPath"), EditPackagesOutPath);
	GConfig->GetString(TEXT("Make"), TEXT("InPath"), EditPackagesInPath);
	GConfig->GetString(TEXT("Make"), TEXT("DumpClass"), DumpClass);
	GConfig->GetArray(TEXT("Make"), TEXT("LoadPackages"), &LoadPackages);
	GConfig->GetArray(TEXT("Make"), TEXT("EditPackages"), &EditPackages);
	GConfig->GetArray(TEXT("Make"), TEXT("Paths"), &GSys->Paths);
	GConfig->GetArray(TEXT("Make"), TEXT("Extensions"), &GSys->Extensions);
	GConfig->GetInt(TEXT("Make"), TEXT("CodeMemoryOffset"), iFunctionOffset);
	GConfig->GetBool(TEXT("Make"), TEXT("Obfuscate"), bShouldObfuscate);
}
UBOOL UEditor::SafeExec(const TCHAR* Cmd, FOutputDevice& Out)
{
	return FALSE;
}

static void DumpStructParms(FOutputDevice& Ar, UStruct* F, UBOOL bIsLocal, UBOOL bIsParms)
{
	UProperty* LastProperty = NULL;
	for (TFieldIterator<UProperty> It(F); It; ++It)
	{
		if (It.GetStruct() == F && It->ElementSize)
		{
			Ar.Log(TEXT("\t"));
			It->ExportCpp(Ar, bIsLocal, bIsParms);
			bool doAlign = false;

			if (It->IsA(UBoolProperty::StaticClass()))
			{
				if (!LastProperty || !LastProperty->IsA(UBoolProperty::StaticClass()))
					doAlign = true;
			}
			else
			{
				if (LastProperty == 0)
					doAlign = true;
				else if (LastProperty->IsA(UBoolProperty::StaticClass()))
					doAlign = true;
				else if (LastProperty->IsA(UByteProperty::StaticClass()) && !It->IsA(UByteProperty::StaticClass()))
					doAlign = true;
			}
			if (doAlign)
				Ar.Log(TEXT(" GCC_PACK(INT_ALIGNMENT)"));
			Ar.Log(TEXT(";\r\n"));
		}
		LastProperty = *It;
	}
}
void UEditor::ExportHeader(UStruct* S)
{
	FStringOutputDevice Out;

	TArray<UStruct*> ExpStructs;
	if (S->GetClass() == UClass::StaticClass())
	{
		UClass* C = (UClass*)S;
		Out.Logf(TEXT("class %ls : public %ls\r\n"), C->GetNameCPP(), C->GetSuperClass()->GetNameCPP());
		Out.Log(TEXT("{\r\npublic:\r\n"));
		DumpStructParms(Out, C, 0, 0);

		for (TFieldIterator<UStructProperty> It(C); It; ++It)
		{
			if (It.GetStruct() == C && It->Struct)
				ExpStructs.AddItem(It->Struct);
		}
		for (TFieldIterator<UArrayProperty> It(C); It; ++It)
		{
			if (It.GetStruct() == C && It->Inner && It->Inner->GetClass()==UStructProperty::StaticClass() && ((UStructProperty*)It->Inner)->Struct)
				ExpStructs.AddItem(((UStructProperty*)It->Inner)->Struct);
		}

		// Declare class.
		DWORD Flags = C->ClassFlags;
		Out.Logf(TEXT("\tDECLARE_CLASS(%ls,%ls,"), C->GetNameCPP(), C->GetSuperClass()->GetNameCPP());

		INT bHadFlag = 0;
		FString FlagsStr;
		if (Flags & CLASS_Transient)
		{
			FlagsStr = TEXT("CLASS_Transient");
			++bHadFlag;
		}
		if (Flags & CLASS_Config)
		{
			FlagsStr += FString(bHadFlag ? TEXT(" | CLASS_Config") : TEXT("CLASS_Config"));
			++bHadFlag;
		}
		if (Flags & CLASS_NativeReplication)
		{
			FlagsStr += FString(bHadFlag ? TEXT(" | CLASS_NativeReplication") : TEXT("CLASS_NativeReplication"));
			++bHadFlag;
		}
		if (Flags & CLASS_SafeReplace)
		{
			FlagsStr += FString(bHadFlag ? TEXT(" | CLASS_SafeReplace") : TEXT("CLASS_SafeReplace"));
			++bHadFlag;
		}
		if (Flags & CLASS_Abstract)
		{
			FlagsStr += FString(bHadFlag ? TEXT(" | CLASS_Abstract") : TEXT("CLASS_Abstract"));
			++bHadFlag;
		}
		if (!bHadFlag)
			Out.Log(TEXT("0"));
		else if (bHadFlag == 1)
			Out.Log(*FlagsStr);
		else Out.Logf(TEXT("(%ls)"), *FlagsStr);

		Out.Logf(TEXT(",%ls);\r\n"), C->GetOuter()->GetName());

		Out.Logf(TEXT("\tNO_DEFAULT_CONSTRUCTOR(%ls);\r\n"), C->GetNameCPP());

		// End of class.
		Out.Log(TEXT("};\r\n"));
	}
	else ExpStructs.AddItem(S);

	for (INT i = 0; i < ExpStructs.Num(); ++i)
	{
		Out.Logf(TEXT("struct F%ls\r\n{\r\n"), ExpStructs(i)->GetName());
		DumpStructParms(Out, ExpStructs(i), 0, 0);
		Out.Logf(TEXT("};\r\n"), ExpStructs(i)->GetName());
	}
	appSaveStringToFile(Out, TEXT("Dump.txt"));
	GLog->Log(TEXT("Output to Dump.txt"));
}

const TCHAR* UEditor::GetObscName(INT Index)
{
#if 0
	static TCHAR Result[5];

	INT i;
	for (i = 0; i < 5; ++i)
	{
		Result[i] = (Index & 0x7F) + 1;
		Index = ((Index & 0xFF80) >> 7);
		if (Index == 0)
			break;
	}
	Result[i+1] = 0;
#else
	static TCHAR Result[5];

	INT i, j;
	for (i = 0; i < 5; ++i)
	{
		j = (Index & 0x1F);
		if (j < 10)
			Result[i] = j + '0';
		else Result[i] = j + ('a' - 10);
		Index = ((Index & 0xFFE0) >> 5);
		if (Index == 0)
			break;
	}
	Result[i + 1] = 0;
#endif
	return Result;
}
const TCHAR* UEditor::GetObscNameUnique(UObject* Outer)
{
	for (INT i = 0; i < (1<<16); ++i)
	{
		const TCHAR* Test = GetObscName(i);
		if (!UObject::StaticFindObject(UObject::StaticClass(), Outer, Test))
			return Test;
	}
	return NULL;
}

static void ObfuscateStruct(UStruct* S)
{
	for (TFieldIterator<UProperty> PIt(S, FALSE); PIt; ++PIt)
		PIt->Rename(UEditor::GetObscNameUnique(PIt->GetOuter()));
}
static void MarkStructUsed(TArray<UScriptStruct*>& SA, UScriptStruct* S)
{
	SA.RemoveItem(S);
	for (TFieldIterator<UProperty> PIt(S, FALSE); PIt; ++PIt)
	{
		if (S->GetClass() == UStructProperty::StaticClass())
			MarkStructUsed(SA, ((UStructProperty*)*PIt)->Struct);
	}
}

void UEditor::ObfuscatePck(UPackage* P)
{
	guard(UEditor::ObfuscatePck);
	
	TArray<UScriptStruct*> PendingObsc;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		if (It->IsIn(P) && !It->GetSuperStruct())
			PendingObsc.AddUniqueItem(*It);
	}

	for (TObjectIterator<UStruct> It; It; ++It)
	{
		if (!It->IsIn(P))
			continue;
		
		if (It->IsA(UFunction::StaticClass()))
		{
			UFunction* F = (UFunction*)*It;
			for (TFieldIterator<UProperty> PIt(F, FALSE); PIt; ++PIt)
				PIt->Rename(GetObscNameUnique(PIt->GetOuter()));
			F->FunctionFlags |= FUNC_Private;
			if (F->HasAnyFunctionFlags(FUNC_Final) && !(F->TempCompileFlags & TFFLAG_Stripped))
			{
				F->Rename(GetObscNameUnique(F->GetOuter()));
				F->FriendlyName = F->GetFName();
			}
		}
		else if (It->IsA(UClass::StaticClass()))
		{
			if(It->TempCompileFlags & TFFLAG_Stripped)
				It->Rename(GetObscNameUnique(It->GetOuter()));
			for (TFieldIterator<UProperty> PIt((UClass*)*It, FALSE); PIt; ++PIt)
			{
				if (!(PIt->PropertyFlags & (CPF_Edit | CPF_Localized | CPF_Config | CPF_GlobalConfig)))
					PIt->Rename(GetObscNameUnique(PIt->GetOuter()));
				else if (PIt->GetClass() == UStructProperty::StaticClass())
					MarkStructUsed(PendingObsc, ((UStructProperty*)*PIt)->Struct); // Don't allow to obfuscate edit struct!
			}
		}
	}

	for (INT i = 0; i < PendingObsc.Num(); ++i)
		ObfuscateStruct(PendingObsc(i));
	unguard;
}

BYTE UEditor::FindEnumValue(FName ValueName)
{
	guard(UEditor::FindEnumValue);
	BYTE Result = 255;
	BYTE* iFound = EnumLookup.Find(ValueName);

	if (iFound)
		return *iFound;

	// Try to find an enum with matching name.
	for (TObjectIterator<UEnum> It; It; ++It)
	{
		INT ix = It->Names.FindItemIndex(ValueName);
		if (ix != INDEX_NONE)
		{
			Result = ix;
			break;
		}
	}
	EnumLookup.Set(ValueName, Result);

	return Result;
	unguard;
}
BYTE UEditor::FindEnumValue(const TCHAR* ValueName)
{
	FName N(ValueName, FNAME_Find);
	if (N != NAME_None)
		return FindEnumValue(N);
	return 255;
}
