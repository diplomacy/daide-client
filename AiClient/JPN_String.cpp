// JPN_String.cpp: implementation of the JPN::String class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~2~b

/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "JPN_String.h"

/////////////////////////////////////////////////////////////////////////////

namespace JPN {

/////////////////////////////////////////////////////////////////////////////

String::String(int n) {
	// Construct String representaion of `n`.

	const int size = 23; // sufficient for 64-bit int and terminating null
	char buffer[size];

#if	WINVER >= 0x0600
	if (sprintf_s(buffer, size, "%d", n) < 0) throw "Buffer overflow in `String(int)`";
#else
	sprintf(buffer, "%d", n); // hope `buffer` is big enough!
#endif

	*this = buffer;
}

int String::Find(char c, int begin) const {
	// Return index of 1st occurence of `c`, starting from `begin`; -1 iff not found.

	int i = find(c, begin);
	return i == npos ? -1 : i;
}

void String::MakeUpper() {
	// Convert to upper case.

	int i;
	int n = GetLength();
	for (i = 0; i < n; ++i) {
		char& c = (*this)[i];
		c = toupper(c);
	}
}

void String::Format(Str format, ...) {
	// Set to `format`, with optional following args substituted.

	va_list arg_list;
	va_start(arg_list, format);

	const int n = 32*1024;
	char buffer[n];

#if	WINVER >= 0x0600
	if (vsprintf_s(buffer, n, format, arg_list) < 0) throw "Buffer overflow in String::Format";
#else
	vsprintf(buffer, format, arg_list); // hope `buffer` is big enough!
#endif

	*this = buffer;

	va_end(arg_list);
}

/////////////////////////////////////////////////////////////////////////////

} // namespace JPN