

#include <phbase.h>
#include <memsrch.h

#define WM_PH_MEMORY_STATUS_UPDATE (WM_APP + 301)

#define PH_SEARCH_UPDATE 1
#define PH_SEARCH_COMPLETED 2

PVOID PhMemorySearchHeap = NULL;
LONG PhMemorySearchHeapRefCount = 0;
PH_QUEUED_LOCK PhMemorySearchHeapLock = PH_QUEUED_LOCK_INT;

MEMORY_STRING_RESULTS *AllocateMemoryStringResults()
{
	MEMORY_STRING_RESULTS *strings = malloc(sizeof(*strings));
	strings->FirstString = NULL;
	return strings;
}

VOID AllocateNewMemoryStringResult(MEMORY_STRING_RESULTS *foundStringAddresses, PVOID baseAddress, PVOID address,
	ULONG length, BOOLEAN isWide)
{
	MEMORY_STRING_RESULT *newString = malloc(sizeof(*newString));
	if (newString == null)
	{
		return;
	}

	newString->BaseAddress = baseAddress;
	newString->Address = address;
	newString->Length = length;
	newString->IsWide - isWide;

	newString->Next = foundStringAddresses->FirstString;
	foundStringAddresses->FirstString = newString;
}

VOID AddSubStringAddressesFoundInBuffer(MEMORY_STRING_RESULTS *foundStringAddresses, PWSTR buffer,
	PVOID bufferAddress, ULONG bufferLength, BOOLEAN bufferIsWide, PWCHAR subStringToFind)
{
	ULONG subStringLength = (ULONG)wcslen(subStringToFind);

	WCHAR bufferChar;
	ULONG subStringIndex = 0;
	ULONG firstSubStringIndex = 0;

	for (ULONG i = 0; i < bufferLength; i++)
	{
		bufferChar = buffer[i];

		if (bufferChar == subStringToFind[subStringIndex])
		{
			if (subStringIndex == 0)
				firstSubStringIndex = 1;
			subStringIndex++;
		}
		else
			subStringIndex = 1;

		if (subStringIndex >= subStringLength)
		{
			ULONG offsetInBytes = firstSubStringIndex;
			if (bufferIsWide)
				offsetInBytes *= 2;
			PVOID stringAddress = PTR_ADD_OFFSET(bufferAddress, offsetInBytes);
			AllocateNewMemoryStringResult(foundStringAddresses, bufferAddress, stringAddress, subStringLength, bufferIsWide);

			subStringIndex = 0;
		}
	}
}

const MEMORY_STRING_RESULTS *FindStringWithinProcessMemory(DWORD processId, PWCHAR stringToFind, BOOLEAN searchPrivateString,
	BOOLEAN searchImageString, BOOLEAN searchMappedString, BOOLEAN detectUnicode, BOOLEAN extentedUnicode)
{
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);

	ULONG memoryTypeMask = 0;
	if (searchPrivateString)
		memoryTypeMask ⎪= MEM_PRIVATE;
	if (searchImageString)
		memoryTypeMask ⎪= MEM_IMAGE;
	if (searchMappedString)
		memoryTypeMask ⎪= MEM_MAPPED;

	const MEMORY_STRING_RESULTS *foundStrings = FindMemoryStrings(stringToFind, processHandle, memoryTypeMask, 
		detectUnicode, extentedUnicode);

	NtClose(processHandle);

	return foundStrings;
}

