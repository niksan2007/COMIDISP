//
// Registry.cpp
//

#include <objbase.h>
#include <assert.h>

#include "Registry.h"

////////////////////////////////////////////////////////
//
// Прототипы внутренних вспомогательных функций
//

// Установить указанный ключ и его значение.
BOOL setKeyAndValue(const char* pszPath,
                    const char* szSubkey,
                    const char* szValue) ;

// Преобразовать GUID в строку char.
void GUIDtochar(const GUID& guid, 
                char* szGUID,
                int length) ;

// Определить, существует ли определенный подключ в реестре.
BOOL SubkeyExists(const char* pszPath,
                  const char* szSubkey) ;

// Удалить szKeyChild и всех его потомков.
LONG recursiveDeleteKey(HKEY hKeyParent, const char* szKeyChild) ;

////////////////////////////////////////////////////////
//
// Константы
//

// Размер GUID в виде строки
const int GUID_STRING_SIZE = 39 ;

/////////////////////////////////////////////////////////
//
// Реализация открытых функций
//

//
// Зарегистрировать компонент в реестре.
//
HRESULT RegisterServer(HMODULE hModule,            // Дескриптор модуля DLL
                       const CLSID& clsid,         // Идентификатор класса
                       const char* szFriendlyName, // Дружественное имя
                       const char* szVerIndProgID, // Программные
                       const char* szProgID,       //   идентификаторы
                       const GUID& libid)          // Идентификатор библиотеки
{
	// Получить расположение сервера.
	char szModule[512] ;
	DWORD dwResult =
		::GetModuleFileName(hModule, 
		                    szModule,
		                    sizeof(szModule)/sizeof(char)) ;
	assert(dwResult != 0) ;

	// Преобразовать CLSID в строку.
	char szCLSID[GUID_STRING_SIZE] ;
	GUIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;

	// Построить ключ CLSID\\{...}
	char szKey[64] ;
	strcpy(szKey, "CLSID\\") ;
	strcat(szKey, szCLSID) ;
  
	// Добавить CLSID в реестр.
	setKeyAndValue(szKey, NULL, szFriendlyName) ;

	// Добавить подключение к файлу сервера под ключом CLSID.
#ifdef _OUTPROC_SERVER_
	setKeyAndValue(szKey, "LocalServer32", szModule) ;
#else
	setKeyAndValue(szKey, "InprocServer32", szModule) ;
#endif

	// Добавить подключение ProgID под ключом CLSID.
	setKeyAndValue(szKey, "ProgID", szProgID) ;

	// Добавить подключение к версии-независимому ProgID под ключом CLSID.
	setKeyAndValue(szKey, "VersionIndependentProgID",
	               szVerIndProgID) ;

	// Добавить подключение к библиотеке типов под ключом CLSID.
	char szLIBID[GUID_STRING_SIZE] ;
	GUIDtochar(libid, szLIBID, sizeof(szLIBID)) ;
	setKeyAndValue(szKey, "TypeLib", szLIBID) ;

	// Добавить версию-независимый ProgID под ключом HKEY_CLASSES_ROOT.
	setKeyAndValue(szVerIndProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szVerIndProgID, "CLSID", szCLSID) ;
	setKeyAndValue(szVerIndProgID, "CurVer", szProgID) ;

	// Добавить версионный ProgID под ключом HKEY_CLASSES_ROOT.
	setKeyAndValue(szProgID, NULL, szFriendlyName) ; 
	setKeyAndValue(szProgID, "CLSID", szCLSID) ;

	return S_OK ;
}

//
// Удалить компонент из реестра.
//
LONG UnregisterServer(const CLSID& clsid,         // Идентификатор класса
                      const char* szVerIndProgID, // Программные
                      const char* szProgID)       //   идентификаторы
{
	// Преобразовать CLSID в строку.
	char szCLSID[GUID_STRING_SIZE] ;
	GUIDtochar(clsid, szCLSID, sizeof(szCLSID)) ;

	// Построить ключ CLSID\\{...}
	char szKey[80] ;
	strcpy(szKey, "CLSID\\") ;
	strcat(szKey, szCLSID) ;

	// Проверить наличие другого сервера для этого компонента.
#ifdef _OUTPROC_SERVER_
	if (SubkeyExists(szKey, "InprocServer32"))
#else
	if (SubkeyExists(szKey, "LocalServer32"))
#endif
	{
		// Удалить только путь для этого сервера.
#ifdef _OUTPROC_SERVER_
		strcat(szKey, "\\LocalServer32") ;
#else
		strcat(szKey, "\\InprocServer32") ;
#endif
		LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
		assert(lResult == ERROR_SUCCESS) ;
	}
	else
	{
		// Удалить все связанные ключи.
		// Удалить ключ CLSID - CLSID\{...}
		LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey) ;
		assert((lResult == ERROR_SUCCESS) ||
		       (lResult == ERROR_FILE_NOT_FOUND)) ; // Подключение может не существовать.

		// Удалить ключ версии-независимого ProgID.
		lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID) ;
		assert((lResult == ERROR_SUCCESS) ||
		       (lResult == ERROR_FILE_NOT_FOUND)) ; // Подключение может не существовать.

		// Удалить ключ ProgID.
		lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID) ;
		assert((lResult == ERROR_SUCCESS) ||
		       (lResult == ERROR_FILE_NOT_FOUND)) ; // Подключение может не существовать.
	}
	return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Внутренние вспомогательные функции
