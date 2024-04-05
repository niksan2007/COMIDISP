#ifndef __CFactory_h__
#define __CFactory_h__

#include "CUnknown.h"
///////////////////////////////////////////////////////////

// Forward reference
class CFactoryData;

// Глобальные данные, используемые CFactory
extern CFactoryData g_FactoryDataArray[];
extern int g_cFactoryDataEntries;

//////////////////////////////////////////////////////////
//
//  Функция создания компонента
//
class CUnknown;

typedef HRESULT (*FPCREATEINSTANCE)(IUnknown*, CUnknown**);

///////////////////////////////////////////////////////////
//
// CFactoryData
//   - Информация, необходимая CFactory для создания компонента,
//     поддерживаемого DLL
//
class CFactoryData
{
public:
    // ID класса для компонента
    const CLSID* m_pCLSID;

    // Указатель на функцию, создающую его
    FPCREATEINSTANCE CreateInstance;

    // Имя компонента для регистрации в реестре
    const char* m_RegistryName;

    // ProgID
    const char* m_szProgID;

    // Независимый от версии ProgID
    const char* m_szVerIndProgID;

    // Вспомогательная функция для поиска ID класса
    BOOL IsClassID(const CLSID& clsid) const
        { return (*m_pCLSID == clsid); }

    // ID библиотеки типов
    const GUID* m_pLIBID;

    //
    // Поддержка сервера вне процесса
    //

    // Указатель на работающую фабрику классов для этого компонента
    IClassFactory* m_pIClassFactory;

    // Магическое значение для идентификации работающего объекта
    DWORD m_dwRegister;
};


///////////////////////////////////////////////////////////
//
// Фабрика классов
//
class CFactory : public IClassFactory
{
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
    virtual ULONG   __stdcall AddRef();
    virtual ULONG   __stdcall Release();
    
    // IClassFactory
    virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
                                             const IID& iid,
                                             void** ppv);
    virtual HRESULT __stdcall LockServer(BOOL bLock); 

    // Конструктор - передаем указатель на данные компонента для создания.
    CFactory(const CFactoryData* pFactoryData);

    // Деструктор
    ~CFactory() { }

    //
    // Поддержка статических данных FactoryData
    //

    // Поддержка DllGetClassObject
    static HRESULT GetClassObject(const CLSID& clsid, 
                                  const IID& iid, 
                                  void** ppv);

    // Вспомогательная функция для DllCanUnloadNow
    static BOOL IsLocked()
        { return (s_cServerLocks > 0); }

    // Функции для [раз]регистрации всех компонентов
    static HRESULT RegisterAll();
    static HRESULT UnregisterAll();

    // Функция для определения возможности выгрузки компонента
    static HRESULT CanUnloadNow();


#ifdef _OUTPROC_SERVER_
    //
    // Поддержка сервера вне процесса
    //

    static BOOL StartFactories();
    static void StopFactories();

    static DWORD s_dwThreadID;

    // Завершение приложения.
    static void CloseExe()
    {
        if (CanUnloadNow() == S_OK)
        {
            ::PostThreadMessage(s_dwThreadID, WM_QUIT, 0, 0);
        }
    }
#else
    // CloseExe ничего не делает, если мы в процессе.
    static void CloseExe() { /* Пусто */ } 
#endif

public:
    // Счетчик ссылок
    LONG m_cRef;

    // Указатель на информацию о классе, создаваемом этой фабрикой
    const CFactoryData* m_pFactoryData;

    // Счетчик блокировок
    static LONG s_cServerLocks;   

    // Дескриптор модуля
    static HMODULE s_hModule;
};

#endif
