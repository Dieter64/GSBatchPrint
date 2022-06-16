#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <iomanip>
#include <winspool.h>

#include "gsbatchprint.h"
#include "gssupport.h"

typedef vector <SBin2Code> VBin2Code;
typedef vector <CFileData> VFileData;


const char *szIniFileName = "gsBatchPrint.ini";


const char *pUsageString = 
"gsbatchprint <optional options> /P Printer /F PDF-Filepattern ... /F PDF-Filepattern\r\n\r\n"
"Prints one or more pdf-files to a windows printer.\r\n"
"All options with upper case letters expect an additional parameter.\r\n\r\n"
"/P\tis the name of a local or network printer\r\n"
"/F\tuse file pattern from the actual path (e.g. a*.pdf), from subpathes\r\n\t(e.g. file\\*.pdf), or full pathes (e.g. c:\\temp\\*.pdf)\r\n"
"All files will be collected and printed in ascending order according to the file date.\r\n\r\n"
"Sort options:\r\n"
"/n\tSort files by name and not by date\r\n"
"/r\tSort in descending order. This works for name and date sorting.\r\n\r\n"
"Color options:\r\n"
"/C\t0 to print mono on color printers. 1 to print in color (default).\r\n"
"/B\tBits per Pixel on color printers. Can be 1, 4, 16 or 32.\r\n\r\n"
"Paper options:\r\n"
"/N\tUse the given paper size. Can be numeric e.g. '/N 9' or text '/N A3'.\r\n\tUse /s to view available paper sizes.\r\n"
"/O\tOrientation. '1' or starting with 'port' for portraint,\r\n\t'2' or starting with 'land' for landscape.\r\n"
"/I\tinput paper bin. Numeric or text possible. Can be defined more than\r\n\tonce. e.g. '/I tray1 /I schacht1'. Comparison without blanks.\r\n\r\n"
"Batch options:\r\n"
"/l\tLooping. Stop with Ctrl+C (only console version).\r\n\tUse /M and /E to prevent endless printing!\r\n"
"/M\tMove correct printed files to the given destination path.\r\n"
"/D\tDelete the source file instead of moving it.\r\n"
"/E\tMove incorrect printed files to the given destination path.\r\n"
"/T\tAmount of time to sleep after a loop in seconds. e.g. '/T 10'\r\n\r\n"
"Ghostscript options:\r\n"
"/G\tCommand line parameter for calling ghostscript. \r\n\te.g. /G /dBATCH (just an example - batch option is always included)\r\n"
"/Z\tOptions for printer device. e.g /Z \"/BitsPerPixel 16\"\r\n"
"/U\tOptions for mswinpr2 ghostscript device. e.g /U \"/Color 1\" for\r\n\tmonochrom 1, color 2 (0=default).\r\n\r\n"
"Additional options:\r\n"
"/R\tResolution for ghostscript rastering. Be careful with high values\r\n\t->Huge spool files! (for color printers)\r\n"
"/q\tQuiet! No output to stdout.\r\n"
"/s\tDont print. Just show printer information for the given printer.\r\n\tNo files needed.\r\n"
"/t\tTest. No printing. Just output the files, that would be printed.\r\n\r\n"
"Examples:\r\n"
"---------\r\n"
"Looping:\r\n  gsbatchprint -T 5 -l -E c:\\printed\\error -M c:\\printed\\ok -F *.pdf -P Printer\r\n\r\n"
"Paper Size:\r\n  gsbatchprint -B schacht1 -O Portrait -N A4 -F *.pdf -P Printer\r\n\r\n"
"Printer Information (paper sizes and bins):\r\n  gsbatchprint -s -P Printer\r\n\r\n";


// 
const char *LogTypeMessages[] =
{
    "OK",
    "WARNING",
    "ERROR",
    "DEBUG",
};


// from Init()-functions
char exePath      [MAX_PATH] = {0};

// generated full name of the inifile
string strIniFileName;
string strLogFile;                 

// commandline parameter

// with values
string  strPrinterName;             // -P  
VString vPDFFilePattern;            // -F  
VString vBinParameters;             // -I  
VString vPaperParameters;           // -N  
string  strMoveOkPath;              // -M  
string  strMoveErrPath;             // -E  
string  strColorPrint;              // -C
string  strBitsPerPixel;            // -B

// additional ghostscript parameter from commandline and ini-file
LString listGSOptions;              // -G  
LString listUserDefinedOptions;     // -U 
LString listPrinterOptions;	    // -Z
int     maxResolution    = -1;      // -R  
int     loopTime = 30;              // -T looptime for -l in seconds .
int     nOrientation = 0;           // -O Portrait or 1, Landscape or 2, 0 no entry

// boolean values
bool    bSortByName = false;        // -n 
bool    bReverseSorting = false;    // -r 
bool    bQuiet = false;             // -q 
bool    bShowBins = false;          // -s 
bool    bLoop = false;              // -l 
bool    bTest = false;              // -t 
bool    bDeleteSource = false;      // -D
bool    bReplaceSlashes = false;    // only ini

bool    bBreak = false;             // Ctrl+C

