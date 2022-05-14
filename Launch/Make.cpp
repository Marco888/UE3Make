
#include "UnEditor.h"
#include "UnScrPrecom.h"
#include "UnLinker.h"

inline UBOOL appIsLinebreak(TCHAR c)
{
	//@todo - support for language-specific line break characters
	return c == TEXT('\n');
}

/**
 * Returns the number of braces in the string specified; when an opening brace is encountered, the count is incremented; when a closing
 * brace is encountered, the count is decremented.
 */
static INT GetLineBraceCount(const TCHAR* Str)
{
	check(Str);

	INT Result = 0;
	while (*Str)
	{
		if (*Str == TEXT('{'))
		{
			Result++;
		}
		else if (*Str == TEXT('}'))
		{
			Result--;
		}

		Str++;
	}

	return Result;
}

/**
 * Parses the text specified for a collection of comma-delimited class names, surrounded by parenthesis, using the specified keyword.
 *
 * @param	InputText					pointer to the text buffer to parse; will be advanced past the text that was parsed.
 * @param	GroupName				the group name to parse (i.e. DependsOn, Implements, Inherits, etc.)
 * @param	out_ParsedClassNames	receives the list of class names that were parsed.
 *
 * @return	TRUE if the group name specified was found and entries were added to the list
 */
static UBOOL ParseDependentClassGroup(const TCHAR*& InputText, const TCHAR* const GroupName, TArray<FName>& out_ParsedClassNames)
{
	UBOOL bSuccess = FALSE;
	const TCHAR* Temp = InputText;

	// EndOfClassDeclaration is used to prevent matches in areas not part of the class declaration (i.e. the rest of the .uc file after the class declaration)
	const TCHAR* EndOfClassDeclaration = appStrfind(Temp, TEXT(";"));
	while ((Temp = appStrfind(Temp, GroupName)) != 0 && (EndOfClassDeclaration == NULL || Temp < EndOfClassDeclaration))
	{
		// advance past the DependsOn/Implements keyword
		Temp += appStrlen(GroupName);

		// advance to the opening parenthesis
		ParseNext(&Temp);
		if (*Temp++ != TEXT('('))
		{
			appThrowf(TEXT("Missing opening '(' in %s list"), GroupName);
		}

		// advance to the next word in the list
		ParseNext(&Temp);
		if (*Temp == 0)
		{
			appThrowf(TEXT("End of file encountered while attempting to parse opening '(' in %s list"), GroupName);
		}
		else if (*Temp == TEXT(')'))
		{
			appThrowf(TEXT("Unexpected ')' - missing class name in %s list"), GroupName);
		}

		// this is used to detect when multiple class names have been specified using something other than a comma as the delimiter
		UBOOL bParsingWord = FALSE;
		FString NextWord;
		do
		{
			// if the next character isn't a valid character for a DependsOn/Implements class name, advance to the next valid character
			if (appIsWhitespace(*Temp) || appIsLinebreak(*Temp) || (*Temp == TEXT('\r') && appIsLinebreak(*(Temp + 1))))
			{
				ParseNext(&Temp);

				// if this character is the closing paren., stop here
				if (*Temp == 0 || *Temp == TEXT(')'))
				{
					break;
				}

				// otherwise, the next character must be a comma
				if (bParsingWord && *Temp != TEXT(','))
				{
					appThrowf(TEXT("Missing ',' or closing ')' in %s list"), GroupName);
				}
			}

			// if we've hit a comma, add the current word to the list
			if (*Temp == TEXT(','))
			{
				if (NextWord.Len() == 0)
				{
					appThrowf(TEXT("Unexpected ',' - missing class name in %s list"), GroupName);
				}

				out_ParsedClassNames.AddUniqueItem(*NextWord);
				NextWord = TEXT("");

				bSuccess = TRUE;
				bParsingWord = FALSE;
			}
			else
			{
				bParsingWord = TRUE;
				NextWord += *Temp;
			}

			Temp++;
		} while (*Temp != 0 && *Temp != TEXT(')'));

		if (*Temp == 0)
		{
			appThrowf(TEXT("End of file encountered while attempting to parse closing ')' in %s list"), GroupName);
		}

		ParseNext(&Temp);
		if (*Temp++ != TEXT(')'))
		{
			appThrowf(TEXT("Missing closing ')' in %s expression"), GroupName);
		}
		else if (NextWord.Len() == 0)
		{
			appThrowf(TEXT("Unexpected ')' - missing class name in %s list"), GroupName);
		}

		bSuccess = TRUE;
		out_ParsedClassNames.AddUniqueItem(*NextWord);
		InputText = Temp;
	}

	return bSuccess;
}

