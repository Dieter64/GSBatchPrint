#include <windows.h>
#include <fstream>
#include <iostream>

#include "gsbatchprint.h"
#include "version.h"
#include "..\common\resource.h"

// DebugOutput
ostream *pOutStream = NULL;
const char *szTitle = "gsBatchPrint";

HINSTANCE hInst = NULL;

// Meldungshandler für Infofeld.
INT_PTR CALLBACK Help(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hMsg;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
        {
            string msg = MAKE_STRING("GSBatchPrint - Version: "<< VER_FILEVERSION_STR);
            hMsg = GetDlgItem (hDlg, IDC_MSG);
            SetWindowText (hMsg, pUsageString);
            SetWindowText (hDlg, msg.c_str ());
		    return (INT_PTR)TRUE;
        }

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void Usage()
{
	DialogBox(hInst, MAKEINTRESOURCE(IDD_HELP), NULL, Help);
}



int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    string strShowError;

    pOutStream = &cout;

    int nResult = GSBatchMain (lpCmdLine, strShowError);

    if (nResult != 0 && !strShowError.empty ())
    {
        MessageBox (NULL, strShowError.c_str (), szTitle, MB_ICONSTOP);
    }

    return nResult;
}