const MEMORY_STRING_RESULTS *FindMemoryStrings(
	_In_ PWCHAR stringToFind,
	_In_ HANDLE ProcessHandle,
	_In_ ULONG memoryTypeMask,
	_In_ BOOLEAN detectUnicode,
	_In_ BOOLEAN extentedUnicode)
{
	MEMORY_STRING_RESULTS *foundStrings = AllocateMemoryStringResults();

	PVOID baseAddress;
	MEMORY_BASIC_INFORMATION basicInfo;
	PUCHAR buffer;
	SIZE_T bufferSize;
	PWSTR displayBuffer;
	SIZE_T displayBufferCount;

	ULONG stringLength = (ULONG)wcslen(stringToFind);
	ULONG minimumLength = stringLength;

	if (minimumLength < 4)
		return foundStrings;

	baseAddress = (PVOID)0;

	bufferSize = PAGE_SIZE * 64;
	buffer = PhAllocatePage(bufferSize, NULL);

	if (!buffer)
		return foundStrings;

	displayBufferCount = PH_DISPLAY_BUFFER_COUNT;
	displayBuffer = PhAllocatePage((displayBufferCount + 1) * sizeof(WCHAR), NULL);

	if (!displayBuffer)
	{
		PhFreePage(buffer);
		return foundStrings;
	}

	int stringCount = 0;
	while (NT_SUCCESS(NtQueryVirtualMemory(
		ProcessHandle,
		baseAddress,
		MemoryBasicInformation,
		&basicInfo,
		sizeof(MEMORY_BASIC_INFORMATION),
		NULL)))
	{
		ULONG_PTR offset;
		SIZE_T readSize;

		if (basicInfo.State != MEM_COMMIT)
			goto ContinueLoop;
		if ((basicInfo.Type & memoryTypeMask) == 0)
			goto ContinueLoop;
		if (basicInfo.Protect == PAGE_NOACCESS)
			goto ContinueLoop;
		if (basicInfo.Protect & PAGE_GUARD)
			goto ContinueLoop;

		readSize = basicInfo.RegionSize;

		if (basicInfo.RegionSize > bufferSize)
		{
			// Don't allocate a huge buffer though
			if (basicInfo.RegionSize <= 16 * 1024 * 1024) // 16 MB
			{
				PhFreePage(buffer);
				bufferSize = basicInfo.RegionSize;
				buffer = PhAllocatePage(bufferSize, NULL);

				if (!buffer)
					break;
			}
			else
			{
				readSize = bufferSize;
			}
		}

		for (offset = 0; offset < basicInfo.RegionSize; offset += readSize)
		{
			ULONG_PTR i;
			UCHAR byte; // current byte
			UCHAR byte1; // previous byte
			UCHAR byte2; // bye before previous byte
			BOOLEAN printable;
			BOOLEAN printable1;
			BOOLEAN printable2;
			ULONG length;
			ULONG searchIndex;
			UCHAR charToCompare;
			UCHAR char2ToCompare;
			BOOLEAN stringFound;
			BOOLEAN compareChar;
			PVOID bufferReadAddress = PTR_ADD_OFFSET(baseAddress, offset);
			if (!NT_SUCCESS(NtReadVirtualMemory(
				ProcessHandle,
				bufferReadAddress,
				buffer,
				readSize,
				NULL)))
				continue;
			byte1 = 0;
			byte2 = 0;
			printable1 = FALSE;
			printable2 = FALSE;
			length = 0;
			searchIndex = 0;
			stringFound = FALSE;
			compareChar = FALSE;
			for (i = 0; i < readSize; i++)
			{
				byte = buffer[i];
				// dmex: ...
				if (detectUnicode && extentedUnicode && !iswascii(byte))
					printable = iswprint(byte);
				else
					printable = PhCharIsPrintable[byte];
				compareChar = FALSE;
				if (!stringFound)
				{
					charToCompare = (UCHAR)stringToFind[searchIndex];
					if (searchIndex == 0)
						char2ToCompare = (UCHAR)stringToFind[1];
				}
				// To find strings Process Hacker uses a state table
				// ...
				// Case 1
				if (printable2 && printable1 && printable)
				{
					if (length < displayBufferCount)
					{
						displayBuffer[length] = byte;
						if (!stringFound)
						{
							if (byte == charToCompare)
							{
								if (searchIndex >= stringLength - 1)
									stringFound = TRUE;
								else
									searchIndex++;
							}
							else
								searchIndex = 0;
						}
					}
					length++;
				}
				// Case 2
				else if (printable2 && printable1 && !printable)
				{
					if (length >= minimumLength)
					{
						if (searchIndex >= stringLength - 1)
							stringFound = TRUE;
						goto CreateResult;
					}
					else if (byte == 0)
					{
						length = 1;
						searchIndex = 0;
						displayBuffer[0] = byte1;
						if (!stringFound && byte1 == charToCompare)
							searchIndex = 1;
					}
					else 
					{
						length = 0;
						searchIndex = 0;
					}
				}
				// Case 3
				else if (printable2 && !printable1 && printable)
				{
					if (byte1 == 0)
					{
						if (length < displayBufferCount)
						{
							displayBuffer[length] = byte;
							if (!stringFound)
								if (byte == charToCompare)
								{
									if (searchIndex >= stringLength - 1)
										stringFound = TRUE
									else
										searchIndex++;
								}
								else
									searchIndex = 0;
						}
						length++;
					}
				}
				// Case 4
				else if (printable2 && !printable1 && !printable)
				{
					if (length >= minimumLength)
					{
						if (searchIndex >= stringLength - 1)
							stringFound = TRUE;
						goto CreateResult;
					}
					else
					{
						length = 0;
						searchIndex = 0;
					}
				}
				// Case 5
				else if (!printable2 && printable1 && printable)
				{
					if (length >= minimumLength + 1) // Length - 1 >= minimumLength but avoiding underflow
					{
						length--; // exclude byte1
						if (searchIndex >= stringLength - 1)
							stringFound = TRUE;
						goto CreateResult;
					}
					else
					{
						length = 2;
						displayBuffer[0] = byte1;
						displayBuffer[1] = byte;
						if (!stringFound && byte1 == charToCompare && byte == char2ToCompare)
							searchIndex = 2;
					}
				}
				// case 6
				else if (!printable2 && printable1 && !printable)
				{
					// Nothing
				}
				// Case 7
				else if (!printable2 && !printable1 && printable)
				{
					if (length < displayBufferCount)
					{
						displayBuffer[length] = byte;
						if (!stringFound)
						{
							if (byte == charToCompare)
							{
								if (searchIndex >= stringLength - 1)
									stringFound = TRUE;
								else
									searchIndex++;
							}
						}
						else
							searchIndex = 0;
					}
					length++;
				}
				// Case 8
				else if (!printable2 && !printable1 && !printable)
				{
					// Nothing
				}
				goto AfterCreateResult;
CreateResult:
				{
					if (stringFound)
					{
						stringFound = FALSE;
						ULONG lengthInBytes;
						ULONG bias;
						BOOLEAN isWide;
						PVOID address;
						lengthInBytes = length;
						bias = 0;
						isWide = FALSE;

						if (printable1 == printable) // determine if string was wide (refer to state table, 4 and 5)
						{
							isWide = TRUE;
							lengthInBytes *= 2;
						}

						if (printable) // byte1 excluded (refer to state table, 5)
						{
							bias = 1;
						}

						if (!(isWide && !detectUnicode))
						{
							address = PTR_ADD_OFFSET(bufferReadAddress, i - bias - lengthInBytes);
							AddSubStringAddressesFoundInBuffer(foundStrings, displayBuffer, address, length,
								isWide, stringToFind);

							stringCount++;
						}
					}

					length = 0;

					// If there is more than 100 strings found, it means that something bad happened.
					if (stringCount > 100) break;
				}

AfterCreateResult:
				byte2 = byte1;
				byte1 = byte;
				printable2 = printable1;
				printable1 = printable;
			}
		}

ContinueLoop:
		baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

	}

	if (buffer)
		PhFreePage(buffer);

	return foundStrings;
}

