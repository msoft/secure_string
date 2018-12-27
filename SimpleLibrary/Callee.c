#include <phbase.h>
#include <memsrch.h>
#include <SearchMemory.h>
#include <stdlib.h>

const REPLACED_MEMORY_STRINGS *ConvertToReplacedMemoryStrings(const MEMORY_STRING_RESULTS *stringResults)
{
	REPLACED_MEMORY_STRINGS *replacedStrings = malloc(sizeof(*replacedStrings));
	replacedStrings->FirstString = NULL;

	MEMORY_STRING_RESULT *stringResult = stringResults->FirstString;
	while (stringResult != NULL)
	{
		REPLACED_MEMORY_STRINGS *replacedString = malloc(sizeof(*replacedString));
		replacedString->Address = stringResult->Address;
		replacedString->BaseAddress = stringResult->BaseAddress;
		replacedString->IsWide = stringResult->IsWide;
		replacedString->Length = stringResult->Length;

		replacedString->Next = replacedString->FirstString;
		replacedStrings->FirstString = replacedString;

		stringResult = stringResult->Next;
	}

	return replacedStrings;
}

const REPLACED_MEMORY_STRINGS *SearchAndEraseStringsWithinMemory(wchar_t *stringToFind, unsigned long processId)
{
	const MEMORY_STRING_RESULTS *foundStrings = FindStringWithinProcessMemory(
		processId, stringToFind, TRUE, FALSE, FALSE, TRUE, TRUE);

	const MEMORY_STRING_RESULTS *replacedStringsInMemory = ReplaceStringWithinProcessMemory(
		processId, foundStrings, stringToFind);

	const REPLACED_MEMORY_STRINGS *replacedStrings = ConvertToReplacedMemoryStrings(
		replacedStringsInMemory);

	FreeMemoryStrings(foundStrings);
	FreeMemoryStrings(replacedStringsInMemory);

	return replacedStrings;
}