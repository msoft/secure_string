#ifndef PH_MEMSRCH_H
#define PH_MEMSRCH_H


typedef struct MEMORY_STRING_RESULT MEMORY_STRING_RESULT;
struct MEMORY_STRING_RESULT
{
	PVOID BaseAddress;
	PVOID Address;
	ULONG Length;
	BOOLEAN IsWide;

	MEMORY_STRING_RESULT *Next;
};

typedef struct _MEMORY_STRING_RESULTS
{
	MEMORY_STRING_RESULT *FirstString;
	ULONG ItemCount;
	ULONG MaxItemCapacity;
} MEMORY_STRING_RESULTS;









#define PH_DISPLAY_BUFFER_COUNT (PAGE_SIZE * 2 - 1)

const MEMORY_STRING_RESULTS *FindMemoryString(
	_In_ PWCHAR stringToFind,
	_In_ HANDLE ProcessHandle,
	_In_ ULONG memoryTypeTask,
	_In_ BOOLEAN detectUnicode,
	_In_ BOOLEAN extendedUnicode);

const MEMORY_STRING_RESULTS *FindStringWithinProcessMemory(DWORD processId, PWCHAR stringToFind, BOOLEAN searchPrivateString,
	BOOLEAN searchImageString, BOOLEAN searchMappedString, BOOLEAN detectUnicode, BOOLEAN extentedUnicode);

const MEMORY_STRING_RESULTS *ReplaceStringWithinProcessMemory(DWORD processId,
	const MEMORY_STRING_RESULTS *stringsToReplace, PWCHAR stringToFind);

VOID FreeMemoryStrings(const MEMORY_STRING_RESULTS *strings);

#endif