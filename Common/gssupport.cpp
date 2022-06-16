#include <iomanip>
#include <iostream>
#include <time.h>
#include "gssupport.h"
#include "gsbatchprint.h"

#pragma warning (disable:4996)

#define GSDLL_STDIN 1	
#define GSDLL_STDOUT 2
#define GSDLL_DEVICE 3
#define GSDLL_SYNC 4
#define GSDLL_PAGE 5	
#define GSDLL_SIZE 6	
#define GSDLL_POLL 7
#define GSDLL_INIT_QUIT    101

// Ghostscript dll functions

bool dummyBreak = false;
bool *pbBreak = &dummyBreak;

string   strGSLogFile;
ofstream gsLogStream;
extern bool     bQuiet;

void Write2GSLogFile (const char *szMessage)
{
    if (szMessage != NULL && strGSLogFile.size () > 0)
    {
        gsLogStream << szMessage << endl;
        gsLogStream.flush ();
    }
}

CGSSupport::CGSSupport ()
{
    m_pfGSRevision = NULL;
    m_pfGSInit = NULL;
    m_pfGSBegin = NULL;
    m_pfGSCont = NULL;
    m_pfGSEnd  = NULL;
    m_pfGSExit = NULL;

    m_hGSDLL = NULL;
}

CGSSupport::~CGSSupport ()
{
    if (m_hGSDLL)
    {
        FreeLibrary (m_hGSDLL);
        m_hGSDLL = NULL;
    }
}

