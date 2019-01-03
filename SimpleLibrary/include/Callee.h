#ifndef PH_CALLEE_H
#define PH_CALLEE_H

typedef struct REPLACED_MEMORY_STRING REPLACED_MEMORY_STRING;
struct REPLACED_MEMORY_STRING
{
	void *BaseAddress;
	void *Address;
	unsigned long Length;
	unsigned char IsWide;

	REPLACED_MEMORY_STRING *Next;
};

typedef struct _REPLACED_MEMORY_STRINGS REPLACED_MEMORY_STRINGS;
struct _REPLACED_MEMORY_STRINGS
{
	REPLACED_MEMORY_STRING *FirstString;
};

extern const REPLACED_MEMORY_STRINGS *SearchAndEraseStringsWithinMemory(wchar_t *stringToFind,
	unsigned long processId);

#endif