////////////////////////////////////////////
// libregistry.cpp
////////////////////////////////////////////
// Beschr.: Hilfsfunktionen aller Kategorien
// Autor  : Dieter Riekert
// Besond.: C++, Unicode/Ansi, keine MFC
// 
// History:
// $Log$
////////////////////////////////////////////
#include <windows.h>
#include <tchar.h>

///////////////////////
// Registryfunctions //
///////////////////////

extern "C" {

// Erzeugt den Key pKey rekursiv, falls er nicht existiert
BOOL _cdecl RegistryCreateKeyIfNotExist (HKEY hBaseKey, LPCTSTR pKey, DWORD *pdwError)
{
   HKEY   hKey = NULL;
   DWORD  dwDummy;
   TCHAR  buf [1000];
   LPTSTR pFindStart = buf;
   LPTSTR pFound;

   // damit ist *pdwError immer gültig
   if (!pdwError)
      pdwError = &dwDummy;

   // nicht das Original bearbeiten
   _tcscpy_s (buf, _countof (buf), pKey);

   do
   {
      pFound = _tcschr (pFindStart, TEXT('\\'));
      if (pFound)
         *pFound = 0;

      *pdwError = RegOpenKeyEx (hBaseKey, buf, 0, KEY_READ, &hKey);

      if (*pdwError != ERROR_SUCCESS)
      {
         // Key existiert nicht, dann versuche ihn anzulegen
         *pdwError = RegCreateKeyEx (hBaseKey, buf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ,
                                     NULL, &hKey, NULL);

         if (*pdwError != ERROR_SUCCESS)
         {
            RegCloseKey (hKey);
            return FALSE;
         }
      }
      // Key existiert, dann schließen
      RegCloseKey (hKey);

      // nächsten Teilstring bearbeiten
      if (pFound)
      {
         *pFound = '\\';
         pFindStart = pFound + 1;
      }
   }
   while (pFound);

   return TRUE;
}


// Allgemeine Einen DWORD-Wert in die Registry schreiben
BOOL _cdecl RegistryWriteGeneral (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD dwRegType, LPBYTE pData, DWORD dwCount, DWORD *pdwError)
{
   HKEY hKey = NULL;
   DWORD dwDummy;

   // damit ist *pdwError immer gültig
   if (!pdwError)
      pdwError = &dwDummy;

   if (!pKey || !pData) // || !pValueKey)
   {
      *pdwError = ERROR_INVALID_PARAMETER;
      return FALSE;
   }

   // Falls der Key nicht existiert, dann neu erzeugen (rekursiv)
   if (RegOpenKey (hBaseKey, pKey, &hKey) != ERROR_SUCCESS)
   {
      if (!RegistryCreateKeyIfNotExist (hBaseKey, pKey, pdwError))
      {
         return FALSE;
      }
   }
   else
   {
      RegCloseKey (hKey);
      hKey = NULL;
   }

   // Der Key existiert, dann nochmals schreibend öffnen
   *pdwError = RegOpenKeyEx (hBaseKey, pKey, 0, KEY_WRITE | KEY_READ, &hKey);

   // Wahrscheinlich zu wenig Rechte
   if (*pdwError != ERROR_SUCCESS)
      return FALSE;

   *pdwError = RegSetValueEx (hKey, pValueKey, 0, dwRegType, (LPBYTE) pData, dwCount);

   RegCloseKey (hKey);

   return (*pdwError == ERROR_SUCCESS);
}

// Einen DWORD-Wert in die Registry schreiben
BOOL _cdecl RegistryWriteDWORD (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD dwValue, DWORD *pdwError)
{
   return (RegistryWriteGeneral (hBaseKey, pKey, pValueKey, REG_DWORD, (LPBYTE) &dwValue, sizeof (DWORD), pdwError));
}

BOOL _cdecl RegistryReadDWORD  (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD &dwValue)
{
   HKEY  hKey;
   DWORD dwType, dwSize;
   BOOL  bResult = FALSE;;

   // Falls der Key nicht existiert, dann neu erzeugen (rekursiv)
   if (RegOpenKey (hBaseKey, pKey, &hKey) == ERROR_SUCCESS)
   {
      // Wert lesen
      dwSize = sizeof (DWORD);
      if (RegQueryValueEx (hKey, pValueKey, NULL, &dwType, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS || dwType != REG_DWORD)
         bResult = FALSE;
      else
         bResult = TRUE;

      RegCloseKey (hKey);
      hKey = NULL;
   }
   if (!bResult)
      dwValue = 0;
   return bResult;
}

// Einen DWORD-Wert in die Registry schreiben
BOOL _cdecl RegistryWriteSZ (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, LPCTSTR pValue, DWORD *pdwError)
{
   if (!pValue)
   {
      *pdwError = ERROR_INVALID_PARAMETER;
      return FALSE;
   }

   return (RegistryWriteGeneral (hBaseKey, pKey, pValueKey, REG_SZ, (LPBYTE) pValue, (_tcslen (pValue) + 1) * sizeof (TCHAR), pdwError));
}

LPTSTR _cdecl RegistryReadSZ (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey)
{
   HKEY  hKey;
   DWORD dwType, dwSize;
   BOOL  bResult = FALSE;;
   LPTSTR pReturn = NULL;

   // Falls der Key nicht existiert, dann neu erzeugen (rekursiv)
   if (RegOpenKey (hBaseKey, pKey, &hKey) == ERROR_SUCCESS)
   {
      // Wert lesen
      dwSize = 0;
      LONG l = RegQueryValueEx (hKey, pValueKey, NULL, &dwType, NULL, &dwSize);

      if (l != ERROR_SUCCESS)
         bResult = FALSE;
      else
      {
         if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
            bResult = FALSE;
         else
         {
            // Speicher besorgen
            pReturn = new TCHAR [dwSize + 1];

            // MORE_DATA
            l = RegQueryValueEx (hKey, pValueKey, NULL, &dwType, (LPBYTE) pReturn, &dwSize);
            bResult = TRUE;
         }
      }

      RegCloseKey (hKey);
      hKey = NULL;
   }
   return bResult ? pReturn : NULL;
}



// Einen Wert aus der Registry löschen
BOOL _cdecl RegistryDeleteValue (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD *pdwError)
{
   HKEY hKey;
   DWORD dwDummy;

   // damit ist *pdwError immer gültig
   if (!pdwError)
      pdwError = &dwDummy;

   // Key öffnen
   *pdwError = RegOpenKeyEx (hBaseKey, pKey, 0, KEY_WRITE | KEY_READ, &hKey);
   if (*pdwError != ERROR_SUCCESS)
      return FALSE;

   // Wert löschen
   *pdwError = RegDeleteValue (hKey, pValueKey);
   RegCloseKey (hKey);

   return (*pdwError == ERROR_SUCCESS);
}

// Einen Key löschen. Unter Windows 95/... darf der Key aus Subkeys enthalten unter 
// NT,2000,XP nicht. Wird später allgemein ersetzt
BOOL _cdecl RegistryDeleteKey (HKEY hBaseKey, LPCTSTR pKey, DWORD *pdwError)
{
   DWORD dwDummy;

   // damit ist *pdwError immer gültig
   if (!pdwError)
      pdwError = &dwDummy;

   if (!pKey)
   {
      *pdwError = ERROR_INVALID_PARAMETER;
      return FALSE;
   }

   *pdwError = RegDeleteKey (hBaseKey, pKey);

   return (*pdwError == ERROR_SUCCESS);
}

}