bool CGSSupport::Init (const char *szIniFile, bool bQuietIn, bool *pbBreakIn, string &strError)
{
    bool   bResult = false;
    string dllPath, strSource;
    const char *product;
    const char *copyright;
    char   buf [1000];

    bQuiet = m_bQuiet = bQuietIn;

    // canceling printing
    pbBreak = pbBreakIn;

    // from Init()-functions
    char exePath      [MAX_PATH] = {0};

    // Read the path, from the exe 
    GetModuleFileName(GetModuleHandle(NULL), exePath, _countof(exePath));

    char * p = strrchr (exePath, '\\');

    if (p != NULL)
        *(p + 1) = 0;
    else
        strcat (exePath, "\\\\");

    if (GetPrivateProfileString ("Ghostscript", "LogFile", NULL, buf, _countof(buf), szIniFile))
    {
        strGSLogFile = buf;
        if (strGSLogFile.size () > 0)
        {
            gsLogStream.clear ();
            gsLogStream.open (strGSLogFile.c_str (), ios::app);
            if (gsLogStream.fail ())
            {
                strGSLogFile.clear();
                strError = MAKE_STRING("The Ghostscript-Logfile '" << strGSLogFile << "' could not be opened");
                goto ErrorExit;
            }
        }
    }

    // first search in actual path
    dllPath = exePath;
    dllPath += GSDLLNAME;

    if (FileExists (dllPath.c_str()))
    {
        m_hGSDLL = LoadLibrary (GSDLLNAME);

        m_gslibPath = "lib;fonts;resource;bin;%SYSTEMROOT%\\fonts";

        if (m_hGSDLL)
        {
            strSource = "local path";
        }
    }

    if (!m_hGSDLL)
    {
        if (GetPrivateProfileString( "Ghostscript", "GS_DLL", NULL, buf, _countof (buf), szIniFile) > 0)
            m_gsdllPath = buf;

        if (GetPrivateProfileString( "Ghostscript", "GS_LIB", NULL, buf, _countof (buf), szIniFile) > 0)
            m_gslibPath = buf;

        // add actual path ?
        if (m_gsdllPath.size () > 2)
        {
            if (m_gsdllPath[1] != ':' && strncmp (m_gsdllPath.c_str (), "\\\\", 2) != 0)
            {
                m_gsdllPath = exePath + m_gsdllPath;
            }
        }
        m_hGSDLL = LoadLibrary (m_gsdllPath.c_str());

        if (m_hGSDLL)
        {
            strSource = MAKE_CSTRING("ini file:" << szIniFile);
        }

        if (!m_hGSDLL)
        {
            HKEY hKeyGS = NULL;
            char gsMaxVersion [100] = {0};
            char gsVersion [100];
            
            if (RegOpenKey (HKEY_LOCAL_MACHINE, "Software\\GPL Ghostscript", &hKeyGS) != ERROR_SUCCESS)
            {
                strError = "Ghostscript DLL not found";
                return false;
            }
            // Enum all subkeys
            for (DWORD i = 0; ; i++)
            {
                if (RegEnumKey (hKeyGS, i, gsVersion, _countof (gsVersion)) != ERROR_SUCCESS)
                    break;
                
                if (!*gsMaxVersion || atof (gsVersion) > atof (gsMaxVersion))
                {
                    strcpy (gsMaxVersion, gsVersion);
                }
            }
            // Open Subkey if a version was fouind
            if (*gsMaxVersion)
            {
                HKEY hSubKey = NULL;

                if (RegOpenKey (hKeyGS, gsMaxVersion, &hSubKey) == ERROR_SUCCESS)
                {
                    DWORD dwType, dwSize;
                    char  path[MAX_PATH];

                    dwSize = _countof (path);
                    if (RegQueryValueEx (hSubKey, "GS_DLL", NULL, &dwType, (LPBYTE)path, &dwSize) == ERROR_SUCCESS)
                    {
                        m_gsdllPath = path;
                        dwSize = _countof (path);
                        if (RegQueryValueEx (hSubKey, "GS_LIB", NULL, &dwType, (LPBYTE)path, &dwSize) == ERROR_SUCCESS)
                        {
                            m_gslibPath = path;

                            // Try to reload
                            m_hGSDLL = LoadLibrary (m_gsdllPath.c_str());
                            if (m_hGSDLL)
                            {
                                strSource = MAKE_STRING("registry. Version:" << gsMaxVersion);
                            }
                        }
                    }

                    RegCloseKey (hSubKey);
                }
            }

            RegCloseKey (hKeyGS);
        }

        if (!m_hGSDLL)
        {
            strError = "Ghostscript DLL not found";
            return false;
        }
    }
    // 
    if (m_hGSDLL)
    {
        // the DLL was loaded successfully
        m_pfGSRevision = (PF_GSRevision) GetProcAddress(m_hGSDLL, "gsdll_revision");
        if (!m_pfGSRevision) 
        {
            strError = "Can't find gsdll_revision";
            return false;
        }

        m_pfGSInit = (PF_GSInit) GetProcAddress(m_hGSDLL, "gsdll_init");
        if (!m_pfGSInit) 
        {
            strError = "Can't find gsdll_init";
            return false;
        }
        m_pfGSBegin = (PF_GSBegin) GetProcAddress(m_hGSDLL, "gsdll_execute_begin");
        if (!m_pfGSBegin) 
        {
            strError = "Can't find gsdll_execute_begin";
            return false;
        }
        m_pfGSCont = (PF_GSCont) GetProcAddress(m_hGSDLL, "gsdll_execute_cont");
        if (!m_pfGSCont) 
        {
            strError = "Can't find gsdll_execute_cont";
            return false;
        }
        m_pfGSEnd = (PF_GSEnd) GetProcAddress(m_hGSDLL, "gsdll_execute_end");
        if (!m_pfGSEnd) 
        {
            strError = "Can't find gsdll_execute_end";
            return false;
        }
        m_pfGSExit = (PF_GSExit) GetProcAddress(m_hGSDLL, "gsdll_exit");
        if (!m_pfGSExit) 
        {
            strError = "Can't find gsdll_exit";
            return false;
        }
        // get the ghostscript revision.
        if (m_pfGSRevision(&product, &copyright, &m_lRevision, &m_lRevisionDate) != 0) 
        {
            strError = "Unable to identify Ghostscript DLL revision - it must be newer than needed.";
            goto ErrorExit;
        }
        m_strProduct = product;
        m_strCopyright = copyright;
    }

    // Expand the pathes, if they are relative
    if (!m_gslibPath.empty ())
    {
        VString strArray;
        string  strError;

        if (SplitStringSTL (m_gslibPath.c_str (), strArray, ';', strError))
        {
            for (VString::iterator it = strArray.begin (); it != strArray.end (); it++)
            {
                char buf [1000];
                string &s = *it;

                // Expand environment string
                ExpandEnvironmentStrings (s.c_str (), buf, _countof (buf));
                s = buf;

                if (s.size () <= 2 || (strncmp (s.c_str(), "\\\\", 2) != 0 && s[1] != ':'))
                {
                    // relative path
                    s = exePath + s;
                }
            }
            m_gslibPath = VString2String(strArray);
        }
    }


    if (m_hGSDLL)
    {
        Write2GSLogFile (MAKE_CSTRING("Ghostscript DLL found! Source:" << strSource << endl << "Dll path:" << m_gsdllPath << endl << "Lib path:" << m_gslibPath << endl));
    }

    bResult = true;
ErrorExit:
    return bResult;
}

