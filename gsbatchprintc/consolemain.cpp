#include <windows.h>
#include <fstream>
#include <iostream>

#include "gsbatchprint.h"
#include "version.h"

// DebugOutput
ostream *pOutStream = NULL;
const char *szTitle = "gsBatchPrint";

void Usage ()
{
    if (pOutStream)
    {
        *pOutStream << "GSBatchPrint - Version: " << VER_FILEVERSION_STR << endl << endl;
        *pOutStream << pUsageString;
    }
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT)
    {
        if (!bQuiet)
        {
            *pOutStream << "Break detected. Leave at next possibility" << endl;
            pOutStream->flush ();
        }

        bBreak = true;
        return true;
    }
    return false;
}


int main(int argc, char* argv1[])
{
    string strShowError;
    char * pDupCommandLine = DupString (GetCommandLine ());

    pOutStream = &cout;

    SetConsoleCtrlHandler(HandlerRoutine, true);

    int nResult = GSBatchMain (pDupCommandLine, strShowError);

    // Cleanup
    delete [] pDupCommandLine;

    if (nResult != 0 && !strShowError.empty ())
    {
        MessageBox (NULL, strShowError.c_str (), szTitle, MB_ICONSTOP);
    }

    return nResult;
}
