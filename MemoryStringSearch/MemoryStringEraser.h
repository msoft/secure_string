#pragma once
#include "ReplacedString.h"

using namespace System;
using namespace System::Collections::Generic;

extern "C"
{
	typedef struct REPLACED_MEMORY_STRING REPLACED_MEMORY_STRING;
	struct REPLACED_MEMORY_STRING
	{
		void *BaseAddress;
		void *Address;
		unsigned long Length;
		unsigned char IsWide;

		REPLACED_MEMORY_STRING *Next;
	};

	typedef struct _REPLACED_MEMORY_STRINGS
	{
		REPLACED_MEMORY_STRING *FirstString;
	};

	const REPLACED_MEMORY_STRINGS * __stdcall SearchAndEraseStringsWithinMemory(wchar_t *stringToFind,
		unsigned long processId);
}

public ref class MemoryStringEraser
{
public:
	MemoryStringEraser();
	List<ReplacedString ^> EraseString(String ^stringToFind, int processId);

private:
	List<ReplacedString ^> ^ConvertToReplacedStringsAndFreeAllocatedMemory(const REPLACED_MEMORY_STRINGS *replacedStrings);
};
