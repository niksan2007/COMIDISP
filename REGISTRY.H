#ifndef __Registry_H__
#define __Registry_H__
//
// Registry.h
//   - Вспомогательные функции для регистрации и удаления компонента.
//

// Эта функция зарегистрирует компонент в реестре.
// Компонент вызывает эту функцию из своей функции DllRegisterServer.
HRESULT RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       const char* szFriendlyName,
                       const char* szVerIndProgID,
                       const char* szProgID,
                       const GUID& libid) ;

// Эта функция отменяет регистрацию компонента. Компоненты
// вызывают эту функцию из своей функции DllUnregisterServer.
HRESULT UnregisterServer(const CLSID& clsid,
                         const char* szVerIndProgID,
                         const char* szProgID) ;

#endif
