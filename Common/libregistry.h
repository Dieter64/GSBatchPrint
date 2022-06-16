////////////////////////////////////////////
// libregistry.h
////////////////////////////////////////////
// Beschr.: Hilfsfunktionen aller Kategorien
// Autor  : Dieter Riekert
// Besond.: C++, Unicode/Ansi, keine MFC
// 
// History:
// $Log$
////////////////////////////////////////////
#ifndef LIBREGISTRY_H
#define LIBREGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL    _cdecl RegistryCreateKeyIfNotExist (HKEY hBaseKey, LPCTSTR pKey, DWORD *pdwError);
extern BOOL    _cdecl RegistryWriteDWORD     (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD dwValue, DWORD *pdwError);
extern BOOL    _cdecl RegistryReadDWORD      (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD &dwValue);

extern BOOL    _cdecl RegistryWriteSZ        (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, LPCTSTR pValue, DWORD *pdwError);
extern LPTSTR  _cdecl RegistryReadSZ         (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey);

extern BOOL    _cdecl RegistryWriteGeneral   (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD dwRegType, LPBYTE pData, DWORD dwCount, DWORD *pdwError);
extern BOOL    _cdecl RegistryDeleteValue    (HKEY hBaseKey, LPCTSTR pKey, LPCTSTR pValueKey, DWORD *pdwError);
extern BOOL    _cdecl RegistryDeleteKey      (HKEY hBaseKey, LPCTSTR pKey, DWORD *pdwError);

#ifdef __cplusplus
}
#endif

#endif