static UClass* FactoryCreateText
(
	UObject* InParent,
	FName				Name,
	EObjectFlags		Flags,
	UObject* Context,
	const TCHAR* Type,
	const TCHAR*& Buffer,
	const TCHAR* BufferEnd,
	FFeedbackContext* Warn
)
{
	guard(FactoryCreateText);
	const TCHAR* InBuffer = Buffer;
	FString StrLine, ClassName, BaseClassName;

	// preprocessor not quite ready yet - define syntax will be changing
	FString ProcessedBuffer;
	FString sourceFileName = FString(*Name) + TEXT(".uc");
	//GWarn->Logf(TEXT("Importing class %ls..."), *Name);

	// this must be declared outside the try block, since it's the ContextObject for the output device.
	// If it's declared inside the try block, when an exception is thrown it will go out of scope when we
	// jump to the catch block
	FMacroProcessingFilter Filter(InParent->GetName(), sourceFileName, Warn);
	try
	{
		// if there are no macro invite characters in the file, no need to run it through the preprocessor
		if (appStrchr(Buffer, FMacroProcessingFilter::CALL_MACRO_CHAR) != NULL)
		{
			// uncomment these two lines to have the input buffer stripped of comments during the preprocessing phase
	//		FCommentStrippingFilter CommentStripper(sourceFileName);
	//		FSequencedTextFilter Filter(CommentStripper, Macro, sourceFileName);
			Filter.Process(Buffer, BufferEnd, ProcessedBuffer);

			const TCHAR* Ptr = *ProcessedBuffer; // have to make a copy of the pointer at the beginning of the FString
			Buffer = Ptr;
			BufferEnd = Buffer + ProcessedBuffer.Len();
		}

		const TCHAR* InBuffer = Buffer;
		FString StrLine, ClassName, BaseClassName;

		// Import the script text.
		TArray<FName> DependentOn;
		FStringOutputDevice ScriptText, DefaultPropText, CppText;
		INT CurrentLine = 0;

		// The layer of multi-line comment we are in.
		INT CommentDim = 0;

		// is the parsed class name an interface?
		UBOOL bIsInterface = FALSE;

		while (ParseLine(&Buffer, StrLine, 1))
		{
			CurrentLine++;
			const TCHAR* Str = *StrLine, * Temp;
			UBOOL bProcess = CommentDim <= 0;	// for skipping nested multi-line comments
			INT BraceCount = 0;

			if (bProcess && ParseCommand(&Str, TEXT("cpptext")))
			{
				if (CppText.Len() > 0)
				{
					appThrowf(TEXT("Multiple cpptext definitions"));
				}

				ScriptText.Log(TEXT("// (cpptext)\r\n// (cpptext)\r\n"));
				ParseLine(&Buffer, StrLine, 1);
				Str = *StrLine;

				CurrentLine++;

				//@fixme - line count messed up by this call (what if there are two empty lines before the opening brace?)
				ParseNext(&Str);
				if (*Str != '{')
				{
					appThrowf(TEXT("Missing '{' bracket after cpptext in %ls"), *ClassName);
				}
				else
				{
					BraceCount += GetLineBraceCount(Str);
				}

				// Get cpptext.
				while (ParseLine(&Buffer, StrLine, 1))
				{
					CurrentLine++;
					ScriptText.Log(TEXT("// (cpptext)\r\n"));
					Str = *StrLine;
					BraceCount += GetLineBraceCount(Str);
					if (BraceCount == 0)
					{
						break;
					}

					CppText.Log(*StrLine);
					CppText.Log(TEXT("\r\n"));
				}
			}
			else if (bProcess && ParseCommand(&Str, TEXT("defaultproperties")))
			{
				// Get default properties text.
				while (ParseLine(&Buffer, StrLine, 1))
				{
					CurrentLine++;
					BraceCount += GetLineBraceCount(*StrLine);

					if (StrLine.InStr(TEXT("{")) != INDEX_NONE)
					{
						DefaultPropText.Logf(TEXT("linenumber=%i\r\n"), CurrentLine);
						break;
					}
				}

				while (ParseLine(&Buffer, StrLine, 1))
				{
					CurrentLine++;
					bProcess = CommentDim <= 0;
					INT Pos, EndPos, StrBegin, StrEnd;
					Pos = EndPos = StrBegin = StrEnd = INDEX_NONE;

					UBOOL bEscaped = FALSE;
					for (INT CharPos = 0; CharPos < StrLine.Len(); CharPos++)
					{
						if (bEscaped)
						{
							bEscaped = FALSE;
						}
						else if (StrLine[CharPos] == TEXT('\\'))
						{
							bEscaped = TRUE;
						}
						else if (StrLine[CharPos] == TEXT('\"'))
						{
							if (StrBegin == INDEX_NONE)
							{
								StrBegin = CharPos;
							}
							else
							{
								StrEnd = CharPos;
								break;
							}
						}
					}

					// Stub out the comments, ignoring anything inside literal strings.
					Pos = StrLine.InStr(TEXT("//"));
					if (Pos >= 0)
					{
						if (StrBegin == INDEX_NONE || Pos < StrBegin || Pos > StrEnd)
							StrLine = StrLine.Left(Pos);

						if (StrLine == TEXT(""))
						{
							DefaultPropText.Log(TEXT("\r\n"));
							continue;
						}
					}

					// look for a /* ... */ block, ignoring anything inside literal strings
					Pos = StrLine.InStr(TEXT("/*"));
					EndPos = StrLine.InStr(TEXT("*/"));
					if (Pos >= 0)
					{
						if (StrBegin == INDEX_NONE || Pos < StrBegin || Pos > StrEnd)
						{
							if (EndPos != INDEX_NONE && (EndPos < StrBegin || EndPos > StrEnd))
							{
								StrLine = StrLine.Left(Pos) + StrLine.Mid(EndPos + 2);
								EndPos = INDEX_NONE;
							}
							else
							{
								// either no closing token or the closing token is inside a literal string (which means it doesn't matter)
								StrLine = StrLine.Left(Pos);
								CommentDim++;
							}
						}

						bProcess = CommentDim <= 1;
					}

					if (EndPos >= 0)
					{
						// if the closing token is not inside a string
						if (StrBegin == INDEX_NONE || EndPos < StrBegin || EndPos > StrEnd)
						{
							StrLine = StrLine.Mid(EndPos + 2);
							CommentDim--;
						}

						bProcess = CommentDim <= 0;
					}

					Str = *StrLine;
					ParseNext(&Str);

					BraceCount += GetLineBraceCount(Str);
					if (*Str == '}' && bProcess && BraceCount == 0)
					{
						break;
					}

					if (!bProcess || StrLine == TEXT(""))
					{
						DefaultPropText.Log(TEXT("\r\n"));
						continue;
					}
					DefaultPropText.Log(*StrLine);
					DefaultPropText.Log(TEXT("\r\n"));
				}
			}
			else
			{
				// the script preprocessor will emit #linenumber tokens when necessary, so check for those
				if (ParseCommand(&Str, TEXT("#linenumber")))
				{
					FString LineNumberText;

					// found one, parse the number and reset our CurrentLine (used for logging defaultproperties warnings)
					ParseToken(Str, LineNumberText, FALSE);
					CurrentLine = appAtoi(*LineNumberText);
				}

				// Get script text.
				ScriptText.Log(*StrLine);
				ScriptText.Log(TEXT("\r\n"));

				INT Pos = INDEX_NONE, EndPos = INDEX_NONE, StrBegin = INDEX_NONE, StrEnd = INDEX_NONE;

				UBOOL bEscaped = FALSE;
				for (INT CharPos = 0; CharPos < StrLine.Len(); CharPos++)
				{
					if (bEscaped)
					{
						bEscaped = FALSE;
					}
					else if (StrLine[CharPos] == TEXT('\\'))
					{
						bEscaped = TRUE;
					}
					else if (StrLine[CharPos] == TEXT('\"'))
					{
						if (StrBegin == INDEX_NONE)
						{
							StrBegin = CharPos;
						}
						else
						{
							StrEnd = CharPos;
							break;
						}
					}
				}

				// Stub out the comments, ignoring anything inside literal strings.
				Pos = StrLine.InStr(TEXT("//"));
				if (Pos >= 0)
				{
					if (StrBegin == INDEX_NONE || Pos < StrBegin || Pos > StrEnd)
						StrLine = StrLine.Left(Pos);

					if (StrLine == TEXT(""))
						continue;
				}

				// look for a / * ... * / block, ignoring anything inside literal strings
				Pos = StrLine.InStr(TEXT("/*"));
				EndPos = StrLine.InStr(TEXT("*/"));
				if (Pos >= 0)
				{
					if (StrBegin == INDEX_NONE || Pos < StrBegin || Pos > StrEnd)
					{
						if (EndPos != INDEX_NONE && (EndPos < StrBegin || EndPos > StrEnd))
						{
							StrLine = StrLine.Left(Pos) + StrLine.Mid(EndPos + 2);
							EndPos = INDEX_NONE;
						}
						else
						{
							StrLine = StrLine.Left(Pos);
							CommentDim++;
						}
					}
					bProcess = CommentDim <= 1;
				}
				if (EndPos >= 0)
				{
					if (StrBegin == INDEX_NONE || EndPos < StrBegin || EndPos > StrEnd)
					{
						StrLine = StrLine.Mid(EndPos + 2);
						CommentDim--;
					}

					bProcess = CommentDim <= 0;
				}

				if (!bProcess || StrLine == TEXT(""))
					continue;

				Str = *StrLine;

				// Get class or interface name
				if (ClassName == TEXT(""))
				{
					if ((Temp = appStrfind(Str, TEXT("class"))) != 0)
					{
						Temp += appStrlen(TEXT("class")) + 1; // plus at least one delimitor
						ParseToken(Temp, ClassName, 0);
					}
					else if ((Temp = appStrfind(Str, TEXT("interface"))) != 0)
					{
						bIsInterface = TRUE;

						Temp += appStrlen(TEXT("interface")) + 1; // plus at least one delimiter
						ParseToken(Temp, ClassName, 0); // space delimited

						// strip off the trailing ';' characters
						while (ClassName.Right(1) == TEXT(";"))
						{
							ClassName = ClassName.LeftChop(1);
						}
					}
				}

				if
					(BaseClassName == TEXT("")
						&& ClassName != TEXT("Object")
						&& ClassName != TEXT("Interface")
						&& (Temp = appStrfind(Str, TEXT("extends"))) != 0)
				{
					Temp += 7;
					ParseToken(Temp, BaseClassName, 0);
					while (BaseClassName.Right(1) == TEXT(";"))
						BaseClassName = BaseClassName.LeftChop(1);
				}

				// check for classes which this class depends on and add them to the DependentOn List
				if (appStrfind(Str, TEXT("DependsOn")) != NULL)
				{
					const TCHAR* PreviousBuffer = Buffer - StrLine.Len() - 2;	// add 2 for the CRLF
					// PreviousBuffer will only be greater than Buffer if the class names were spanned across more than one line
					if (ParseDependentClassGroup(PreviousBuffer, TEXT("DependsOn"), DependentOn) && PreviousBuffer > Buffer)
					{
						// now copy the text that was parsed into an string so that we can add it to the ScriptText buffer
						INT CharactersProcessed = PreviousBuffer - Buffer;
						FString ProcessedCharacters(CharactersProcessed, Buffer);
						ScriptText.Log(*ProcessedCharacters);

						Buffer = PreviousBuffer;
					}
				}
				// only non-interface classes can have 'implements' keyword
				else if (!bIsInterface && appStrfind(Str, TEXT("Implements")) != NULL)
				{
					const TCHAR* PreviousBuffer = Buffer - StrLine.Len() - 2;	// add 2 for the CRLF
					// PreviousBuffer will only be greater than Buffer if the class names were spanned across more than one line
					if (ParseDependentClassGroup(PreviousBuffer, TEXT("Implements"), DependentOn) && PreviousBuffer > Buffer)
					{
						// now move the text that was parsed into a temporary buffer so that we can feed it to the script text output archive
						// now copy the text that was parsed into an string so that we can add it to the ScriptText buffer
						INT CharactersProcessed = PreviousBuffer - Buffer;
						FString ProcessedCharacters(CharactersProcessed, Buffer);
						ScriptText.Log(*ProcessedCharacters);

						Buffer = PreviousBuffer;
					}
				}
			}
		}

		// a base interface implicitly inherits from class 'Interface', unless it is the 'Interface' class itself
		if (bIsInterface == TRUE && (BaseClassName.Len() == 0 || BaseClassName == TEXT("Object")))
		{
			if (ClassName != TEXT("Interface"))
			{
				BaseClassName = TEXT("Interface");
			}
			else
			{
				BaseClassName = TEXT("Object");
			}
		}

		debugfSlow(TEXT("Class: %s extends %s"), *ClassName, *BaseClassName);

		// Handle failure.
		if (ClassName == TEXT("") || (BaseClassName == TEXT("") && ClassName != TEXT("Object")))
		{
			Warn->Logf(NAME_Error,
				TEXT("Bad class definition '%s'/'%s'/%i/%i"), *ClassName, *BaseClassName, BufferEnd - InBuffer, appStrlen(InBuffer));
			return NULL;
		}
		else if (ClassName == BaseClassName)
		{
			Warn->Logf(NAME_Error, TEXT("Class is extending itself '%s'"), *ClassName);
			return NULL;
		}
		else if (ClassName != *Name)
		{
			Warn->Logf(NAME_Error, TEXT("Script vs. class name mismatch (%s/%s)"), *Name, *ClassName);
		}

		UClass* ResultClass = FindObject<UClass>(InParent, *ClassName);

		// if we aren't generating headers, then we shouldn't set misaligned object, since it won't get cleared
		const UBOOL bSkipNativeHeaderGeneration = TRUE;

		const static UBOOL bVerboseOutput = ParseParam(appCmdLine(), TEXT("VERBOSE"));
		if (ResultClass && ResultClass->HasAnyFlags(RF_Native))
		{
			if (!bSkipNativeHeaderGeneration)
			{
				// Gracefully update an existing hardcoded class.
				if (bVerboseOutput)
				{
					debugf(NAME_Log, TEXT("Updated native class '%s'"), *ResultClass->GetFullName());
				}

				// assume that the property layout for this native class is going to be modified, and
				// set the RF_MisalignedObject flag to prevent classes of this type from being created
				// - when the header is generated for this class, we'll unset the flag once we verify
				// that the property layout hasn't been changed

				if (!ResultClass->HasAnyClassFlags(CLASS_NoExport))
				{
					if (!bIsInterface && ResultClass != UObject::StaticClass() && !ResultClass->HasAnyFlags(RF_MisalignedObject))
					{
						ResultClass->SetFlags(RF_MisalignedObject);

						// propagate to all children currently in memory, ignoring the object class
						for (TObjectIterator<UClass> It; It; ++It)
						{
							if (It->GetSuperClass() == ResultClass && !It->HasAnyClassFlags(CLASS_NoExport))
							{
								It->SetFlags(RF_MisalignedObject);
							}
						}
					}
				}
			}

			UClass* SuperClass = ResultClass->GetSuperClass();
			if (SuperClass && BaseClassName != SuperClass->GetName())
			{
				// the code that handles the DependsOn list in the script compiler doesn't work correctly if we manually add the Object class to a class's DependsOn list
				// if Object is also the class's parent.  The only way this can happen (since specifying a parent class in a DependsOn statement is a compiler error) is
				// in this block of code, so just handle that here rather than trying to make the script compiler handle this gracefully
				if (BaseClassName != TEXT("Object"))
				{
					// we're changing the parent of a native class, which may result in the
					// child class being parsed before the new parent class, so add the new
					// parent class to this class's DependsOn() array to guarantee that it
					// will be parsed before this class
					DependentOn.AddUniqueItem(*BaseClassName);
				}

				// if the new parent class is an existing native class, attempt to change the parent for this class to the new class 
				UClass* NewSuperClass = FindObject<UClass>(ANY_PACKAGE, *BaseClassName);
				if (NewSuperClass != NULL)
				{
					ResultClass->SuperStruct = NewSuperClass;
				}
			}
		}
		else
		{
			// detect if the same class name is used in multiple packages
			if (ResultClass == NULL)
			{
				UClass* ConflictingClass = FindObject<UClass>(ANY_PACKAGE, *ClassName, TRUE);
				if (ConflictingClass != NULL)
				{
					Warn->Logf(NAME_Warning, TEXT("Duplicate class name: %s also exists in package %s"), *ClassName, ConflictingClass->GetOutermost()->GetName());
				}
			}

			// Create new class.
			ResultClass = new(InParent, *ClassName, Flags)UClass(NULL);

			// add CLASS_Interface flag if the class is an interface
			// NOTE: at this pre-parsing/importing stage, we cannot know if our super class is an interface or not,
			// we leave the validation to the script compiler
			if (bIsInterface == TRUE)
			{
				ResultClass->ClassFlags |= CLASS_Interface;
			}

			// Find or forward-declare base class.
			ResultClass->SuperStruct = FindObject<UClass>(InParent, *BaseClassName);
			if (ResultClass->SuperStruct == NULL)
			{
				//@todo ronp - do we really want to do this?  seems like it would allow you to extend from a base in a dependent package.
				ResultClass->SuperStruct = FindObject<UClass>(ANY_PACKAGE, *BaseClassName);
			}

			if (ResultClass->SuperStruct == NULL)
			{
				// don't know its parent class yet
				ResultClass->SuperStruct = new(InParent, *BaseClassName) UClass(NULL);
			}
			else if (!bIsInterface)
			{
				// if the parent is misaligned, then so are we
				ResultClass->SetFlags(ResultClass->SuperStruct->GetFlags() & RF_MisalignedObject);
			}

			if (bVerboseOutput)
			{
				debugf(NAME_Log, TEXT("Imported: %s"), *ResultClass->GetFullName());
			}
		}

		// Set class info.
		ResultClass->ScriptText = new UTextBuffer(*ScriptText);
		ResultClass->DefaultPropText = DefaultPropText;
		ResultClass->DependentOn = DependentOn;

		if (bVerboseOutput)
		{
			for (INT DependsIndex = 0; DependsIndex < DependentOn.Num(); DependsIndex++)
			{
				debugf(TEXT("\tAdding %s as a dependency"), *DependentOn(DependsIndex));
			}
		}
		if (CppText.Len())
		{
			ResultClass->CppText = new UTextBuffer(*CppText);
		}
		return ResultClass;
	}
	catch (const TCHAR* ErrorMsg)
	{
		// Catch and log any warnings
		Warn->Log(NAME_Error, ErrorMsg);
		GWarn->Logf(NAME_Error, TEXT("Failed to import class %ls"), *ClassName);
		return NULL;
	}
	catch (...)
	{
		throw;
	}
#if 0
	// Import the script text.
	FStringOutputDevice ScriptText, DefaultPropText;
	TArray<FName> DependentOn; //amb,gam
	UBOOL InMComment = false; //elmuerte: to get rid of multiline comments
	while (ParseLine(&Buffer, StrLine, 1))
	{
		const TCHAR* Str = *StrLine, * Temp;
		if (ParseCommand(&Str, TEXT("defaultproperties")))
		{
			// Get default properties text.
			while (ParseLine(&Buffer, StrLine, 1))
			{
				Str = *StrLine;
				ParseNext(&Str);
				if (*Str == '}')
					break;
				DefaultPropText.Logf(TEXT("%ls\r\n"), *StrLine);
			}
		}
		else
		{
			INT Pos;
			// Get script text.
			ScriptText.Logf(TEXT("%ls\r\n"), *StrLine);

			//elmuerte: inside a /* ... */
			if (InMComment)
			{
				Pos = StrLine.InStr(TEXT("*/"));
				if (Pos >= 0)
				{
					StrLine = StrLine.Right(Pos + 2);
					InMComment = false;
				}
				else continue; //elmuerte: still inside a multiline comment
			}

			// Stub out the comments.
			Pos = StrLine.InStr(TEXT("//"));
			if (Pos >= 0)
				StrLine = StrLine.Left(Pos);

			//elmuerte: Stub out the multi line comments.
			Pos = StrLine.InStr(TEXT("/*"));
			if (Pos >= 0)
			{
				StrLine = StrLine.Left(Pos);
				InMComment = true;
			}

			Str = *StrLine;

			// Get class name.
			if (ClassName == TEXT("") && (Temp = appStrfind(Str, TEXT("class"))) != 0)
			{
				Temp += 6;
				ParseToken(Temp, ClassName, 0);
			}
			if
				(BaseClassName == TEXT("")
					&& ((Temp = appStrfind(Str, TEXT("expands"))) != 0 || (Temp = appStrfind(Str, TEXT("extends"))) != 0))
			{
				Temp += 7;
				ParseToken(Temp, BaseClassName, 0);
				while (BaseClassName.Right(1) == TEXT(";"))
					BaseClassName = BaseClassName.LeftChop(1);
			}

			Temp = Str;
			while ((Temp = appStrfind(Temp, TEXT("dependson("))) != 0)
			{
				Temp += 10;
				TCHAR s[NAME_SIZE];
				INT i = 0;
				while (*Temp != ')' && i < NAME_SIZE)
					s[i++] = *Temp++;
				s[i] = 0;
				FName dependsOnName(s);
				DependentOn.AddUniqueItem(dependsOnName);
			}
		}
	}

	// Handle failure.
	if (ClassName == TEXT("") || (BaseClassName == TEXT("") && ClassName != TEXT("Object")))
	{
		Warn->Logf(NAME_Error, TEXT("Bad class definition '%ls'/'%ls'/%i/%i"), *ClassName, *BaseClassName, BufferEnd - InBuffer, appStrlen(InBuffer)); // gam
		return NULL;
	}
	else if (ClassName != *Name)
	{
		Warn->Logf(NAME_Error, TEXT("Script vs. class name mismatch (%ls/%ls)"), *Name, *ClassName); // gam
	}

	UClass* ResultClass = FindObject<UClass>(InParent, *ClassName);
	if (ResultClass && (ResultClass->GetFlags() & RF_Native))
	{
		// Gracefully update an existing hardcoded class.
		debugf(NAME_Log, TEXT("Updated native class '%ls'"), ResultClass->GetFullName());
		ResultClass->ClassFlags &= ~(CLASS_Parsed | CLASS_Compiled);
	}
	else
	{
		// Create new class.
		ResultClass = new(InParent, *ClassName, Flags)UClass(NULL);

		// Find or forward-declare base class.
		ResultClass->SuperStruct = FindObject<UClass>(InParent, *BaseClassName);
		if (!ResultClass->SuperStruct)
			ResultClass->SuperStruct = FindObject<UClass>(ANY_PACKAGE, *BaseClassName);
		if (!ResultClass->SuperStruct)
			ResultClass->SuperStruct = new(InParent, *BaseClassName)UClass(ResultClass);
		debugf(NAME_Log, TEXT("Imported: %ls"), ResultClass->GetFullName());
	}

	// Set class info.
	ResultClass->ScriptText = new(ResultClass, TEXT("ScriptText"), RF_Transient)UTextBuffer(*ScriptText);
	ResultClass->DefaultPropText = DefaultPropText;
	ResultClass->DependentOn = DependentOn;

	return ResultClass;
#endif
	unguard;
}