VFileData vFileData;
ofstream  logStream;

bool WalkThroughCommandLine (char * pCmdLine, string &strError);


void Write2LogFile (ELogTypes logType, const char *szFile, const char *szMessage)
{
    if (szFile != NULL && szMessage != NULL && strLogFile.size () > 0)
    {
        char timebuf [22];
        time_t now = time(NULL);

        strftime (timebuf, _countof(timebuf), "%Y/%m/%d %H:%M:%S", localtime(&now));

        logStream << timebuf << ";" << LogTypeMessages[(int)logType] << ";" << szFile << ";" << szMessage << endl;
    }
}

bool SetOrientation (const char *pParam, string &strError)
{
    if (strnicmp (pParam, "port", 4) == 0)
        nOrientation = 1;
    else if (strnicmp (pParam, "land", 4) == 0)
        nOrientation = 2;
    else
        nOrientation = atol (pParam);

    if (nOrientation < 0 || nOrientation > 2)
    {
        strError = MAKE_STRING("Invalid orientation option -O'");
        return false;
    }
    return true;
}

bool SetColor (const char *pParam, string &strError)
{
    strColorPrint = pParam;
    if (strcmp (pParam, "0") != 0 && strcmp (pParam, "1") != 0)
    {
        strError = MAKE_STRING("expect 0 or 1 for color value (option /C)");
        return false;
    }
    return true;
}

bool SetBitsPerPixel (const char *pParam, string &strError)
{
    int nBpp = atol (pParam);
    if (nBpp != 1 && nBpp != 4 && nBpp != 8 && nBpp != 32)
    {
        strError = MAKE_STRING("wrong bits per pixel value. Expect 1, 4, 8 or 32");
        return false;
    }
    strBitsPerPixel = pParam;
    return true;
}

bool CommandLineParam (int nPos, char * pParam, string &strError)
{
    static char lastOption = 0;

    int nLen = (int)strlen (pParam);

    if (nPos == 0 && *pParam != '/' && *pParam != '-')
        return true;

    if (lastOption == 0 && (*pParam == '-' || *pParam == '/'))
    {
        char chOption = *(pParam + 1);

        if (strchr ("CIONPFBLMGURTZE", chOption) != NULL)
        {
            lastOption = chOption;
        }
        else
        {
            // single charachter options
            switch (chOption)
            {
            case 'r':
                bReverseSorting = true;
                break;
            case 'n':
                bSortByName = true;
                break;
            case 's':
                bShowBins = true;
                break;
            case 'q':
                bQuiet = true;
                break;
            case 'D':
            case 'd':
                bDeleteSource = true;
				break;
            case 'l':
                bLoop = true;
                break;
            case 't':
                bTest = true;
                break;
            case '?':
            case 'H':
            case 'h':
                Usage ();
                return false;
            default:
                strError = MAKE_STRING("Unknown option '-" <<lastOption <<"'. Position: " << nPos+1 << " Parameter:" << pParam );
    
                strError = MAKE_STRING("Unbekannter Kommandozeilenschalter: '" << chOption << "'");
                return false;
            }
        }
    }
    else
    {
        switch (lastOption)
        {
        case 'O':
            if (!SetOrientation (pParam, strError))
                return false;
            break;
        case 'P':
            strPrinterName = pParam;
            break;
        case 'F':
            vPDFFilePattern.push_back (pParam);
            break;
        case 'C':
            if (!SetColor (pParam, strError))
                return false;
            break;
        case 'B':
            if (!SetBitsPerPixel (pParam, strError))
                return false;
            break;
        case 'I':
            vBinParameters.push_back (pParam);
            break;
        case 'N':
            vPaperParameters.push_back (pParam);
            break;
        case 'G':
            listGSOptions.push_back (pParam);
            break;
        case 'R':
            maxResolution = atol (pParam);
            break;
        case 'T':
            loopTime = atol(pParam);
            break;
        case 'U':
            listUserDefinedOptions.push_back (pParam);
            break;
        case 'Z':
            listPrinterOptions.push_back (pParam);
            break;
        case 'M':
            strMoveOkPath = pParam;
            break;
        case 'E':
            strMoveErrPath = pParam;
            break;
        default:
            strError = MAKE_STRING("Unknown option. Position: " << nPos+1 << " Parameter:" << pParam );
            return false;
        }
        lastOption = 0;
    }
    return true;
}


