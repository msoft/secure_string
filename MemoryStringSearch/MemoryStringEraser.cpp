#include "stdafx.h"
#include "MemoryStringEraser.h"

MemoryStringEraser::MemoryStringEraser()
{}

List<ReplacedString ^> ^MemoryStringEraser::EraseString(String ^stringToFind, int processId)
{
	IntPtr unicodeStr = System::Runtime::InteropServices::Marshal::StringToHGlobalUni(stringToFind);
	wchar_t *convertedString = (wchar_t *)unicodeStr.ToPointer();

	const REPLACED_MEMORY_STRINGS *replacedMemoryStrings = SearchAndEraseStringsWithinMemory(convertedString, processId);
	List<ReplacedString ^> ^replacedStrings = ConvertToReplacedStringsAndFreeAllocatedMemory(replacedMemoryStrings);

	System::Runtime::InteropServices::Marshal::FreeHGlobal(unicodeStr);

	return replacedStrings;
}

List<ReplacedString ^> ^MemoryStringEraser::ConvertToReplacedStringsAndFreeAllocatedMemory(
	const REPLACED_MEMORY_STRINGS *replacedStrings)
{
	List<ReplacedString ^> ^convertedStrings = gcnew List<ReplacedString ^>();

	REPLACED_MEMORY_STRING *replacedMemoryString = replacedStrings->FirstString;
	while (replacedMemoryString != nullptr)
	{
		IntPtr replacedStringBaseAddress(replacedMemoryString->BaseAddress);
		IntPtr replacedStringAddress(replacedMemoryString->Address);

		ReplacedString ^convertedString = gcnew ReplacedString(replacedStringAddress, replacedStringBaseAddress,
			replacedMemoryString->Length, replacedMemoryString->IsWide);
		convertedStrings->Add(convertedString);

		REPLACED_MEMORY_STRING *nextMemoryString = replacedMemoryString->Next;
		delete replacedMemoryString;
		replacedMemoryString = nextMemoryString;
	}

	delete replacedStrings;

	return convertedStrings;
}