static void DEBUG_PrintProperties(UStruct* S)
{
	if (S->GetSuperStruct())
		DEBUG_PrintProperties(S->GetSuperStruct());

	for (UField* F = S->Children; F; F = F->Next)
	{
		if (F->IsA(UProperty::StaticClass()))
			debugf(TEXT("-- %i\t%p\t%ls"), ((UProperty*)F)->Offset, F, F->GetFullName());
	}
}
static void DEBUG_PrintValues(UObject* Obj)
{
	UProperty* P = NULL;
	guard(DEBUG_PrintValues);
	debugf(TEXT("==== ExportProperties %ls"), Obj->GetFullName());
	BYTE* Data = Obj->GetScriptData();
	if (!Data)
		return;
	FString V;
	INT i;
	for (TFieldIterator<UProperty> It(Obj->GetClass()); It; ++It)
	{
		P = *It;
		if (P->ArrayDim == 1)
		{
			V.Empty();
			P->ExportTextItem(V, Data + P->Offset, NULL, 0);
			debugf(TEXT("   -%ls = '%ls' (%p)"), P->GetName(), *V, Data + P->Offset);
		}
		else
		{
			for (i = 0; i < P->ArrayDim; ++i)
			{
				V.Empty();
				P->ExportTextItem(V, Data + P->Offset + (i*P->ElementSize), NULL, 0);
				debugf(TEXT("   -%ls[%i] = '%ls' (%p)"), P->GetName(), i, *V, Data + P->Offset + (i * P->ElementSize));
			}
		}
	}
	unguardf((TEXT("(%ls)"),P->GetFullName()));
}

static INT CDECL QFNCompare(const FString& A, const FString& B)
{
	return appStricmp(*A, *B);
}

