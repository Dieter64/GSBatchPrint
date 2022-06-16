#include <windows.h>

#include "gsbatchprint.h"

static char tcharBuf [1000];

const char * GetFileExtFromFilename (const char * pFileName)
{
   if (!pFileName)
      return NULL;

   const char * pDot       = strrchr (pFileName, '.');
   const char * pBackSlash = strrchr (pFileName, '\\');

   if (!pDot)
      return "";
   if (pBackSlash && pBackSlash > pDot)
      return "";

   return pDot + 1;
}

const char * GetFileNoExtFromFilename (const char * pFileName)
{
   if (!pFileName)
      return NULL;

   strncpy (tcharBuf, pFileName, _countof (tcharBuf));

   char *pDot       = strrchr (tcharBuf, '.');
   char *pBackSlash = strrchr(tcharBuf, '\\');

   // kein . oder Punkt vor dem letzten Backslash
   if (!pDot || (pBackSlash && pBackSlash > pDot))
      return pFileName;

   *pDot = 0;
   return tcharBuf;
}

bool FileExists (const char *pFile)
{
   if (!pFile)
      return FALSE;

   HANDLE hFile = CreateFile (pFile, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   bool   bExist = (hFile != INVALID_HANDLE_VALUE);

   if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle (hFile);
   
   return bExist;
}

// Splits a string in it's components seperated by one delimiter
bool SplitStringSTL (const char *pString, VString &strArray, char chDelim, string &strError)
{
   int nString = 0;
   char szDelFind [2];
   char szDelFindHyphen [3];

   if (!pString || !chDelim)
   {
      strError = "SplitString: Interner Fehler: Ungültiger Parameter (pString oder pResult = NULL";
      return false;
   }
   // Suchstrings vorbereiten
   szDelFind [0] = chDelim;
   szDelFind [1] = 0;
   szDelFindHyphen [0] = '\"';
   szDelFindHyphen [1] = chDelim;
   szDelFindHyphen [2] = 0;
   

   // erster Parameter
   for (nString = 0;;nString++)
   {
      char * pDelFind;
      char * pDelim;
      size_t nStringLen = strlen (pString);

      // Falls das erste Zeichen des String ein Anführungszeichen, dann suche nach "<Trenner>
      if (*pString == '\"')
      {
         pString++;
         pDelFind = szDelFindHyphen;
         pDelim = strstr ((char *)pString, pDelFind);
         if (!pDelim)
         {
            // kein Anführungsende gefunden, dann nimm ganzen String
             nStringLen = strlen (pString);
             pDelim = NULL;
         }
         else
            nStringLen = (size_t)(pDelim - pString); // inclusive Anführungszeichen
      }
      else
      {
         pDelFind = szDelFind;
		 // Attention, not correct, has to change somewhen
         pDelim = strstr ((char *) pString, pDelFind);
         if (pDelim)
            nStringLen = (size_t)(pDelim - pString);
         else
            nStringLen = strlen (pString);
      }
      // ans Ende anfügen
      string str (pString, nStringLen);
      back_inserter (strArray) = str;
      // gefunden
      if (!pDelim)
         return true;

      pString = pDelim + strlen(pDelFind);
   }
   return false;
}

// deletes all whitespace characters blow 32 from a string
string StringWithoutBlanks (const char *pValue)
{
    string result;

    for (; *pValue != 0; pValue++)
    {
        unsigned char ch = *(unsigned char *) pValue;
        if (ch > 32)
            result += tolower (*pValue);
    }
    return result;
}

string VString2String(VString value)
{
    VString::iterator it;
    string result;
    int    n;

    for (n = 0, it = value.begin(); it != value.end(); it++, n++)
    {
        result += it->c_str ();
        if (n != value.size () - 1)
            result += ";";
    }
    return result;
}


string WinErrMsg (DWORD dwError)
{
   DWORD    dwLen;
   char *   pLast;
   char *   pFirstAfterCode;
   char     charWinMsg [5000];

   int      nCodeLen = sprintf (charWinMsg, "(%d) ", dwError);

   // zeigt direkt nach den Code
   pFirstAfterCode = charWinMsg + nCodeLen;

   dwLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
                         NULL, dwError, 0, pFirstAfterCode, 
                         sizeof (charWinMsg) / sizeof (charWinMsg[0]) - nCodeLen, NULL);
   if (0 == dwLen)
   {
      strcpy (pFirstAfterCode, "unknown error");

      return charWinMsg; 
   }
   dwLen;

   // Leerstellen am Ende weg
   for (pLast = pFirstAfterCode + dwLen - 1; dwLen > 0 && isspace (*pLast); dwLen--, pLast--)
      ;

   *(pLast + 1) = 0;

   return charWinMsg;
}


// Duplicates a string with the new function
char * DupString (const char * pString)
{
    if (!pString)
        return NULL;

    size_t nStringLen = strlen (pString) + 1;

    char * pResult = new char [nStringLen];
    strcpy_s (pResult, nStringLen, pString);

    return pResult;
}