//

// Преобразовать GUID в строку char.
void GUIDtochar(const GUID& guid,
                char* szGUID,
                int length)
{
	assert(length >= GUID_STRING_SIZE) ;
	// Получить строку в виде wide character.
	LPOLESTR wszGUID = NULL ;
	HRESULT hr = StringFromCLSID(guid, &wszGUID) ;
	assert(SUCCEEDED(hr)) ;

	// Преобразовать из wide characters в не-wide.
	wcstombs(szGUID, wszGUID, length) ;

	// Освободить память.
	CoTaskMemFree(wszGUID) ;
}

//
// Удалить ключ и всех его потомков.
//
LONG recursiveDeleteKey(HKEY hKeyParent,           // Родительский ключ для удаления
                        const char* lpszKeyChild)  // Ключ для удаления
{
	// Открыть дочерний ключ.
	HKEY hKeyChild ;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
	                         KEY_ALL_ACCESS, &hKeyChild) ;
	if (lRes != ERROR_SUCCESS)
	{
		return lRes ;
	}

	// Перечислить всех потомков этого дочернего ключа.
	FILETIME time ;
	char szBuffer[256] ;
	DWORD dwSize = 256 ;
	while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL,
	                    NULL, NULL, &time) == S_OK)
	{
		// Удалить потомков этого дочернего ключа.
		lRes = recursiveDeleteKey(hKeyChild, szBuffer) ;
		if (lRes != ERROR_SUCCESS)
		{
			// Очистка перед выходом.
			RegCloseKey(hKeyChild) ;
			return lRes;
		}
		dwSize = 256 ;
	}

	// Закрыть дочерний ключ.
	RegCloseKey(hKeyChild) ;

	// Удалить этот дочерний ключ.
	return RegDeleteKey(hKeyParent, lpszKeyChild) ;
}

//
// Определить, существует ли определенный подключ.
//
BOOL SubkeyExists(const char* pszPath,    // Путь к ключу для проверки
                  const char* szSubkey)   // Подключение для проверки
{
	HKEY hKey ;
	char szKeyBuf[80] ;

	// Скопировать имя ключа в буфер.
	strcpy(szKeyBuf, pszPath) ;

	// Добавить имя подключения в буфер.
	if (szSubkey != NULL)
	{
		strcat(szKeyBuf, "\\") ;
		strcat(szKeyBuf, szSubkey ) ;
	}

	// Определить, существует ли ключ путем попытки открыть его.
	LONG lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, 
	                              szKeyBuf,
	                              0,
	                              KEY_ALL_ACCESS,
	                              &hKey) ;
	if (lResult == ERROR_SUCCESS)
	{
		RegCloseKey(hKey) ;
		return TRUE ;
	}
	return FALSE ;
}

//
// Создать ключ и установить его значение.
//   - Эта вспомогательная функция была заимствована и изменена из
//     книги Крейга Брокшмита "Inside OLE".
//
BOOL setKeyAndValue(const char* szKey,
                    const char* szSubkey,
                    const char* szValue)
{
	HKEY hKey;
	char szKeyBuf[1024] ;

	// Скопировать имя ключа в буфер.
	strcpy(szKeyBuf, szKey) ;

	// Добавить имя подключения в буфер.
	if (szSubkey != NULL)
	{
		strcat(szKeyBuf, "\\") ;
		strcat(szKeyBuf, szSubkey ) ;
	}

	// Создать и открыть ключ и подключение.
	long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT ,
	                              szKeyBuf, 
	                              0, NULL, REG_OPTION_NON_VOLATILE,
	                              KEY_ALL_ACCESS, NULL, 
	                              &hKey, NULL) ;
	if (lResult != ERROR_SUCCESS)
	{
		return FALSE ;
	}

	// Установить значение.
	if (szValue != NULL)
	{
		RegSetValueEx(hKey, NULL, 0, REG_SZ, 
		              (BYTE *)szValue, 
		              strlen(szValue)+1) ;
	}

	RegCloseKey(hKey) ;
	return TRUE ;
}