void MakeMain()
{
	guard(MakeMain);
	UEditor::InitEditor();
	UEditor::InitDebugger();

	if (!PrepareAssetCache())
	{
		return;
	}

	GWarn->Log(NAME_Title, TEXT("Loading packages..."));
	TArray<FName> BuiltPckList;

	// Load classes for editing.
	UBOOL Success = TRUE;

	// Load LoadPackages
	guard(LoadEditPackages);
	for( INT i=(GEditor->LoadPackages.Num()-1); i>=0; --i )
	{
		GWarn->Logf(NAME_Heading,TEXT("Loading %s"),*GEditor->LoadPackages(i));
		GUglyHackFlags |= 2;
		UObject* P = UObject::LoadPackage(NULL, *GEditor->LoadPackages(i), LOAD_Forgiving);
		GUglyHackFlags &= ~2;
		if( !P )
			appErrorf(TEXT("Couldn't load package %s"),*GEditor->LoadPackages(i));
		else
		{
			FMacroProcessingFilter DummyMacroFilter(P->GetName(), TEXT("NULL"));
			BuiltPckList.AddItem(P->GetFName());
		}
	}
	unguard;

	guard(DumpHeader);
	if (GEditor->DumpClass.Len())
	{
		UStruct* S = FindObject<UStruct>(ANY_PACKAGE, *GEditor->DumpClass, 0);
		debugf(TEXT("Attempt dump class '%ls': %ls"), *GEditor->DumpClass, S->GetFullName());
		if (S)
			GEditor->ExportHeader(S);
	}
	unguard;

	// Compile packages.
	if (GEditor->EditPackages.Num())
	{
		guard(BuildingCode);
		INT j;
		GWarn->Log(NAME_Heading, TEXT("Compiling packages..."));
		for (INT i = (GEditor->EditPackages.Num() - 1); i >= 0; --i)
		{
			const FString& Pkg = GEditor->EditPackages(i);
			GWarn->Logf(NAME_Title, TEXT("Compiling %ls"), *Pkg);
			GWarn->Logf(NAME_Heading, TEXT("Compile %ls"), *Pkg);

			// Create package.
			GWarn->Log(TEXT("Analyzing..."));
			UPackage* PkgObject = UObject::CreatePackage(NULL, *Pkg);

			// Try reading from package's .ini file.
			PkgObject->PackageFlags = (PKG_ContainsScript | PKG_StrippedSource);
			FString IniName = GEditor->EditPackagesInPath * Pkg * Pkg + TEXT(".upkg");
			if(GFileManager->FileSize(*IniName)==INDEX_NONE)
				IniName = GEditor->EditPackagesInPath * Pkg * TEXT("Classes") * Pkg + TEXT(".upkg");
			UBOOL B = 0;
			if (!GConfig->GetBool(TEXT("Flags"), TEXT("AllowDownload"), B, *IniName) || B)
				PkgObject->PackageFlags |= PKG_AllowDownload;
			if (GConfig->GetBool(TEXT("Flags"), TEXT("ClientOptional"), B, *IniName) && B)
				PkgObject->PackageFlags |= PKG_ClientOptional;
			if (GConfig->GetBool(TEXT("Flags"), TEXT("ServerSideOnly"), B, *IniName) && B)
				PkgObject->PackageFlags |= PKG_ServerSideOnly;
			TMultiMap<FString, FString>* LP = GConfig->GetSectionPrivate(TEXT("Load"), 0, 1, *IniName);
			if (LP)
			{
				TArray<FString> Pcks;
				LP->MultiFind(TEXT("Need"), Pcks);
				for (j = (Pcks.Num() - 1); j >= 0; --j)
				{
					FName N(*Pcks(j));
					if (BuiltPckList.FindItemIndex(N) == INDEX_NONE)
					{
						GWarn->Logf(TEXT("Loading needed reference %ls..."), *N);
						GUglyHackFlags |= 2;
						UObject* P = UObject::LoadPackage(NULL, *N, LOAD_Forgiving);
						GUglyHackFlags &= ~2;
						if (!P)
							GWarn->Logf(TEXT("Couldn't load package %s"), *N);
						else
						{
							FMacroProcessingFilter DummyMacroFilter(P->GetName(), TEXT("NULL"));
							BuiltPckList.AddItem(P->GetFName());
						}
					}
				}
				Pcks.Empty();
				LP->MultiFind(TEXT("Include"), Pcks);
				for (j = (Pcks.Num() - 1); j >= 0; --j)
				{
					FName N(*Pcks(j));
					UObject* P = NULL;
					if (BuiltPckList.FindItemIndex(N) == INDEX_NONE)
					{
						GWarn->Logf(TEXT("Loading merging reference %ls..."), *Pcks(j));
						GUglyHackFlags |= 2;
						UObject* P = UObject::LoadPackage(NULL, *N, LOAD_NoFail);
						GUglyHackFlags &= ~2;
					}
					else
					{
						GWarn->Logf(TEXT("Merging package %ls..."), *Pcks(j));
						P = FindObject<UPackage>(NULL, *N);
					}
					if (!P)
						appErrorf(TEXT("Couldn't load package %s"), *Pcks(j));
					else
					{
						FMacroProcessingFilter DummyMacroFilter(P->GetName(), TEXT("NULL"));
						BuiltPckList.AddItem(P->GetFName());
						debugf(TEXT("Merging %ls with %ls"), P->GetName(), PkgObject->GetName());

						for (FObjectIterator It(UObject::StaticClass()); It; ++It)
							if (It->IsIn(P) && !It->IsA(ULinker::StaticClass()))
								It->Rename(It->GetName(), PkgObject);
					}
				}
			}

			// Rebuild the class from its directory.
			FString Spec = GEditor->EditPackagesInPath * Pkg * TEXT("Classes") * TEXT("*.uc");
			TArray<FString> Files = GFileManager->FindFiles(*Spec, 1, 0);

			if (Files.Num() == 0)
			{
				GWarn->Logf(TEXT("Can't find files matching %ls"), *Spec);
				appErrorf(TEXT("Can't find files matching %ls"), *Spec);
			}

			appQsort(&Files(0), Files.Num(), sizeof(FString), (QSORT_COMPARE)QFNCompare);
			TArray<UClass*> AllClasses;
			for (j = 0; j < Files.Num(); ++j)
			{
				// Load classes for editing.
				FString Filename = GEditor->EditPackagesInPath * Pkg * TEXT("Classes") * Files(j);
				FString ClassName = Files(j).GetFilenameOnly();
				FString FileContents;
				if (appLoadFileToString(FileContents, *Filename))
				{
					const TCHAR* S = *FileContents;
					const TCHAR* End = &(*FileContents)[FileContents.Len()];
					UClass* C = FactoryCreateText(PkgObject, *ClassName, (RF_Public | RF_Standalone), NULL, TEXT("uc"), S, End, GWarn);
					if (C)
						AllClasses.AddItem(C);
				}
				else GWarn->Logf(TEXT("Failed to load file %ls!"), *Filename);
			}

			// Verify that all script declared superclasses exist.
			for (j = 0; j < AllClasses.Num(); ++j)
			{
				UClass* TClass = AllClasses(j);
				UClass* SClass = TClass->GetSuperClass();
				if (!SClass || AllClasses.FindItemIndex(SClass) == INDEX_NONE)
					continue;
				if (TClass->ScriptText && !SClass->ScriptText)
					appErrorf(TEXT("Superclass %ls of %ls not found"), SClass->GetFullName(), TClass->GetFullName());
			}

			// Bootstrap-recompile changed scripts.
			GEditor->Bootstrapping = 1;
			if (!GEditor->MakeScripts(GWarn, PkgObject, AllClasses))
			{
				GWarn->Log(NAME_Title, TEXT("Failed due to errors!"));
				return;
			}
			GEditor->Bootstrapping = 0;

			// Save package.
			ULinkerLoad* Conform = NULL;
			FString SeekFile(GEditor->EditPackagesInPath * Pkg + TEXT(".u"));
			guard(LoadConform);
			if (GFileManager->FileSize(*SeekFile) > 0)
			{
				UObject::BeginLoad();
				Conform = UObject::GetPackageLinker(UObject::CreatePackage(NULL, *(US + Pkg + TEXT("_OLD"))), *SeekFile, LOAD_NoWarn | LOAD_NoVerify);
				UObject::EndLoad();
			}
			unguard;
			if (Conform)
				debugf(TEXT("Conforming: %ls"), *Pkg);

			/*for (TObjectIterator<UClass> CIt; CIt; ++CIt)
			{
				if (!CIt->IsIn(PkgObject))
					continue;
				debugf(TEXT("%ls ================================"), CIt->GetFullName());
				DEBUG_PrintProperties(*CIt);
			}*/
			//DEBUG_PrintValues(FindObject<UObject>(NULL, TEXT("Engine.Default__Info.Sprite")));
			if (GEditor->bShouldObfuscate)
				GEditor->ObfuscatePck(PkgObject);
			UObject::SavePackage(PkgObject, NULL, RF_Standalone, *(GEditor->EditPackagesOutPath * Pkg + TEXT(".u")), GError, Conform, 1);
			BuiltPckList.AddItem(PkgObject->GetFName());
		}
		GWarn->Log(NAME_Heading, TEXT("Compiling finished!"));
		unguard;
	}
	GWarn->Log(NAME_Title, TEXT("Completed!"));

	if(FMacroProcessingFilter::GlobalSymbols)
		delete FMacroProcessingFilter::GlobalSymbols;
	FMacroProcessingFilter::GlobalSymbols = NULL;
	unguard;
}

class FDefaultPropertiesContextSupplier : public FContextSupplier
{
public:
	/** the current line number */
	INT CurrentLine;

	/** the package we're processing */
	FString PackageName;

	/** the class we're processing */
	FString ClassName;

	FString GetContext()
	{
		return FString::Printf
		(
			TEXT("%s\\%s\\Classes\\%s.uc(%i)"),
			*GEditor->EditPackagesInPath,
			*PackageName,
			*ClassName,
			CurrentLine
		);
	}

	FDefaultPropertiesContextSupplier() {}
	FDefaultPropertiesContextSupplier(const TCHAR* Package, const TCHAR* Class, INT StartingLine)
		: PackageName(Package), ClassName(Class), CurrentLine(StartingLine)
	{
	}
};
static FDefaultPropertiesContextSupplier* ContextSupplier = NULL;