// Read some values
bool ReadIniFileParameter (string &strError)
{
    char buf [5000];
    char key [100];
    int  i;

    // Read the path, from the exe 
    GetModuleFileName(GetModuleHandle(NULL), exePath, _countof(exePath));

    char * p = strrchr (exePath, '\\');

    if (p != NULL)
        *p = 0;

    // name of the ini
    strIniFileName = MAKE_CSTRING(exePath << "\\" << szIniFileName);

    for (i = 0; i < 100; i++)
    {
        sprintf (key, "Arg%d", i);
        if (GetPrivateProfileString( "Ghostscript", key, NULL, buf, _countof (buf), strIniFileName.c_str ()) > 0)
            listGSOptions.push_back (buf);
    }

    for (i = 0; i < 100; i++)
    {
        sprintf (key, "UserArg%d", i);
        if (GetPrivateProfileString( "Ghostscript", key, NULL, buf, _countof (buf), strIniFileName.c_str ()) > 0)
            listUserDefinedOptions.push_back (buf);
    }

    for (i = 0; i < 100; i++)
    {
        sprintf (key, "PrinterArg%d", i);
        if (GetPrivateProfileString( "Ghostscript", key, NULL, buf, _countof (buf), strIniFileName.c_str ()) > 0)
            listPrinterOptions.push_back (buf);
    }

    if (GetPrivateProfileString ("GsBatchPrint", "Printer", NULL, buf, _countof(buf), strIniFileName.c_str ()))
        strPrinterName = buf;

    if (GetPrivateProfileString ("GsBatchPrint", "BitsPerPixel", NULL, buf, _countof(buf), strIniFileName.c_str ()))
    {
        if (!SetBitsPerPixel (buf, strError))
            return false;
    }

    if (GetPrivateProfileString ("GsBatchPrint", "Color", NULL, buf, _countof(buf), strIniFileName.c_str ()))
    {
        if (!SetColor (buf, strError))
            return false;
    }
    if (GetPrivateProfileString ("GsBatchPrint", "Paper", NULL, buf, _countof(buf), strIniFileName.c_str ()))
        vPaperParameters.push_back (buf);

    if (GetPrivateProfileString ("GsBatchPrint", "Orientation", NULL, buf, _countof(buf), strIniFileName.c_str ()))
    {
        if (!SetOrientation (buf, strError))
            return false;
    }

    if (GetPrivateProfileString ("GsBatchPrint", "Resolution", NULL, buf, _countof(buf), strIniFileName.c_str ()))
        maxResolution = atol(buf);

    if (GetPrivateProfileString ("GsBatchPrint", "LogFile", NULL, buf, _countof(buf), strIniFileName.c_str ()))
        strLogFile = buf;

    if (GetPrivateProfileString ("GsBatchPrint", "MoveOkPath", NULL, buf, _countof(buf), strIniFileName.c_str ()))
        strMoveOkPath = buf;

    if (GetPrivateProfileString ("GsBatchPrint", "Delete", NULL, buf, _countof(buf), strIniFileName.c_str ()))
		bDeleteSource = (*buf == '1');

    if (GetPrivateProfileString ("GsBatchPrint", "MoveErrPath", NULL, buf, _countof(buf), strIniFileName.c_str ()))
        strMoveErrPath = buf;

    bReplaceSlashes = (GetPrivateProfileInt ("GsBatchPrint", "ReplaceSlashes", 0, strIniFileName.c_str ())) != 0;

    return true;
}


bool CheckParameters (string &strError)
{
    if (strMoveOkPath.size() > 0)
    {
        if (strMoveOkPath[strMoveOkPath.size () - 1] != '\\')
            strMoveOkPath += "\\";
        CreateDirectory (strMoveOkPath.c_str (), NULL);
    }

    if (strMoveErrPath.size() > 0)
    {
        if (strMoveErrPath[strMoveErrPath.size () - 1] != '\\')
            strMoveErrPath += "\\";
        CreateDirectory (strMoveErrPath.c_str (), NULL);
    }

    return true;
}

void SortFiles ()
{
    sort (vFileData.begin (), vFileData.end ());
}

void DetectPrintingFiles()
{
    VString::iterator itFilePattern;

    vFileData.erase (vFileData.begin (), vFileData.end());

    for (itFilePattern = vPDFFilePattern.begin (); itFilePattern != vPDFFilePattern.end(); itFilePattern++)
    {
        WIN32_FIND_DATA FindFileData;
        HANDLE          hFind = NULL;;
        string strPDFFilePattern = *itFilePattern;
        string path, pattern;
        bool   bAppendOwnPath = false;

        size_t posSlash = strPDFFilePattern.rfind ('\\');
        if (posSlash != string::npos)
        {
            path = strPDFFilePattern.substr (0, posSlash + 1);
            pattern = strPDFFilePattern.substr (posSlash + 1);
            bAppendOwnPath =  true;
        }
        else
        {
            pattern = strPDFFilePattern;
        }

        // Expand PDF files
        hFind = FindFirstFile(strPDFFilePattern.c_str (), &FindFileData);
        if (hFind != INVALID_HANDLE_VALUE) 
        {
            char buf [MAX_PATH];
            do
            {
                string file;

                if (bAppendOwnPath)
                {
                    // if file is not an absolulte path, add the current path
                    if (path.size () > 2 && (path [1] == ':' || (path[0] == '\\' && path[1] == '\\')))
                    {
                        // absolute path, just add
                        file = MAKE_STRING( path << FindFileData.cFileName);
                    }
                    else
                    {
                        GetCurrentDirectory (_countof (buf), buf);

                        file = MAKE_STRING (buf << "\\" << path << FindFileData.cFileName);
                    }
                }
                else
                {
                    GetFullPathName (FindFileData.cFileName, _countof(buf), buf, NULL);
                    file = buf;
                }
                CFileData filedata (file, FindFileData.ftLastWriteTime);

                vFileData.insert (vFileData.end(), filedata);
            }
            while (FindNextFile(hFind, &FindFileData) != 0);
            FindClose(hFind);
        }
    }
}

