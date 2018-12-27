#pragma once

using namespace System;

public ref class ReplacedString
{
private:
	IntPtr baseAddress;
	IntPtr address;
	unsigned long length;
	bool isWide;

public:
	ReplacedString(IntPtr address, IntPtr baseAddress, unsigned long length, bool isWide);

	property IntPtr BaseAddress
	{
		IntPtr get() { return baseAddress; }
	}

	property IntPtr Address
	{
		IntPtr get() { return address; }
	}

	property unsigned long Length
	{
		unsigned long get() { return length; }
	}

	property bool IsWide
	{
		bool get() { return isWide; }
	}
};