inline UBOOL GetBEGIN(const TCHAR** Stream, const TCHAR* Match)
{
	const TCHAR* Original = *Stream;
	if (ParseCommand(Stream, TEXT("BEGIN")) && ParseCommand(Stream, Match))
		return TRUE;
	*Stream = Original;
	return FALSE;
}
inline UBOOL GetEND(const TCHAR** Stream, const TCHAR* Match)
{
	const TCHAR* Original = *Stream;
	if (ParseCommand(Stream, TEXT("END")) && ParseCommand(Stream, Match)) return TRUE; // Gotten.
	*Stream = Original;
	return FALSE;
}
static FString ReadStructValue(const TCHAR* pStr, const TCHAR*& SourceTextBuffer)
{
	if (*pStr == '{')
	{
		INT bracketCnt = 0;
		UBOOL bInString = FALSE;
		FString Result;
		FString CurrentLine;

		// increment through each character until we run out of brackets
		do
		{
			// check to see if we've reached the end of this line
			if (*pStr == '\0' || *pStr == NULL)
			{
				// parse a new line
				if (ParseLine(&SourceTextBuffer, CurrentLine, TRUE))
				{
					if (ContextSupplier != NULL)
					{
						ContextSupplier->CurrentLine++;
					}

					pStr = *CurrentLine;
				}
				else
				{
					// no more to parse
					break;
				}
			}

			if (*pStr != NULL && *pStr != '\0')
			{
				TCHAR Char = *pStr;

				// if we reach a bracket, increment the count
				if (Char == '{')
				{
					bracketCnt++;
				}
				// if we reach and end bracket, decrement the count
				else if (Char == '}')
				{
					bracketCnt--;
				}
				else if (Char == '\"')
				{
					// flip the "we are in a string" switch
					bInString = !bInString;
				}
				else if (Char == TEXT('\n'))
				{
					if (ContextSupplier != NULL)
					{
						ContextSupplier->CurrentLine++;
					}
				}

				// add this character to our end result
				// if we're currently inside a string value, allow the space character
				if (bInString ||
					(Char != '\n' && Char != '\r' && !appIsWhitespace(Char)
						&& Char != '{' && Char != '}'))
				{
					Result += Char;
				}

				pStr++;
			}
		} while (bracketCnt != 0);


		return Result;
	}

	return pStr;
}
static const INT ReadArrayIndex(UStruct* ObjectStruct, UClass* TopOuter, const TCHAR*& Str, FFeedbackContext* Warn)
{
	const TCHAR* Start = Str;
	INT Index = INDEX_NONE;
	SkipWhitespace(Str);

	if (*Str == '(' || *Str == '[')
	{
		Str++;
		FString IndexText(TEXT(""));
		while (*Str && *Str != ')' && *Str != ']')
		{
			if (*Str == TCHAR('='))
			{
				// we've encountered an equals sign before the closing bracket
				Warn->Logf(NAME_Warning, TEXT("Missing ')' in default properties subscript: %s"), Start);
				return 0;
			}

			IndexText += *Str++;
		}

		if (*Str++)
		{
			if (IndexText.Len() > 0)
			{
				if (appIsAlpha(IndexText[0]))
				{
					FName IndexTokenName = FName(*IndexText, FNAME_Find);
					if (IndexTokenName != NAME_None)
					{
						// Search for the enum in question.
						// The make commandlet keeps track of this information via a map, so use the shortcut in this case.
						BYTE eValue = GEditor->FindEnumValue(IndexTokenName);

						if (eValue == 255)
						{
							// search for const ref
							UConst* Const = FindField<UConst>(ObjectStruct, *IndexText);
							if (Const == NULL && TopOuter != NULL)
							{
								Const = FindField<UConst>(TopOuter, *IndexText);
							}

							if (Const != NULL)
							{
								Index = appAtoi(*Const->Value);
							}
							else
							{
								Index = 0;
								Warn->Logf(NAME_Warning, TEXT("Invalid subscript in default properties: %s"), Start);
							}
						}
						else Index = eValue;
					}
					else
					{
						Index = 0;

						// unknown or invalid identifier specified for array subscript
						Warn->Logf(NAME_Warning, TEXT("Invalid subscript in default properties: %s"), Start);
					}
				}
				else
				{
					Index = appAtoi(*IndexText);
				}
			}
			else
			{
				Index = 0;

				// nothing was specified between the opening and closing parenthesis
				Warn->Logf(NAME_Warning, TEXT("Invalid subscript in default properties: %s"), Start);
			}
		}
		else
		{
			Index = 0;
			Warn->Logf(NAME_Warning, TEXT("Missing ')' in default properties subscript: %s"), Start);
		}
	}
	return Index;
}
struct FDefinedProperty
{
	UProperty* Property;
	INT Index;
	inline bool operator== (const FDefinedProperty& Other) const
	{
		return((Property == Other.Property) && (Index == Other.Index));
	}
};

struct FObjectRefFinder : public FArchive
{
private:
	UClass* ClassType;
	FName ObjName;
	TArray<UObject*> SeekList;

public:
	UObject* ResultObject;

	FObjectRefFinder(UClass* T, FName N)
		: ClassType(T), ObjName(N), ResultObject(NULL)
	{
	}
	void SeekForObject(UObject* Obj)
	{
		SeekList.AddItem(Obj);
		for (INT i = 0; (i < SeekList.Num() && !ResultObject); ++i)
		{
			SeekList(i)->Serialize(*this);
		}
		SeekList.Empty();
	}
	FArchive& operator<<(class UObject*& Res)
	{
		if (!ResultObject && Res && Res->IsValid() && !Res->IsA(UField::StaticClass()))
		{
			if (Res->GetFName() == ObjName && (!ClassType || Res->IsA(ClassType)))
				ResultObject = Res;
			else SeekList.AddUniqueItem(Res);
		}
		return *this;
	}
};

struct FReplaceRefs : public FArchive
{
public:
	TMap<UObject*, UObject*> ReplaceMap;

	FReplaceRefs()
	{
	}
	void ReplaceRefs(UObject* Obj)
	{
		if (ReplaceMap.Num())
			Obj->Serialize(*this);
	}
	FArchive& operator<<(class UObject*& Res)
	{
		if (Res)
		{
			UObject* NewRef = ReplaceMap.FindRef(Res);
			if (NewRef)
				Res = NewRef;
		}
		return *this;
	}
};