// returns all binname/code combinations in a vector
bool GetPrinterPapers (const char *pPrinter, VBin2Code &vPaper2Code, string &strError)
{
    HANDLE hPrinter = NULL;
    bool   bResult = false;
    char   *pPaperNames = NULL;
    WORD   *pwPaperCodes = NULL;
    char   onePaper[65];

    if (!OpenPrinter ((char *)pPrinter, &hPrinter, NULL))
    {
        strError = MAKE_STRING ("Could not open the printer '" << pPrinter << "'. Error msg:" << WinErrMsg (GetLastError()));
        goto ErrorExit;
    }

    vPaper2Code.clear ();

    int nPapers = DeviceCapabilities (pPrinter, NULL, DC_PAPERNAMES, NULL, NULL);
    if (nPapers <= 0)
    {
		nPapers = 0;
		// Plotters seams not to support plotters. Just ignore that problem and return zero papers
		/*
        strError = MAKE_STRING ("Could not detect the number of input bins for the printer '" << pPrinter << "'. Error msg:" << WinErrMsg (GetLastError ()));
        goto ErrorExit;
		*/
    }
	pPaperNames  = nPapers > 0 ? new char [nPapers * 64] : NULL;
	pwPaperCodes = nPapers > 0 ? new WORD [nPapers] : NULL;

	if (nPapers > 0)
	{
		DeviceCapabilities (pPrinter, NULL, DC_PAPERNAMES, (LPSTR) pPaperNames,  NULL);
		DeviceCapabilities (pPrinter, NULL, DC_PAPERS,     (LPSTR) pwPaperCodes, NULL);

		for (int i = 0; i < nPapers; i++)
		{
			// if the name is exactly 24 characters no \0 is appended
			memcpy (onePaper, (char *)(pPaperNames + i * 64), 64);
			onePaper[64] = 0;

			SBin2Code b2c = {onePaper, pwPaperCodes[i]};

			vPaper2Code.push_back (b2c);
		}
	}

    bResult = true;
ErrorExit:
    if (hPrinter != NULL)
    {
        ClosePrinter (hPrinter);
        hPrinter = NULL;
    }
	if (pPaperNames)
		delete [] pPaperNames;

	if (pwPaperCodes)
		delete [] pwPaperCodes;

    return bResult;
}


// returns all binname/code combinations in a vector
bool GetPrinterBins (const char *pPrinter, VBin2Code &vBin2Code, string &strError)
{
    HANDLE hPrinter = NULL;
    bool   bResult = false;
    char   *pBinNames = NULL;
    WORD   *pwBinCodes = NULL;
    char   onebin[25];

    if (!OpenPrinter ((char *)pPrinter, &hPrinter, NULL))
    {
        strError = MAKE_STRING ("Could not open the printer '" << pPrinter << "'. Error msg:" << WinErrMsg (GetLastError()));
        goto ErrorExit;
    }

    vBin2Code.clear ();

    int nBins = DeviceCapabilities (pPrinter, NULL, DC_BINNAMES, NULL, NULL);
    if (nBins == -1)
    {
		nBins = 0;
		// Plotters seams not to support plotters. Just ignore that problem and return zero papers
		/*
        strError = MAKE_STRING ("Could not detect the number of papers for the printer '" << pPrinter << "'. Error msg:" << WinErrMsg (GetLastError ()));
        goto ErrorExit;
		*/
    }
	pBinNames  = nBins > 0 ? new char [nBins * 24] : NULL;
	pwBinCodes = nBins > 0 ? new WORD [nBins] : NULL;

	if (nBins > 0)
	{
		DeviceCapabilities (pPrinter, NULL, DC_BINNAMES, (LPSTR) pBinNames,  NULL);
		DeviceCapabilities (pPrinter, NULL, DC_BINS,     (LPSTR) pwBinCodes, NULL);

		for (int i = 0; i < nBins; i++)
		{
			// if the name is exactly 24 characters no \0 is appended
			memcpy (onebin, (char *)(pBinNames + i * 24), 24);
			onebin[24] = 0;

			SBin2Code b2c = {onebin, pwBinCodes[i]};

			vBin2Code.insert (vBin2Code.end(), b2c);
		}
	}
    bResult = true;
ErrorExit:
    if (hPrinter != NULL)
    {
        ClosePrinter (hPrinter);
        hPrinter = NULL;
    }
	if (pBinNames)
		delete [] pBinNames;
	if (pwBinCodes)
		delete [] pwBinCodes;

    return bResult;
}

