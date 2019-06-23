// JPN_String.h: interface for the JPN::String class and related material.

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~2~b

/////////////////////////////////////////////////////////////////////////////

#ifndef _JPN_STRING
#define _JPN_STRING

namespace JPN {

/////////////////////////////////////////////////////////////////////////////

typedef const char* Str;

using namespace std;

class String: public std::string {
public:
	String() {}
	String(Str s): string(s) {}
	String(const String& s): string(s) {}
	String(const std::string& s): string(s) {}
	explicit String(int n);

	operator LPCSTR() const {return c_str();}
	String operator +(Str t) const {return std::string(*this).append(t);}
	friend String operator +(Str u, const String& v) {return std::string(u).append(v);}
	char operator[](int i) const {return string::operator[](i);}
	char& operator[](int i) {return string::operator[](i);}

	int GetLength() const {return size();}
	int Find(char c, int begin = 0) const;
	String Mid(int begin, int n = npos) const {return substr(begin, n);}
	String Left(int n) const {return substr(0, n);}
	void MakeUpper();
	void Format(Str format, ...);
};

/////////////////////////////////////////////////////////////////////////////

} // namespace JPN

#endif