static const TCHAR* ImportProperties(
	BYTE* DestData,
	const TCHAR* SourceText,
	UStruct* ObjectStruct,
	UObject* SubobjectRoot,
	UObject* SubobjectOuter,
	TMap<FName, UComponent*>& ComponentNameToInstanceMap,
	TArray<UComponent*>* RemovedComponents,
	FFeedbackContext* Warn,
	INT							Depth
)
{
	guard(ImportProperties);
	check(ObjectStruct != NULL);
	check(DestData != NULL);

	if (SourceText == NULL)
		return NULL;

	// Cannot create subobjects when importing struct defaults, or if SubobjectOuter (used as the Outer for any subobject declarations encountered) is NULL
	UBOOL bSubObjectsAllowed = !ObjectStruct->IsA(UScriptStruct::StaticClass()) && SubobjectOuter != NULL;

	// TRUE when DestData corresponds to a subobject in a class default object
	UBOOL bSubObject = FALSE;

	UClass* ComponentOwnerClass = NULL;

	if (bSubObjectsAllowed)
	{
		bSubObject = SubobjectRoot != NULL && SubobjectRoot->HasAnyFlags(RF_ClassDefaultObject);
		if (SubobjectRoot == NULL)
		{
			SubobjectRoot = SubobjectOuter;
		}

		ComponentOwnerClass = SubobjectOuter != NULL
			? SubobjectOuter->IsA(UClass::StaticClass())
			? CastChecked<UClass>(SubobjectOuter)
			: SubobjectOuter->GetClass()
			: NULL;
	}


	// The PortFlags to use for all ImportText calls
	DWORD PortFlags = PPF_Delimited | PPF_CheckReferences | PPF_RestrictImportTypes | PPF_ParsingDefaultProperties | PPF_ExecImport | PPF_Exec;

	FString StrLine;

	// If bootstrapping, check the class we're BEGIN OBJECTing has had its properties imported.
	if (Depth == 0)
	{
		const TCHAR* TempData = SourceText;
		while (ParseLine(&TempData, StrLine))
		{
			const TCHAR* Str = *StrLine;
			if (GetBEGIN(&Str, TEXT("Object")))
			{
				UClass* TemplateClass;
				if (ParseObject<UClass>(Str, TEXT("Class="), TemplateClass, ANY_PACKAGE))
				{
					if (!UEditor::ImportDefaultProps(TemplateClass))
						return NULL;
				}
			}
		}
	}

	GLog->Logf(TEXT("Importing defaultproperties of %ls"), ObjectStruct->GetFullName());

	TArray<FDefinedProperty> DefinedProperties;
	FReplaceRefs ReplaceSer;

	// Parse all objects stored in the actor.
	// Build list of all text properties.
	UBOOL ImportedBrush = 0;
	while (ParseLine(&SourceText, StrLine, TRUE))
	{
		const TCHAR* Str = *StrLine;

		if (ContextSupplier != NULL)
		{
			ContextSupplier->CurrentLine++;
		}
		if (appStrlen(Str) == 0)
			continue;
		if (GetBEGIN(&Str, TEXT("Object")))
		{
			// If SubobjectOuter is NULL, we are importing defaults for a UScriptStruct's defaultproperties block
			if (!bSubObjectsAllowed)
			{
				Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: Subobjects are not allowed in this context"));
				return NULL;
			}

			// Parse subobject default properties.
			// Note: default properties subobjects have compiled class as their Outer (used for localization).
			UClass* TemplateClass = NULL;
			ParseObject<UClass>(Str, TEXT("Class="), TemplateClass, ANY_PACKAGE);

			// parse the name of the template
			FName	TemplateName = NAME_None;
			Parse(Str, TEXT("Name="), TemplateName);
			if (TemplateName == NAME_None)
			{
				Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: Must specify valid name for subobject/component: %s"), *StrLine);
				return NULL;
			}

			// points to the parent class's template subobject/component, if we are overriding a subobject/component declared in our parent class
			UObject* BaseTemplate = NULL;
			if (TemplateClass)
			{
				if (TemplateClass->HasAnyClassFlags(CLASS_NeedsDefProps))
				{
					// defer until the subobject's class has imported its defaults and initialized its CDO
					return NULL;
				}
			}
			else
			{
				// If no class was specified, we are overriding a template from a parent class; this is only allowed during script compilation
				// and only when importing a subobject for a CDO (inheritance isn't allowed in a nested subobject) so check all that stuff first
				if (!SubobjectOuter->HasAnyFlags(RF_ClassDefaultObject))
				{
					Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: Missing class in subobject/component definition: %s"), *StrLine);
					return NULL;
				}

				// next, verify that a template actually exists in the parent class
				UClass* ParentClass = ComponentOwnerClass->GetSuperClass();
				check(ParentClass);

				UObject* ParentCDO = ParentClass->GetDefaultObject();
				check(ParentCDO);

				FObjectRefFinder Finder(NULL, TemplateName);
				Finder.SeekForObject(ParentCDO);

				BaseTemplate = Finder.ResultObject;
				if (BaseTemplate == NULL)
				{
					// wasn't found
					Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: No base template named %s found in parent class %s: %s"), *TemplateName, ParentClass->GetName(), *StrLine);
					return NULL;
				}

				TemplateClass = BaseTemplate->GetClass();
			}

			// @todo nested components: the last check for RF_ClassDefaultObject is stopping nested components,
			// because the outer won't be a default object

			check(TemplateClass);
			if (TemplateClass->IsChildOf(UComponent::StaticClass()) && SubobjectOuter->HasAnyFlags(RF_ClassDefaultObject))
			{
				UComponent* OverrideComponent = ComponentOwnerClass->ComponentNameToDefaultObjectMap.FindRef(TemplateName);
				if (OverrideComponent)
				{
					if (BaseTemplate == NULL)
					{
						// BaseTemplate should only be NULL if the Begin Object line specified a class
						Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: The component name %s is already used (if you want to override the component, don't specify a class): %s"), *TemplateName, *StrLine);
						return NULL;
					}

					// the component currently in the component template map and the base template should be the same
					checkf(OverrideComponent == BaseTemplate, TEXT("OverrideComponent: '%s'   BaseTemplate: '%s'"), OverrideComponent->GetFullName(), BaseTemplate->GetFullName());
				}


				UClass* ComponentSuperClass = TemplateClass;

				// Propagate object flags to the sub object.
				const EObjectFlags NewFlags = SubobjectOuter->GetFlags() & RF_PropagateToSubObjects;

				UComponent* ComponentTemplate = ConstructObject<UComponent>(
					TemplateClass,
					SubobjectOuter,
					TemplateName,
					NewFlags | ((TemplateClass->ClassFlags & CLASS_Localized) ? RF_PerObjectLocalized : 0),
					OverrideComponent,
					SubobjectOuter);
				ReplaceSer.ReplaceMap.Set(BaseTemplate, ComponentTemplate);

				ComponentTemplate->TemplateName = TemplateName;

				// if we had a template, update any properties in this class to point to the new component instead of the template
				if (OverrideComponent)
				{
					// replace all properties in this subobject outer' class that point to the original subobject with the new subobject
					TMap<UComponent*, UComponent*> ReplacementMap;
					ReplacementMap.Set(OverrideComponent, ComponentTemplate);
					FArchiveReplaceObjectRef<UComponent> ReplaceAr(SubobjectOuter, ReplacementMap, FALSE, FALSE, FALSE);
				}

				// import the properties for the subobject
				SourceText = ImportObjectProperties(
					ComponentTemplate->GetScriptData(),
					SourceText,
					TemplateClass,
					SubobjectRoot,
					ComponentTemplate,
					Warn,
					Depth + 1,
					ContextSupplier ? ContextSupplier->CurrentLine : 0
				);

				ComponentNameToInstanceMap.Set(TemplateName, ComponentTemplate);

				if (!bSubObject || SubobjectOuter == SubobjectRoot)
				{
					ComponentOwnerClass->ComponentNameToDefaultObjectMap.Set(TemplateName, ComponentTemplate);

					// now add any components that were instanced as a result of references to other components in the class
					TArray<UComponent*> ReferencedComponents;
					//InstanceGraph.RetrieveComponents(SubobjectRoot, ReferencedComponents, FALSE);

					for (INT ComponentIndex = 0; ComponentIndex < ReferencedComponents.Num(); ComponentIndex++)
					{
						UComponent* Component = ReferencedComponents(ComponentIndex);
						ComponentNameToInstanceMap.Set(Component->GetFName(), Component);
						ComponentOwnerClass->ComponentNameToDefaultObjectMap.Set(Component->GetFName(), Component);
					}
				}

				// let the UComponent hook up it's source pointers, but only for placed subobjects (if the outer is a class, we are in the .u file, no linking)
				if (!ComponentTemplate->IsTemplate())
				{
					ComponentTemplate->LinkToSourceDefaultObject(NULL, ComponentOwnerClass, TemplateName);
					// make sure that this points to something
					// @todo: handle this error nicely, in case there was a poorly typed .t3d file...)
					check(ComponentTemplate->ResolveSourceDefaultObject() != NULL);
				}
			}
			// handle the non-template case (subobjects and non-template components)
			else
			{
				// prevent component subobject definitions inside of other subobject definitions, as the correct instancing behavior in this situation is undefined
				if (TemplateClass->IsChildOf(UComponent::StaticClass()) && SubobjectRoot->HasAnyFlags(RF_ClassDefaultObject))
				{
					Warn->Logf(NAME_Error, TEXT("Nested component definitions (%s in %s) in class files are currently not supported: %s"), *TemplateName, SubobjectOuter->GetFullName(), *StrLine);
					return NULL;
				}

				UObject* TemplateObject;

				// for .t3d placed components that aren't in the class, and have no template, we need the object name
				FName	ObjectName = NAME_None;
				if (!Parse(Str, TEXT("ObjName="), ObjectName))
				{
					ObjectName = TemplateName;
				}

				// if an archetype was specified in the Begin Object block, use that as the template for the ConstructObject call.
				// @fixme subobjects: Generalize this whole system into using Archetype= for all Object importing
				FString ArchetypeName;
				UObject* Archetype = NULL;
				if (Parse(Str, TEXT("Archetype="), ArchetypeName))
				{
					// if given a name, break it up along the ' so separate the class from the name
					TArray<FString> Refs;
					ArchetypeName.ParseIntoArray(&Refs, TEXT("'"), TRUE);
					// find the class
					UClass* ArchetypeClass = (UClass*)UObject::StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *Refs(0));
					if (ArchetypeClass)
					{
						// if we had the class, find the archetype
						// @fixme ronp subobjects: this _may_ need StaticLoadObject, but there is currently a bug in StaticLoadObject that it can't take a non-package pathname properly
						Archetype = UObject::StaticFindObject(ArchetypeClass, ANY_PACKAGE, *Refs(1));
					}
				}
				else
				{
					Archetype = BaseTemplate;
				}

				UObject* ExistingObject;
				if ((ExistingObject = FindObject<UObject>(SubobjectOuter, *ObjectName)) != NULL)
				{
					// if we're overriding a subobject declared in a parent class, we should already have an object with that name that
					// was instanced when ComponentOwnerClass's CDO was initialized; if so, it's archetype should be the BaseTemplate.  If it
					// isn't, then there are two unrelated subobject definitions using the same name.
					if (ExistingObject->GetArchetype() != BaseTemplate)
					{
						Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: name %s redefined: %s"), *ObjectName, *StrLine);
					}
					else if (BaseTemplate == NULL)
					{
						// BaseTemplate should only be NULL if the Begin Object line specified a class
						Warn->Logf(NAME_Error, TEXT("BEGIN OBJECT: A subobject named %s is already declared in a parent class.  If you intended to override that subobject, don't specify a class in the derived subobject definition: %s"), *TemplateName, *StrLine);
						return NULL;
					}
				}

				// Propagate object flags to the sub object.
				const EObjectFlags NewFlags = SubobjectOuter->GetFlags() & RF_PropagateToSubObjects;

				// create the subobject
				TemplateObject = ConstructObject<UObject>(
					TemplateClass,
					SubobjectOuter,
					ObjectName,
					NewFlags | ((TemplateClass->ClassFlags & CLASS_Localized) ? RF_PerObjectLocalized : 0),
					Archetype,
					SubobjectRoot
					);
				ReplaceSer.ReplaceMap.Set(BaseTemplate, TemplateObject);

				// replace any already-overridden non-component subobjects with their new instances in this subobject
				{
					//FArchiveReplaceObjectRef<UObject> ReplaceAr(TemplateObject, *InstanceGraph.GetSourceToDestinationMap(), FALSE, FALSE, TRUE);
				}

				if (Archetype != NULL && !Archetype->HasAnyFlags(RF_ClassDefaultObject))
				{
					// replace all properties in the class we're importing with to the original subobject with the new subobject
					TMap<UObject*, UObject*> ReplacementMap;
					ReplacementMap.Set(Archetype, TemplateObject);
					FArchiveReplaceObjectRef<UObject> ReplaceAr(SubobjectOuter, ReplacementMap, FALSE, FALSE, TRUE);
				}

				SourceText = ImportObjectProperties(
					TemplateObject->GetScriptData(),
					SourceText,
					TemplateClass,
					SubobjectRoot,
					TemplateObject,
					Warn,
					Depth + 1,
					ContextSupplier ? ContextSupplier->CurrentLine : 0
				);

				UComponent* ComponentSubobject = Cast<UComponent>(TemplateObject);
				if (ComponentSubobject != NULL)
				{
					ComponentNameToInstanceMap.Set(TemplateName, ComponentSubobject);
				}
			}
		}
		else if (ParseCommand(&Str, TEXT("CustomProperties")))
		{
			check(SubobjectOuter);

			//SubobjectOuter->ImportCustomProperties(Str, Warn);
		}
		else if (GetEND(&Str, TEXT("Actor")) || GetEND(&Str, TEXT("DefaultProperties")) || GetEND(&Str, TEXT("structdefaultproperties")) || (GetEND(&Str, TEXT("Object")) && Depth))
		{
			// End of properties.
			break;
		}
		else
		{
			// Property.
			TCHAR Token[4096];

			while (*Str == ' ' || *Str == 9)
			{
				Str++;
			}

			const TCHAR* Start = Str;

			while (*Str && *Str != '=' && *Str != '(' && *Str != '[' && *Str != '.')
			{
				Str++;
			}

			if (*Str)
			{
				appStrncpy(Token, Start, Str - Start + 1);

				// strip trailing whitespace on token
				INT l = appStrlen(Token);
				while (l && (Token[l - 1] == ' ' || Token[l - 1] == 9))
				{
					Token[l - 1] = 0;
					--l;
				}

				// Parse an array operation, if present.
				enum EArrayOp
				{
					ADO_None,
					ADO_Add,
					ADO_Remove,
					ADO_RemoveIndex,
					ADO_Empty,
				};

				EArrayOp	ArrayOp = ADO_None;
				if (*Str == '.')
				{
					Str++;
					if (ParseCommand(&Str, TEXT("Empty")))
					{
						ArrayOp = ADO_Empty;
					}
					else if (ParseCommand(&Str, TEXT("Add")))
					{
						ArrayOp = ADO_Add;
					}
					else if (ParseCommand(&Str, TEXT("Remove")))
					{
						ArrayOp = ADO_Remove;
					}
					else if (ParseCommand(&Str, TEXT("RemoveIndex")))
					{
						ArrayOp = ADO_RemoveIndex;
					}
				}

				UProperty* Property = FindField<UProperty>(ObjectStruct, Token);

				// this the default parent to use
				UProperty::ImportTextParent = (GEditor->Bootstrapping && SubobjectOuter != NULL) ? SubobjectOuter : *(UObject**)DestData;

				if (!Property)
				{
					// Check for a delegate property
					FString DelegateName = FString::Printf(TEXT("__%s__Delegate"), Token);
					Property = FindField<UDelegateProperty>(ObjectStruct, *DelegateName);
					if (!Property)
					{
						Warn->Logf(NAME_Warning, TEXT("Unknown property in defaults: %s (looked in %s)"), *StrLine, *ObjectStruct->GetName());
						continue;
					}
				}

				// If the property is native then we usually won't import it.  However, sometimes we want
				// an editor user to be able to copy/paste native properties, in which case the properly
				// will be marked as SerializeText.
				const UBOOL bIsNativeProperty = (Property->PropertyFlags & CPF_Native);
				const UBOOL bAllowSerializeText = (Property->PropertyFlags & CPF_SerializeText);

				UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
				if (ArrayOp != ADO_None)
				{
					if (ArrayProperty == NULL)
					{
						Warn->Logf(NAME_ExecWarning, TEXT("Array operation performed on non-array variable: %s"), *StrLine);
						continue;
					}

					// if we're compiling a class, we want to pass in that class for the parent of the component property, even if the property is inside a nested component
					if (GEditor->Bootstrapping && ArrayProperty->Inner->HasAnyPropertyFlags(CPF_Component))
					{
						UProperty::ImportTextParent = SubobjectOuter;
					}
					FArray* Array = (FArray*)(DestData + Property->Offset);

					if (ArrayOp == ADO_Empty)
					{
						Array->Empty(ArrayProperty->Inner->ElementSize);
					}
					else if (ArrayOp == ADO_Add || ArrayOp == ADO_Remove)
					{
						SkipWhitespace(Str);
						if (*Str++ != '(')
						{
							Warn->Logf(NAME_ExecWarning, TEXT("Missing '(' in default properties array operation: %s"), *StrLine);
							continue;
						}
						SkipWhitespace(Str);

						FString ValueText = ReadStructValue(Str, SourceText);
						Str = *ValueText;

						if (ArrayOp == ADO_Add)
						{
							INT Size = ArrayProperty->Inner->ElementSize;
							INT	Index = Array->AddZeroed(Size);
							BYTE* ElementData = (BYTE*)Array->GetData() + Index * Size;

							if (ArrayProperty->Inner->IsA(UDelegateProperty::StaticClass()))
							{
								FString Temp;
								if (appStrchr(Str, '.') == NULL)
								{
									// if no class was specified, use the class currently being imported
									Temp = Depth ? SubobjectRoot->GetName() : ObjectStruct->GetName();
									Temp = Temp + TEXT(".") + Str;
								}
								else
								{
									Temp = Str;
								}
								FScriptDelegate* D = (FScriptDelegate*)(ElementData);
								D->Object = NULL;
								D->FunctionName = NAME_None;
								const TCHAR* Result = ArrayProperty->Inner->ImportText(*Temp, ElementData, PortFlags);
								UBOOL bFailedImport = (Result == NULL || Result == *Temp);
								if (bFailedImport)
								{
									Warn->Logf(NAME_Warning, TEXT("Delegate assignment failed: %s"), *StrLine);
								}
							}
							else
							{
								UStructProperty* StructProperty = Cast<UStructProperty>(ArrayProperty->Inner);
								if (StructProperty)
								{
									// Initialize struct defaults.
									BYTE* Def = StructProperty->Struct->GetDefaults();
									if(Def)
										StructProperty->CopySingleValue(ElementData, Def);
								}

								const TCHAR* Result = ArrayProperty->Inner->ImportText(Str, ElementData, PortFlags);
								if (Result == NULL || Result == Str)
								{
									Warn->Logf(NAME_Warning, TEXT("Unable to parse parameter value '%s' in defaultproperties array operation: %s"), Str, *StrLine);
									Array->Remove(Index, 1, Size);
								}
							}
						}
						else if (ArrayOp == ADO_Remove)
						{
							INT Size = ArrayProperty->Inner->ElementSize;

							BYTE* Temp = new BYTE[Size];
							appMemzero(Temp, Size);

							// export the value specified to a temporary buffer
							const TCHAR* Result = ArrayProperty->Inner->ImportText(Str, Temp, PortFlags);
							if (Result == NULL || Result == Str)
							{
								Warn->Logf(NAME_Error, TEXT("Unable to parse parameter value '%s' in defaultproperties array operation: %s"), Str, *StrLine);
							}
							else
							{
								// find the array member corresponding to this value
								UBOOL bIsComponentsArray = ArrayProperty->GetFName() == NAME_Components && ArrayProperty->GetOwnerClass()->GetFName() == NAME_Actor;
								UBOOL bFound = FALSE;
								for (UINT Index = 0; Index < (UINT)Array->Num(); Index++)
								{
									BYTE* DestData = (BYTE*)Array->GetData() + Index * Size;
									if (ArrayProperty->Inner->Identical(Temp, DestData))
									{
										if (RemovedComponents && bIsComponentsArray)
										{
											RemovedComponents->AddUniqueItem(*(UComponent**)DestData);
										}
										Array->Remove(Index--, 1, ArrayProperty->Inner->ElementSize);
										bFound = TRUE;
									}
								}
								if (!bFound)
								{
									Warn->Logf(NAME_Warning, TEXT("%s.Remove(): Value not found in array"), ArrayProperty->GetName());
								}
							}
							ArrayProperty->Inner->DestroyValue(Temp);
							delete[] Temp;
						}
					}
					else if (ArrayOp == ADO_RemoveIndex)
					{
						SkipWhitespace(Str);
						if (*Str++ != '(')
						{
							Warn->Logf(NAME_ExecWarning, TEXT("Missing '(' in default properties array operation:: %s"), *StrLine);
							continue;
						}
						SkipWhitespace(Str);

						FString strIdx;
						while (*Str != ')')
						{
							strIdx += *Str;
							Str++;
						}
						INT removeIdx = appAtoi(*strIdx);

						UBOOL bIsComponentsArray = ArrayProperty->GetFName() == NAME_Components && ArrayProperty->GetOwnerClass()->GetFName() == NAME_Actor;
						if (bIsComponentsArray && RemovedComponents)
						{
							if (removeIdx < Array->Num())
							{
								BYTE* ElementData = (BYTE*)Array->GetData() + removeIdx * ArrayProperty->Inner->ElementSize;
								RemovedComponents->AddUniqueItem(*(UComponent**)ElementData);
							}
						}
						Array->Remove(removeIdx, 1, ArrayProperty->Inner->ElementSize);
					}
				}
				else
				{
					// try to read an array index
					INT Index = ReadArrayIndex(ObjectStruct, Cast<UClass>(SubobjectRoot), Str, Warn);

					// check for out of bounds on static arrays
					if (ArrayProperty == NULL && Index >= Property->ArrayDim)
					{
						Warn->Logf(NAME_Warning, TEXT("Out of bound array default property (%i/%i): %s"), Index, Property->ArrayDim, *StrLine);
						continue;
					}


					// check to see if this property has already imported data
					FDefinedProperty D;
					D.Property = Property;
					D.Index = Index;
					if (DefinedProperties.FindItemIndex(D) != INDEX_NONE)
					{
						Warn->Logf(NAME_Warning, TEXT("redundant data: %s"), *StrLine);
						continue;
					}
					DefinedProperties.AddItem(D);

					// strip whitespace before =
					SkipWhitespace(Str);
					if (*Str++ != '=')
					{
						Warn->Logf(NAME_Warning, TEXT("Missing '=' in default properties assignment: %s"), *StrLine);
						continue;
					}
					// strip whitespace after =
					SkipWhitespace(Str);

					// limited multi-line support, look for {...} sequences and condense to a single entry
					FString FullText = ReadStructValue(Str, SourceText);

					// set the pointer to our new text
					Str = *FullText;
					if (Property->GetFName() != NAME_Name)
					{
						l = appStrlen(Str);
						while (l && (Str[l - 1] == ';' || Str[l - 1] == ' ' || Str[l - 1] == 9))
						{
							*(TCHAR*)(&Str[l - 1]) = 0;
							--l;
						}
						if (Property->IsA(UStrProperty::StaticClass()) && (!l || *Str != '"' || Str[l - 1] != '"'))
							Warn->Logf(NAME_Warning, TEXT("Missing '\"' in string default properties: %s"), *StrLine);

						if (Index > -1 && ArrayProperty != NULL) //set single dynamic array element
						{
							FArray* Array = (FArray*)(DestData + Property->Offset);
							
							if (Index >= Array->Num())
							{
								INT NumToAdd = Index - Array->Num() + 1;
								Array->AddZeroed(ArrayProperty->Inner->ElementSize, NumToAdd);
								UStructProperty* StructProperty = Cast<UStructProperty>(ArrayProperty->Inner);
								if (StructProperty && StructProperty->Struct->GetDefaultsCount())
								{
									// initialize struct defaults for each element we had to add to the array
									for (INT i = 1; i <= NumToAdd; i++)
									{
										StructProperty->CopySingleValue((BYTE*)Array->GetData() + ((Array->Num() - i) * ArrayProperty->Inner->ElementSize), StructProperty->Struct->GetDefaults());
									}
								}
							}

							// if we're compiling a class, we want to pass in that class for the parent of the component property, even if the property is inside a nested component
							if (GEditor->Bootstrapping)
							{
								UProperty::ImportTextParent = SubobjectOuter;
							}

							const TCHAR* Result;
							if (ArrayProperty->Inner->IsA(UDelegateProperty::StaticClass()))
							{
								FString Temp;
								if (appStrchr(Str, '.') == NULL)
								{
									// if no class was specified, use the class currently being imported
									Temp = Depth ? SubobjectRoot->GetName() : ObjectStruct->GetName();
									Temp = Temp + TEXT(".") + Str;
								}
								else
								{
									Temp = Str;
								}
								FScriptDelegate* D = (FScriptDelegate*)((BYTE*)Array->GetData() + Index * ArrayProperty->Inner->ElementSize);
								D->Object = NULL;
								D->FunctionName = NAME_None;
								Result = ArrayProperty->Inner->ImportText(*Temp, (BYTE*)Array->GetData() + Index * ArrayProperty->Inner->ElementSize, PortFlags);
								UBOOL bFailedImport = (Result == NULL || Result == *Temp);
								if (bFailedImport)
								{
									Warn->Logf(NAME_Warning, TEXT("Delegate assignment failed: %s"), *StrLine);
								}
							}
							else
							{
								Result = ArrayProperty->Inner->ImportText(Str, (BYTE*)Array->GetData() + Index * ArrayProperty->Inner->ElementSize, PortFlags);
							}
							UBOOL bFailedImport = (Result == NULL || Result == Str);
							// @todo: only run this code when importing from a .uc file, not from a .t3d file
							if (bFailedImport)
							{
								// The property failed to import the value - this means the text was't valid for this property type
								// it could be a named constant, so let's try checking for that first

								// If this is a subobject definition, search for a matching const in the containing (top-level) class first
								UConst* Const = FindField<UConst>(bSubObject ? ComponentOwnerClass : ObjectStruct, Str);
								if (Const == NULL && bSubObject)
								{
									// if it still wasn't found, try searching the subobject's class
									Const = FindField<UConst>(ObjectStruct, Str);
								}
								if (Const == NULL)
									Const = FindObject<UConst>(ANY_PACKAGE, Str, 1);

								if (Const)
								{
									// found a const matching the value specified - retry to import the text using the value of the const
									Result = ArrayProperty->Inner->ImportText(*Const->Value, (BYTE*)Array->GetData() + Index * ArrayProperty->Inner->ElementSize, PortFlags);
									bFailedImport = (Result == NULL || Result == *Const->Value);
								}

								if (bFailedImport)
								{
									Warn->Logf(NAME_Warning, TEXT("Invalid property value in defaults: %s"), *StrLine);
								}
							}
						}
						else if (Property->IsA(UDelegateProperty::StaticClass()))
						{
							if (Index == -1) Index = 0;
							FString Temp;
							if (appStrchr(Str, '.') == NULL)
							{
								// if no class was specified, use the class currently being imported
								Temp = Depth ? SubobjectRoot->GetName() : ObjectStruct->GetName();
								Temp = Temp + TEXT(".") + Str;
							}
							else
								Temp = Str;
							FScriptDelegate* D = (FScriptDelegate*)(DestData + Property->Offset + Index * Property->ElementSize);
							D->Object = NULL;
							D->FunctionName = NAME_None;
							const TCHAR* Result = Property->ImportText(*Temp, DestData + Property->Offset + Index * Property->ElementSize, PortFlags);
							UBOOL bFailedImport = (Result == NULL || Result == *Temp);
							if (bFailedImport)
							{
								Warn->Logf(NAME_Warning, TEXT("Delegate assignment failed: %s"), *StrLine);
							}
						}
						else
						{
							if (Index == INDEX_NONE)
							{
								Index = 0;
							}

							// if we're compiling a class, we want to pass in that class for the parent of the component property, even if the property is inside a nested component
							if (GEditor->Bootstrapping)
							{
								UProperty::ImportTextParent = SubobjectOuter;
							}

							const TCHAR* Result = Property->ImportText(Str, DestData + Property->Offset + Index * Property->ElementSize, PortFlags);
							UBOOL bFailedImport = (Result == NULL || Result == Str);

							// @todo: only run this code when importing from a .uc file, not from a .t3d file
							if (bFailedImport)
							{
								// The property failed to import the value - this means the text was't valid for this property type
								// it could be a named constant, so let's try checking for that first
								// If this is a subobject definition, search for a matching const in the containing (top-level) class first
								UConst* Const = FindField<UConst>(bSubObject ? SubobjectRoot->GetClass() : ObjectStruct, Str);
								if (Const == NULL && bSubObject)
								{
									// if it still wasn't found, try searching the subobject's class
									Const = FindField<UConst>(ObjectStruct, Str);
								}
								if (Const == NULL)
									Const = FindObject<UConst>(ANY_PACKAGE, Str, 1);

								if (Const)
								{
									// found a const matching the value specified - retry to import the text using the value of the const
									Result = Property->ImportText(*Const->Value, DestData + Property->Offset + Index * Property->ElementSize, PortFlags);
									bFailedImport = (Result == NULL || Result == *Const->Value);
								}

								if (bFailedImport)
									Warn->Logf(NAME_Warning, TEXT("Invalid property value in defaults: %s"), *StrLine);
							}
						}
					}
				}
			}
		}
	}
	ReplaceSer.ReplaceRefs(SubobjectRoot);
	return SourceText;
	unguard;
}