void OutputBinsFile (const char *pPrinter, VBin2Code &vBin2Code, VBin2Code &vPaper2Codes)
{
    string filename ("PrinterInfo.txt");
    
    ofstream ofile(filename.c_str (), ios::app);
    ofile << "Printer:" << pPrinter << endl;
    ofile << "----------- input bins ----------------------------------------" << endl;

    VBin2Code::iterator it;

    for (it = vBin2Code.begin (); it != vBin2Code.end (); it++)
    {
        ofile << setw(5) << it->wBinCode << ": " << it->strBinName.c_str () << endl;
    }
    ofile << "----------- paper names ---------------------------------------" << endl;
    for (it = vPaper2Codes.begin (); it != vPaper2Codes.end (); it++)
    {
        ofile << setw(5) << it->wBinCode << ": " << it->strBinName.c_str () << endl;
    }
    ofile << "---------------------------------------------------" << endl << endl;

    ofile.close ();

    ShellExecute (NULL, "open", filename.c_str (), NULL, NULL, SW_SHOW);
    
}

string Convert2GSPrinter (const char *pNormalPrinterName)
{
    HANDLE hPrinter = NULL;
    PRINTER_INFO_2 *pPrinterInfo = NULL;
    string strNewPrinterName, strReturnName;
    DWORD dwNeeded = 0;

    // if no networkprinter nothing to do
    if (strchr (pNormalPrinterName, '\\') == NULL)
        return pNormalPrinterName;

    if (!OpenPrinter ((char *)pNormalPrinterName, &hPrinter, NULL))
        return pNormalPrinterName; // this should be impossible, since the printer is opened before

    // Has to replace Sharename of the printer  by the printer name, so that ghostscript understand
    strNewPrinterName = pNormalPrinterName;

    // it is a network printer
    GetPrinter (hPrinter, 2, NULL, 0, &dwNeeded);
    pPrinterInfo = (PRINTER_INFO_2 *)new byte [dwNeeded];
    GetPrinter (hPrinter, 2, (LPBYTE)pPrinterInfo, dwNeeded, &dwNeeded);

    const char *pNormalPrinterShareName = strchr (pNormalPrinterName + 2, '\\');

    if (pNormalPrinterShareName != NULL)
    {
        pNormalPrinterShareName++;

        // if it is the sharename of the printer, replace it by the Printer name
        if (stricmp (pNormalPrinterShareName, pPrinterInfo->pShareName) == 0)
        {
            strNewPrinterName = pPrinterInfo->pPrinterName;
        }
    }
    // Duplicate all backslashes
    for (string::iterator it = strNewPrinterName.begin (); it != strNewPrinterName.end (); it++)
    {
        if (*it == '\\')
            strReturnName += "\\\\";
        else
            strReturnName.push_back (*it);
    }

    delete [] pPrinterInfo;
    ClosePrinter (hPrinter);
    return strReturnName;
}

