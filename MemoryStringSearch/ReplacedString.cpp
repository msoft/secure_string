#include "ReplacedString.h"

ReplacedString::ReplacedString(IntPtr address, IntPtr baseAddress, unsigned long length,
	bool isWide)
{
	this->address = address;
	this->baseAddress = baseAddress;
	this->length = length;
	this->isWide = isWide;
}