const TCHAR* ImportObjectProperties(
	BYTE* DestData,
	const TCHAR* SourceText,
	UStruct* ObjectStruct,
	UObject* SubobjectRoot,
	UObject* SubobjectOuter,
	FFeedbackContext* Warn,
	INT					Depth,
	INT					LineNumber
)
{
	guard(ImportObjectProperties);
	TMap<FName, UComponent*>	ComponentNameToInstanceMap;

	FDefaultPropertiesContextSupplier Supplier;
	if (LineNumber != INDEX_NONE)
	{
		if (SubobjectRoot == NULL)
		{
			Supplier.PackageName = ObjectStruct->GetOwnerClass()->GetOutermost()->GetName();
			Supplier.ClassName = ObjectStruct->GetOwnerClass()->GetName();
			Supplier.CurrentLine = LineNumber;

			ContextSupplier = &Supplier;
		}
		else if (ContextSupplier != NULL)
			ContextSupplier->CurrentLine = LineNumber;
		GWarn->SetContext(ContextSupplier);
	}

	// Parse the object properties.
	const TCHAR* NewSourceText =
		ImportProperties(
			DestData,
			SourceText,
			ObjectStruct,
			SubobjectRoot,
			SubobjectOuter,
			ComponentNameToInstanceMap,
			NULL,
			GWarn,
			Depth);

	if (SubobjectOuter != NULL)
	{
		check(SubobjectRoot);

		// Update the object properties to point to the newly imported component objects.
		// Templates inside classes never need to have components instanced.
		if (!SubobjectRoot->HasAnyFlags(RF_ClassDefaultObject))
		{
			for (TMap<FName, UComponent*>::TIterator It(ComponentNameToInstanceMap); It; ++It)
			{
				FName ComponentName = It.Key();
				UComponent* Component = It.Value();
				UComponent* ComponentArchetype = Cast<UComponent>(Component->GetArchetype());
			}

			UObject* SubobjectArchetype = SubobjectOuter->GetArchetype();
			BYTE* DefaultData = SubobjectArchetype->GetScriptData();

			//ObjectStruct->InstanceComponentTemplates(DestData, DefaultData, SubobjectArchetype ? SubobjectArchetype->GetClass()->GetPropertiesSize() : NULL, SubobjectOuter);
		}
	}

	if (LineNumber != INDEX_NONE)
	{
		if (ContextSupplier == &Supplier)
		{
			ContextSupplier = NULL;
			Warn->SetContext(NULL);
		}
	}

	return NewSourceText;
	unguard;
}