// searchs in the vector vBin2Code for the corresponding value. 
// pInputBinOrCode can be a string name or a code
bool GetInputBinCode (VBin2Code &vBin2Code, VString vBinParameter, WORD &wBinCode)
{
    bool bBinFound = false;
    bool bOnlyDigits = true;
    VBin2Code::iterator itBinCode;
    VString::iterator   itBinParam;

    for (int loop = 0; loop < 3; loop++)
    {
        for (itBinParam = vBinParameter.begin (); itBinParam != vBinParameter.end(); itBinParam++)
        {
            string bin2Compare = (loop == 0) ? itBinParam->c_str () : StringWithoutBlanks (itBinParam->c_str ());

            // Search for the entries
            for (const char *pTest = bin2Compare.c_str (); *pTest; pTest++)
            {
                if (*pTest <'0' || *pTest > '9')
                {
                    bOnlyDigits = false;
                    break;
                }
            }
            // a Number ?
            if (bOnlyDigits)
            {
                if (loop == 0)
                {
                    WORD w = (WORD)atol (bin2Compare.c_str ());
                    for (itBinCode = vBin2Code.begin (); itBinCode != vBin2Code.end (); itBinCode++)
                    {
                        if (itBinCode->wBinCode == w)
                        {
                            wBinCode = w;
                            return true;
                        }
                    }
                }
            }
            else
            {
                for (itBinCode = vBin2Code.begin (); itBinCode != vBin2Code.end (); itBinCode++)
                {
                    switch (loop)
                    {
                    case 0:
                        // Total identical
                        if (_stricmp (itBinCode->strBinName.c_str (), bin2Compare.c_str ()) == 0)
                        {
                            wBinCode = itBinCode->wBinCode;
                            return true;
                        }
                        break;
                    case 1:
                        {
                            // identical without blanks
                            string codeW = StringWithoutBlanks (itBinCode->strBinName.c_str ());
                            if (_stricmp (codeW.c_str (), bin2Compare.c_str ()) == 0)
                            {
                                wBinCode = itBinCode->wBinCode;
                                return true;
                            }
                        }
                        break;
                    case 2:
                        {
                            // contained
                            string codeWC = StringWithoutBlanks (itBinCode->strBinName.c_str ());
                            if (strstr (codeWC.c_str (), bin2Compare.c_str ()) != NULL)
                            {
                                wBinCode = itBinCode->wBinCode;
                                return true;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    return false;
}

bool MoveFileWithSuffix (const char *pSrcFullFile, const char *pFileName, const char *pDstPath, string &strError)
{
    // Start with original name
    string strDstFile = MAKE_STRING (pDstPath << pFileName);
    if (!FileExists (strDstFile.c_str ()))
    {
        if (!MoveFile (pSrcFullFile, strDstFile.c_str ()))
        {
            strError = WinErrMsg (GetLastError ());
            return FALSE;
        }
        return true;
    }
    // Need to split the filename to append a number
    string strFileBaseName = GetFileNoExtFromFilename (pFileName);
    string strFileExt      = GetFileExtFromFilename (pFileName);

    // simple file exists, then append numbers. Test simple increment. If it takes too long, 
    int i = 0;
    while (i < 10000)
    {
        strDstFile = MAKE_STRING (pDstPath << strFileBaseName << i);

        if (!strFileExt.empty ())
            strDstFile += MAKE_STRING("." << strFileExt);

        if (!FileExists (strDstFile.c_str ()))
        {
            // if (!MoveFile (pSrcFullFile, strDstFile.c_str ()))
            if (!MoveFile(pSrcFullFile, strDstFile.c_str ()))
            {
                strError = WinErrMsg (GetLastError ());
                return FALSE;
            }
            return true;
        }
        i++;
    }
    strError = MAKE_STRING("Too many files in destination path for file: " << pFileName << ". Please delete some files");
    return false;
}

int GSBatchMain (char *pCommandLine, string &strShowError)
{
    // Duplicate the string to insert \0 Bytes
    DWORD        dwSize;
    DWORD        dwResultCode = 1; // Error
    string       strError, strPDFFile, strPDFOnlyFile, strUserParam, strlibPath, strGSPrinter;
    char         defaultPrinter [500];
    WORD         wBinCode, wPaperCode;
    bool         bBinFound = false, bPaperFound = false;
    VBin2Code    bincodes, papercodes;
    const char **gsArgv = NULL;
    int          ngsBaseArg, i;
    CGSSupport  *pGS = NULL;
    LString      listLocalGSOptions;            // -G
    LString::iterator itGSParam, itString;
    bool         hasPrinter = false;

    if (!ReadIniFileParameter (strShowError))
    {
        Usage ();
        goto ErrorExit;
    }

    // open log files
    if (strLogFile.size () > 0)
    {
        logStream.clear ();
        logStream.open (strLogFile.c_str (), ios::app);
        if (logStream.fail ())
        {
            strShowError = MAKE_STRING("Could not open the log file '" << strLogFile << "'.");
            goto ErrorExit;
        }
    }

    // Parse command line parameters
    if (!WalkThroughCommandLine (pCommandLine, strShowError))
        goto ErrorExit;

    dwSize = _countof (defaultPrinter);
    if (!GetDefaultPrinter (defaultPrinter, &dwSize))
    {
        strShowError = MAKE_STRING("Could not detect the standard printer. Error msg:: " << WinErrMsg (GetLastError ()));
        goto ErrorExit;
    }

    // Default printer
    if (stricmp (strPrinterName.c_str (), "STD") == 0)
        strPrinterName = defaultPrinter;

    hasPrinter = !strPrinterName.empty ();

    // let the flag in for showing a printer dialog or something else later
    if (!hasPrinter)
    {
        strShowError = "No printer name provided. Use -? to show usage information";
        goto ErrorExit;
    }

    // test move pathes
    if (!CheckParameters (strShowError))
        goto ErrorExit;

    /*
    if (strPrinterName.empty ())
        strPrinterName = defaultPrinter;
    */

    if (hasPrinter)
    {
        // read the printer settings
        if (!GetPrinterBins (strPrinterName.c_str (), bincodes, strShowError))
            goto ErrorExit;

        if (!GetPrinterPapers (strPrinterName.c_str (), papercodes, strShowError))
            goto ErrorExit;

        // output bins to text file ?
        if (bShowBins)
        {
            OutputBinsFile (strPrinterName.c_str (), bincodes, papercodes);
            goto OkExit;
        }
    }

    if (vPDFFilePattern.empty ())
    {
        strShowError = "No pdf file pattern defined. Please use option /F. Available options with /?";
        goto ErrorExit;
    }

    if (hasPrinter && vBinParameters.size () > 0)
    {
        bBinFound = GetInputBinCode (bincodes, vBinParameters, wBinCode);
        if (!bBinFound)
        {
            Write2LogFile (Warning, "", MAKE_CSTRING("input bin(s) '" << VString2String(vBinParameters) << "' not found for printer '" << strPrinterName));
        }
    }

    if (hasPrinter && vPaperParameters.size () > 0)
    {
        bPaperFound = GetInputBinCode (papercodes, vPaperParameters, wPaperCode);
        if (!bPaperFound)
        {
            Write2LogFile (Warning, "", MAKE_CSTRING("paper name(s) '" << VString2String(vPaperParameters) << "' not found for printer '" << strPrinterName));
        }
    }

    pGS = new CGSSupport();

    // Load ghostscript dll 
    if (!pGS->Init (strIniFileName.c_str (), bQuiet, &bBreak, strShowError))
    {
        goto ErrorExit;
    }

    listLocalGSOptions.push_back ("dummy.exe");
    listLocalGSOptions.push_back ("-dBATCH");

    strlibPath = pGS->LibPath ();
    if (!strlibPath.empty ())
    {
        strlibPath = "-I" + strlibPath;
        listLocalGSOptions.push_back (strlibPath);
    }

    if (!strColorPrint.empty ())
    {
        int nColor = atol (strColorPrint.c_str());

        // if bitsperpixel is set, it will lead to color print in any case
        if (nColor == 0)
            strBitsPerPixel.clear();

        // 0 = nothing, 1 = mono, 2 = color
        listUserDefinedOptions.push_back (MAKE_STRING ("/Color " << nColor + 1));
    }

    if (!strBitsPerPixel.empty ())
    {
        listPrinterOptions.push_back (MAKE_STRING ("/BitsPerPixel " << atol(strBitsPerPixel.c_str())));
    }


    // Add commandline/ini parameters
    listLocalGSOptions.insert (listLocalGSOptions.end(), listGSOptions.begin (), listGSOptions.end());

    ngsBaseArg = (int)listLocalGSOptions.size ();

    // three dynamic parameters are added inside the file loop
    gsArgv = new const char *[ngsBaseArg + 4];

    // copy base parameters
    for (i = 0, itString = listLocalGSOptions.begin (); itString != listLocalGSOptions.end(); itString++, i++)
    {
        gsArgv[i] = (char *)itString->c_str ();
    }

    do
    {
        if (bBreak)
            break;

        // Parse the file patterns
        DetectPrintingFiles();

        // Sort the files according to the sort commandline parameter
        SortFiles ();

        string strGSPrinter;

        if (hasPrinter) 
        {
            // Ghostscript needs real printernames for network printers not the share name and backslashes must be doubled.
            strGSPrinter = Convert2GSPrinter (strPrinterName.c_str ());
        }

        // Walk through all files
        for (VFileData::iterator it = vFileData.begin (); !bBreak && it != vFileData.end (); it++)
        {
            bool bCallExit = false;

            if (!bQuiet && pOutStream)
            {
                *pOutStream << it->strFileName << endl;
                pOutStream->flush ();
            }

            // get the filename for Document title
            size_t nSlash  = it->strFileName.rfind ('\\');
            strPDFOnlyFile = (nSlash == string::npos) ? it->strFileName : it->strFileName.substr (nSlash + 1);

            // Build user defined settings string
            strUserParam = MAKE_STRING("mark");
            for (itString = listPrinterOptions.begin (); itString != listPrinterOptions.end(); itString++)
            {
                strUserParam += " ";
                strUserParam += (char *)itString->c_str ();
            }

            if (hasPrinter)
            {
                strUserParam += MAKE_STRING(" /OutputFile (%printer%" << strGSPrinter << ")");
            }
            strUserParam += " /UserSettings << /DocumentName (";
            strUserParam += strPDFOnlyFile;
            strUserParam += ")";

            // mswinpr2 device ignores resolutions <= 50
            if (maxResolution >= 0 && maxResolution < 51)
            {
                maxResolution = 51; 
                if (!bQuiet && pOutStream)
                {
                    *pOutStream << "set resolution to 51 (useful minimum)" << endl;
                    pOutStream->flush ();
                }
            }

            if (maxResolution >= 0)
            {
                strUserParam += MAKE_STRING(" /MaxResolution " << maxResolution);
            }

            for (itString = listUserDefinedOptions.begin (); itString != listUserDefinedOptions.end(); itString++)
            {
                strUserParam += " ";
                strUserParam += (char *)itString->c_str ();
            }

            if (bBinFound)
                strUserParam += MAKE_STRING(" /InputBin " << wBinCode);

            if (bPaperFound)
                strUserParam += MAKE_STRING(" /Paper " << wPaperCode);

            if (nOrientation > 0)
                strUserParam += MAKE_STRING(" /Orientation " << nOrientation);

            strUserParam += " >> ";

            strUserParam += " (mswinpr2) finddevice putdeviceprops setdevice";

            if (bReplaceSlashes)
            {
               for (string::iterator itString = it->strFileName.begin (); itString != it->strFileName.end(); itString++)
               {
                   if (*itString == '\\')
                       *itString = '/';
               }
            }
            gsArgv[ngsBaseArg + 0] = "-c";
            gsArgv[ngsBaseArg + 1] = strUserParam.c_str ();
            gsArgv[ngsBaseArg + 2] = "-f";
            gsArgv[ngsBaseArg + 3] = it->strFileName.c_str ();
            int code = 0;
            if (!bTest)
            {
                if (!pGS->Start (ngsBaseArg + 4, (char **)gsArgv, strPDFOnlyFile.c_str (), strError))
                {
                    Write2LogFile (Error, strPDFOnlyFile.c_str (), strError.c_str());
                    if (!bQuiet && pOutStream)
                        *pOutStream << "Error:" << strError << endl;

                    if (strMoveErrPath.size () > 0)
                    {
                        string strLocalError;
                        string dstFile = strMoveErrPath + strPDFOnlyFile;
                        if (!MoveFileWithSuffix (it->strFileName.c_str (), strPDFOnlyFile.c_str (), strMoveErrPath.c_str (), strLocalError))
                        {
                            if (bLoop)
                            {
                                strError = MAKE_STRING("Could not move the file. Stop looping. Error msg:" << strLocalError);
                                Write2LogFile (Error, strPDFOnlyFile.c_str (), strLocalError.c_str ());
                                bLoop = false;
                            }
                            else
                            {
                                strError = MAKE_STRING("Could not move the file. Error msg:" << strLocalError);
                                Write2LogFile (Error, strPDFOnlyFile.c_str (), strLocalError.c_str ());
                            }
                            if (!bQuiet && pOutStream)
                                *pOutStream << "Error Moving file:" << strLocalError << endl;
                        }
                    }
                }
                else
                {
                    if (!pGS->Begin (strError) || !pGS->End (strError) || !pGS->Exit (strError))
                    {
                        Write2LogFile (Error, strPDFOnlyFile.c_str (), strError.c_str ());
                        if (!bQuiet && pOutStream)
                             *pOutStream << "Error:" << strError << endl;
                    }
                    else
                    {
                        Write2LogFile (Ok, strPDFOnlyFile.c_str (), "");
                        if (!bQuiet && pOutStream)
                            *pOutStream << "Printed:Ok" << endl;

						if (bDeleteSource)
						{
							if (!::DeleteFile(it->strFileName.c_str ()))
							{
								string strLocalError;
								if (bLoop)
								{
									strError = MAKE_STRING("Could not delete the file. Stop looping. Error msg:" << strLocalError);
									Write2LogFile (Error, it->strFileName.c_str (), strLocalError.c_str ());
									bLoop = false;
								}
								else
								{
									strError = MAKE_STRING("Could not delete the file. Error msg:" << strLocalError);
									Write2LogFile (Error, it->strFileName.c_str (), strLocalError.c_str ());
								}
								if (!bQuiet && pOutStream)
									*pOutStream << "Error Moving file:" << strLocalError << endl;
							}
						}
						else if (strMoveOkPath.size () > 0)
                        {
							string dstFile = strMoveOkPath + strPDFOnlyFile;
							string strLocalError;
							if (!MoveFileWithSuffix (it->strFileName.c_str (), strPDFOnlyFile.c_str (), strMoveOkPath.c_str (), strLocalError))
							{
								if (bLoop)
								{
									strError = MAKE_STRING("Could not move the file. Stop looping. Error msg:" << strLocalError);
									Write2LogFile (Error, strPDFOnlyFile.c_str (), strLocalError.c_str ());
									bLoop = false;
								}
								else
								{
									strError = MAKE_STRING("Could not move the file. Error msg:" << strLocalError);
									Write2LogFile (Error, strPDFOnlyFile.c_str (), strLocalError.c_str ());
								}
								if (!bQuiet && pOutStream)
									*pOutStream << "Error Moving file:" << strLocalError << endl;
							}
                        }
                    }
                }
            }
        }
        if (bBreak)
            break;

        // Wait ?
        if (bLoop && loopTime > 0)
        {
            if (!bQuiet && pOutStream)
                *pOutStream << "Wait " << loopTime << " seconds" << endl;

            int seconds = loopTime;
            while (seconds > 0 && !bBreak)
            {
                Sleep (1000);
                seconds--;
            }
        }
    }
    while (bLoop);

OkExit:
    dwResultCode = 0;
ErrorExit:
    delete [] gsArgv;
    gsArgv = NULL;
    delete pGS;
    return dwResultCode;
}

bool WalkThroughCommandLine (char * pCmdLine, string &strError)
{
    int    state = 0;
    char * pParamStart = pCmdLine;
    int    argc = 0;

    for (;;)
    {
        char ch = *pCmdLine;

        switch (state)
        {
        case 0:
            // first character of a new string
            if (ch == '\"')
            {
                state = 2;
                pCmdLine++; // points to first real character
            }
            else if (ch == 0)
                return true;
            else
                state = 1;

            pParamStart = pCmdLine;
            break;
        case 1:
            // search until blank
            if (ch == 0)
            {
                state = 10;
            }
            else if (ch <= ' ')
            {
                *pCmdLine = 0;
                pCmdLine++;
                state = 10;
            }
            else
                pCmdLine++;
            break;
        case 2:
            // search until end or quote
            if (ch == '\"')
            {
                state = 10;
                *pCmdLine = 0;
                pCmdLine++;
            }
            else if (ch == 0)
            {
                state = 10;
            }
            else
                pCmdLine++;
            break;
        case 10:
            // pCmdLine points to the first limit parameter
            if (!CommandLineParam (argc, pParamStart, strError))
                return false;

            if (ch == 0)
                return true;
            else
                state = 11;
            break;
        case 11:
            // overread blanks until next real characeter
            if (ch == 0)
                return true;
            else if (ch > ' ')
            {
                argc++;
                state = 0;
            }
            else
                pCmdLine++;
            break;
        }
    }

    
    return false;
}