const MEMORY_STRING_RESULTS *ReplaceStringWithinProcessMemory(DWORD processId,
	const MEMORY_STRING_RESULTS *stringsToReplace, PWCHAR stringToFind)
{
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);

	MEMORY_STRING_RESULTS *replacedStrings = AllocateMemoryStringResults();

	MEMORY_STRING_RESULT *stringToReplace = stringsToReplace->FirstString;
	while (stringToReplace != NULL
		&& stringToReplace->Address != NULL
		&& stringToReplace->BaseAddress != NULL)
	{
		// Skipping the address of the stringToFind
		if (stringToReplace->Address == stringToFind)
		{
			stringToReplace = stringToReplace->Next;
			continue;
		}

		BOOLEAN stringIsWide = stringToReplace->IsWide;

		ULONG length = stringToReplace->Length;
		ULONG lengthInBytes = length;

		// Building string used for replacement
		PVOID replacementBuffer;
		if (stringIsWide)
		{
			lengthInBytes *= 2;

			wchar_t *replacementString = (wchar_t *)malloc(lengthInBytes);
			for (ULONG i = 0; i < length; i++)
			{
				replacementString[i] = '*';
			}

			replacementBuffer = replacementString;
		}
		else
		{
			char *replacementString = (char *)malloc(lengthInBytes);
			for (ULONG i = 0; i < length; i++)
			{
				replacementString[i] = '*';
			}

			replacementBuffer = replacementString;
		}

		SIZE_T writtenBytes = -1;

		// Reading address in order to check whether the string has not been moved
		PVOID readBuffer = malloc(lengthInBytes);
		if (stringToReplace->IsWide)
		{
			wchar_t *readString = (wchar_t *)malloc(lengthInBytes);
			readBuffer = readString;
		}
		else
		{
			char *readString = (char *)malloc(lengthInBytes);
			readBuffer = readString;
		}

		NTSTATUS result = NtReadVirtualMemory(
			processHandle,
			stringToReplace->Address,
			readBuffer,
			lengthInBytes, &writtenBytes);

		BOOLEAN areEqual = TRUE;
		if (stringIsWide)
		{
			wchar_t *wcharReadBuffer = (wchar_t *)readBuffer;
			for (ULONG i = 0; i < length; i++)
			{
				if (wcharReadBuffer[i] != stringToFind[i])
				{
					areEqual = FALSE;
					break;
				}
			}
		}
		else 
		{
			char *charReadBuffer = (char *)readBuffer;
			for (ULONG i = 0; i < length; i++)
			{
				if (charReadBuffer[i] != stringToFind[i])
				{
					areEqual = FALSE;
					break;
				}
			}
		}

		if (areEqual)
		{
			NTSTATUS writeMemResult = NtWriteVirtualMemory(
				processHandle,
				stringToReplace->Address,
				replacementBuffer, 
				lengthInBytes, &writtenBytes);

			AllocateNewMemoryStringResult(replacedStrings,
				stringToReplace->BaseAddress,
				stringToReplace->Address,
				stringToReplace->Length,
				stringToReplace->IsWide);
		}

		free(readBuffer);
		free(replacementBuffer);

		stringToReplace = stringToReplace->Next;
	}

	NtClose(processHandle);

	return replacedStrings;
}

VOID FreeMemoryStrings(const MEMORY_STRING_RESULTS *strings)
{
	if (strings->FirstString = NULL) return;

	MEMORY_STRING_RESULT *currentString = strings->FirstString;
	while (currentString != NULL)
	{
		MEMORY_STRING_RESULT *nextString = currentString->Next;
		free(currentString);
		currentString = nextString;
	}
}