static void RemoveComponentsFromClass(UObject* Owner, TArray<UComponent*> RemovedComponents)
{
	check(Owner);

	UClass* OwnerClass = Owner->GetClass();
	if (RemovedComponents.Num())
	{
		// find out which components are still being referenced by the object
		TArray<UComponent*> ComponentReferences;
		TArchiveObjectReferenceCollector<UComponent> InheritedCollector(&ComponentReferences, Owner->GetArchetype(), TRUE, TRUE);
		TArchiveObjectReferenceCollector<UComponent> Collector(&ComponentReferences, Owner, TRUE, TRUE);
		Owner->Serialize(InheritedCollector);
		Owner->Serialize(Collector);

		for (INT RemovalIndex = RemovedComponents.Num() - 1; RemovalIndex >= 0; RemovalIndex--)
		{
			UComponent* Component = RemovedComponents(RemovalIndex);
			for (INT ReferenceIndex = 0; ReferenceIndex < ComponentReferences.Num(); ReferenceIndex++)
			{
				// this component is still being referenced by the object, so it needs to remain
				// in the ComponentNameToDefaultObjectMap
				UComponent* ReferencedComponent = ComponentReferences(ReferenceIndex);
				if (ReferencedComponent == Component)
				{
					RemovedComponents.Remove(RemovalIndex);
					break;
				}
			}
		}

		for (INT RemovalIndex = 0; RemovalIndex < RemovedComponents.Num(); RemovalIndex++)
		{
			// these components are no longer referened by the object, so they should also be 
			// removed from the class's name->component map, so that child classes don't copy
			// those re-add those components when they copy inherited components
			UComponent* Component = RemovedComponents(RemovalIndex);
			OwnerClass->ComponentNameToDefaultObjectMap.Remove(Component->GetFName());
		}
	}
}

const TCHAR* ImportDefaultProperties(
	UClass* Class,
	const TCHAR* Text,
	FFeedbackContext* Warn,
	INT					Depth,
	INT					LineNumber
)
{
	guard(ImportDefaultProperties);
	UBOOL	Success = 1;

	FDefaultPropertiesContextSupplier Context(Class->GetOuter()->GetName(), Class->GetName(), LineNumber);
	ContextSupplier = &Context;
	Warn->SetContext(ContextSupplier);

	// this tracks any components that were removed from the class via the Components.Remove() functionality
	TArray<UComponent*> RemovedComponents;

	// Parse the default properties.
	if (Text)
	{
		try
		{
			Text = ImportProperties(
				Class->GetDefaults(),
				Text,
				Class,
				NULL,
				Class->GetDefaultObject(),
				Class->ComponentNameToDefaultObjectMap,
				&RemovedComponents,
				Warn,
				Depth
			);
		}
		catch (TCHAR* ErrorMsg)
		{
			Warn->Log(NAME_Error, ErrorMsg);
			Text = NULL;
		}
		Success = Text != NULL;
	}

	if (Success)
	{
		// if we've removed items from the Components array we'll need
		// to also remove that component from the owner class's ClassNameToDefaultObjectMap, assuming
		// that there are no other references to that UComponent.
		RemoveComponentsFromClass(Class->GetDefaultObject(), RemovedComponents);
	}

	ContextSupplier = NULL;
	Warn->SetContext(NULL);
	return Text;
	unguardf((TEXT("(%ls)"), Class->GetFullName()));
}