int gsdll_callback(int message, char *str, unsigned long count)
{
    char *p;

    switch (message) {
        case GSDLL_STDIN:
            p = fgets(str, count, stdin);
            if (p)
                return (int)strlen(str);
            else
                return 0;
        case GSDLL_STDOUT:
            if (str != (char *)NULL)
            {
                string s (str, count);
                if (!bQuiet && s.find ("Page") != string::npos)
                {
                    cout << ".";
                    cout.flush ();
                }
                if (strGSLogFile.size () > 0)
                {
                    gsLogStream << s;
                    gsLogStream.flush ();
                }
            }
            return count;
        case GSDLL_DEVICE:
            // fprintf(stdout,"Callback: DEVICE %p %s\n", str,
            //     count ? "open" : "close");
            break;
        case GSDLL_SYNC:
            // fprintf(stdout,"Callback: SYNC %p\n", str);
            break;
        case GSDLL_PAGE:
            // fprintf(stdout,"Callback: PAGE %p\n", str);
            break;
        case GSDLL_SIZE:
            fprintf(stdout,"Callback: SIZE %p width=%d height=%d\n", str,
                (int)(count & 0xffff), (int)((count>>16) & 0xffff) );
            break;
        case GSDLL_POLL:
            return *pbBreak ? 1: 0; /* no error */
        default:
            fprintf(stdout,"Callback: Unknown message=%d\n",message);
            break;
    }
    return 0;
}

bool CGSSupport::Start (int nArgs, char **argv, const char *pFileName, string &strError)
{
    // Output parameter
    if (strGSLogFile.size () > 0)
    {
        char timebuf [22];
        time_t now = time(NULL);

        strftime (timebuf, _countof(timebuf), "%Y/%m/%d %H:%M:%S ", localtime(&now));

        gsLogStream << "------------------------------------" << endl;
        gsLogStream << timebuf << pFileName << endl;
        for (int j = 0; j < nArgs; j++)
        {
            gsLogStream << setw(2) << j << ":" << argv[j] << endl;
        }
        gsLogStream.flush ();
    }

    m_code = (m_pfGSInit)(gsdll_callback, NULL, nArgs, (char **)argv);

    if (m_code != 0 && m_code != GSDLL_INIT_QUIT)
    {
        strError = MAKE_STRING("error calling gsdll_Init! Resultcode:" << m_code);
        if (m_code <= -100)
            m_pfGSExit ();

        return false;
    }
    return true;
}

bool CGSSupport::Begin (string &strError)
{
    m_pfGSBegin();
    return true;
}

bool CGSSupport::End   (string &strError)
{
    m_pfGSEnd();
    return true;
}

bool CGSSupport::Exit  (string &strError)
{
    if (m_code == 0)
        m_pfGSExit ();
    return true;
}
