
#ifndef _GSBATCHPRINT_H
#define _GSBATCHPRINT_H

// #include <windows.h>
#include <string>
#include <iterator>
#include <vector>
#include <list>
#include <sstream>
#include <algorithm>

using namespace std;

#pragma warning (disable:4996)

typedef vector<string> VString;
typedef list<string>   LString;

// STL Formatting
#define MAKE_STRING( msg )  ( ((std::ostringstream&)(std::ostringstream() << (string)"" << msg)).str() )
#define MAKE_CSTRING( msg )  ( ((std::ostringstream&)(std::ostringstream() << (string)"" << msg)).str().c_str() )


typedef struct
{
    string strBinName;
    WORD   wBinCode;
} SBin2Code;

// for CFileData
extern bool bSortByName;        // -n
extern bool bReverseSorting;    // -r

// information about files and sorting algorithm
class CFileData
{
public:
    CFileData (string fileName, FILETIME ft)
    {
        strFileName = fileName;
        filetime    = ft;
    }

    static int compare( const CFileData &fl1, const CFileData &fl2) 
    {
        int result;

        if (!bSortByName)
        {
            LONGLONG fltime1 = ((LONGLONG)(fl1.filetime.dwHighDateTime) << 32) + fl1.filetime.dwLowDateTime;
            LONGLONG fltime2 = ((LONGLONG)(fl2.filetime.dwHighDateTime) << 32) + fl2.filetime.dwLowDateTime;

            if (fltime1 < fltime2)
                return -1;
            else if (fltime1 > fltime2)
                return 1;
            else
                return 0;
        }
        else
        {
            result = stricmp (fl1.strFileName.c_str (), fl2.strFileName.c_str ());
        }

        return bReverseSorting ? -result :result;
    }

    // need a weak comparision function. May not 
    bool operator <(const CFileData &f1)
    {
        bool result;

        LONGLONG fthistime = ((LONGLONG)(filetime.dwHighDateTime) << 32) + filetime.dwLowDateTime;
        LONGLONG f1time    = ((LONGLONG)(f1.filetime.dwHighDateTime) << 32) + f1.filetime.dwLowDateTime;
        if (!bSortByName)
        {
            if (fthistime == f1time)
                result = strFileName < f1.strFileName;
            else
                result = fthistime < f1time;
        }
        else
        {
            if (strFileName == f1.strFileName)
                result = fthistime < f1time;
            else
                result = strFileName < f1.strFileName;
        }
        return bReverseSorting ? !result : result;
    }
    string strFileName;
    FILETIME filetime;
};


// For log output
typedef enum LogTypes {Ok = 0, Warning=1, Error=2, Dbg=3} ELogTypes;

//  Common global variable
extern ostream *pOutStream;
extern const char *pUsageString;
extern bool bQuiet;
extern bool bBreak;

// Common functions
extern const char *GetFileExtFromFilename (LPCTSTR pFileName);
extern const char *GetFileNoExtFromFilename (const char *pFileName);
extern bool   FileExists (const char *pFile);
extern bool   SplitStringSTL (const char *pString, VString &strArray, char chDelim, string &strError);
extern string StringWithoutBlanks (const char *pValue);
extern string VString2String(VString value);
extern string WinErrMsg (DWORD dwError);
extern char * DupString (const char * pString);
extern void   Write2LogFile (ELogTypes logType, const char *szFile, const char *szMessage);

extern int GSBatchMain (char *commandLine, string &strShowError);
extern void Usage ();
#endif