
#ifndef GSSUPPORT_H
#define GSSUPPORT_H

#include <windows.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

typedef vector<string> VString;

// STL Formatting
#define MAKE_STRING( msg )  ( ((std::ostringstream&)(std::ostringstream() << (string)"" << msg)).str() )
#define MAKE_CSTRING( msg )  ( ((std::ostringstream&)(std::ostringstream() << (string)"" << msg)).str().c_str() )

#ifdef _WIN64
	#define GSDLLNAME   "gsdll64.dll"
#else
	#define GSDLLNAME   "gsdll32.dll"
#endif

typedef int (* GSDLL_CALLBACK)        (int, char *, unsigned long);

typedef int (_stdcall * PF_GSRevision)(const char ** product, const char ** copyright, long * revision, long * revisiondate);
typedef int (_stdcall * PF_GSInit)    (GSDLL_CALLBACK, HWND, int argc, char * * argv);
typedef int (_stdcall * PF_GSBegin)   (void);
typedef int (_stdcall * PF_GSCont)    (const char * str, int len);
typedef int (_stdcall * PF_GSEnd)     (void);
typedef int (_stdcall * PF_GSExit)    (void);

class CGSSupport
{
public:
    CGSSupport ();
    virtual ~CGSSupport ();

    bool Init (const char *szIniFile, bool bQuiet, bool *pbBreak, string &strError);

    bool Start (int nArgs, char **argv, const char *pFileName, string &strError);
    bool Begin (string &strError);
    bool End   (string &strError);
    bool Exit  (string &strError);

    const char *DLLPath () 
    {
        return m_gsdllPath.c_str();
    }

    const char *LibPath ()
    {
        return m_gslibPath.c_str();
    }

    const char *Product () 
    {
        return m_strProduct.c_str();
    }

    const char *Copyright()
    {
        return m_strCopyright.c_str();
    }

    long Revision()
    {
        return m_lRevision;
    }

    long RevisionDate()
    {
        return m_lRevisionDate;
    }

private:
    PF_GSRevision   m_pfGSRevision;
    PF_GSInit       m_pfGSInit;
    PF_GSBegin      m_pfGSBegin;
    PF_GSCont       m_pfGSCont;
    PF_GSEnd        m_pfGSEnd;
    PF_GSExit       m_pfGSExit;
    string          m_gsdllPath;
    string          m_gslibPath;
    string          m_strProduct;
    string          m_strCopyright;
    long            m_lRevision;
    long            m_lRevisionDate;

    int             m_code;
    bool            m_bQuiet; // if false out a dot for every page

    HINSTANCE       m_hGSDLL;
};

#endif