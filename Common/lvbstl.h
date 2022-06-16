/// Definitionen für die Benutzung der STL
#ifndef STL_DEFINE_H
#define STL_DEFINE_H

#pragma warning (disable : 4786)
         
#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <algorithm>

using namespace std;


#if defined (UNICODE) || defined (_UNICODE)
   typedef wstring ustring;
#else
   typedef string  ustring;
#endif

typedef list   <ustring> StringList;
typedef vector <ustring> StringArray;
typedef set    <ustring> StringSet;
typedef set    <wstring> WStringSet;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(n) (sizeof(n)/sizeof(n[0]))
#endif

#ifndef MAKE_STRING
// direct formatting with  streamstream
// example: wstring str = MAKE_STRING (L"This is a number." << 34);
#if defined (UNICODE) || defined (_UNICODE)
    #define MAKE_STRING( msg )  ( ((std::wostringstream&)(std::wostringstream() << (ustring)_T("") << msg)).str() )
    #define MAKE_CSTRING( msg )  ( ((std::wostringstream&)(std::wostringstream() << (ustring)_T("") << msg)).str().c_str() )
#else
    #define MAKE_STRING( msg )  ( ((std::ostringstream&)(std::ostringstream() << (ustring)_T("") << msg)).str() )
    #define MAKE_CSTRING( msg )  ( ((std::ostringstream&)(std::ostringstream() << (ustring)_T("") << msg)).str().c_str() )
#endif
#endif

/*
// formating stl string. Don't know, if it is used anymore
int  STLFormat    (ustring &str, LPCTSTR pFormat, ...);
void STLTrimLeft  (ustring &str);
void STLTrimRight (ustring &str);
void STLTrim      (ustring &str);
*/


#endif
