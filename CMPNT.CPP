//
// Cmpnt.cpp - Компонент
//
#include <objbase.h>
#include <stdio.h>    // sprintf
#include <stdlib.h>   // splitpath

#include "Iface.h"
#include "Util.h"
#include "CUnknown.h"
#include "CFactory.h" // Необходимо для обработки модуля
#include "Cmpnt.h"

static inline void trace(char* msg)
    { Util::Trace("Component", msg, S_OK) ;}
static inline void trace(char* msg, HRESULT hr)
    { Util::Trace("Component", msg, hr) ;}

//
// Имя библиотеки типов
//
const char szTypeLibName[] = "Server.tlb" ;

///////////////////////////////////////////////////////////
//
// Реализация интерфейса IX
//
HRESULT __stdcall CA::Fx()
{ 
    trace("!!!!! Мы добрались до этого момента !!!!!") ;
    return S_OK ;
}

HRESULT __stdcall CA::FxStringIn(BSTR bstrIn)
{ 
    // Отобразить входную строку.
    ostrstream sout ;
    sout << "FxStringIn получила строку:  "
         << bstrIn 
         << ends ;
    trace(sout.str()) ;
    return S_OK ;
}

HRESULT __stdcall CA::FxStringOut(BSTR* pbstrOut)
{
    const wchar_t wsz[] = L"[Строка из FxStringOut]" ;

    // Выделить память под исходящую строку.
    *pbstrOut = ::SysAllocString(wsz) ;
    if (*pbstrOut == NULL)
    {
        return E_OUTOFMEMORY ;
    }
    return S_OK ;
}

HRESULT __stdcall CA::FxFakeError()
{
    trace("FxFakeError имитирует ошибку.") ;

    // Создать объект информации об ошибке.
    ICreateErrorInfo* pICreateErr ;
    HRESULT hr = ::CreateErrorInfo(&pICreateErr) ;
    if (FAILED(hr))
    {
        return E_FAIL ;
    }

    // Установить источник ошибки.
    pICreateErr->SetSource(L"InsideCOM.Chap11") ;
    pICreateErr->SetDescription(
        L"Это поддельная ошибка, сгенерированная компонентом.") ;
    IErrorInfo* pIErrorInfo = NULL ;
    hr = pICreateErr->QueryInterface(IID_IErrorInfo,
                                     (void**)&pIErrorInfo) ;
    if (SUCCEEDED(hr))
    {
        ::SetErrorInfo(0L, pIErrorInfo) ;
        pIErrorInfo->Release() ;
    }
    pICreateErr->Release() ;
    return E_FAIL ;
}

//
// Конструктор
//
CA::CA(IUnknown* pUnknownOuter)
: CUnknown(pUnknownOuter), 
  m_pITypeInfo(NULL)
{
    // Пусто
}

//
// Деструктор
//
CA::~CA()
{
    if (m_pITypeInfo != NULL)
    {
        m_pITypeInfo->Release() ;
    }

    trace("Уничтожить себя.") ;
}

//
// Реализация NondelegatingQueryInterface
//
HRESULT __stdcall CA::NondelegatingQueryInterface(const IID& iid,   //не делегирует запрос на получение интерфейса базовому классу или другому объекту. Вместо этого, он обрабатывает запросы непосредственно в самом классе.
                                                  void** ppv)
{   
    if (iid == IID_IX)
    {
        return FinishQI(static_cast<IX*>(this), ppv) ;
    }
    else     if (iid == IID_IDispatch)
    {
        trace("Запрошен IDispatch.") ;
        return FinishQI(static_cast<IDispatch*>(this), ppv) ;
    }
    else     if (iid == IID_ISupportErrorInfo)
    {
        trace("Запрошен ISupportErrorInfo.") ;
        return FinishQI(static_cast<ISupportErrorInfo*>(this), ppv) ;
    }
    else
    {
        return CUnknown::NondelegatingQueryInterface(iid, ppv) ;
    }
}

//
// Загрузить и зарегистрировать библиотеку типов.
//
HRESULT CA::Init()
{
    HRESULT hr ;

    // Загрузить TypeInfo по требованию, если мы его еще не загрузили.
    if (m_pITypeInfo == NULL)
    {
        ITypeLib* pITypeLib = NULL ;
        hr = ::LoadRegTypeLib(LIBID_ServerLib, 
                              1, 0, // Мажорная/минорная версии
                              0x00, 
                              &pITypeLib) ;
        if (FAILED(hr)) 
        {
            trace("Загрузка реестра библиотеки типов не удалась, теперь пытаемся загрузить из файла.", hr) ;

            // Если она не была зарегистрирована, попытаемся загрузить ее из пути.

            // Получить полное имя исполняемого файла сервера.
            char szModule[512] ;
            DWORD dwResult = ::GetModuleFileName(CFactory::s_hModule,
                                                 szModule,
                                                 512) ; 

            // Разделить полное имя, чтобы получить путь.
            char szDrive[_MAX_DRIVE];
            char szDir[_MAX_DIR];
            _splitpath(szModule, szDrive, szDir, NULL, NULL) ;

            // Добавить имя реестра.
            char szTypeLibFullName[_MAX_PATH];
            sprintf(szTypeLibFullName,
                    "%s%s%s",
                    szDrive,
                    szDir,
                    szTypeLibName) ;

            // Преобразовать в широкий символ
            wchar_t wszTypeLibFullName[_MAX_PATH] ;
            mbstowcs(wszTypeLibFullName, szTypeLibFullName, _MAX_PATH) ;

            // Если LoadTypeLib выполнится успешно, она автоматически зарегистрирует
            // библиотеку типов для нас в следующий раз.  
            hr = ::LoadTypeLib(wszTypeLibFullName,
                               &pITypeLib) ;
            if(FAILED(hr))        
            {
                trace("Загрузка библиотеки типов не удалась.", hr) ;
                return hr;   
            }

            // Убедиться, что библиотека типов зарегистрирована.
            hr = RegisterTypeLib(pITypeLib, wszTypeLibFullName, NULL) ;
            if(FAILED(hr))        
            {
                trace("Регистрация библиотеки типов не удалась.", hr) ;
                return hr ;   
            }
        }

        // Получить информацию о типе интерфейса объекта.
        hr = pITypeLib->GetTypeInfoOfGuid(IID_IX,
                                          &m_pITypeInfo) ;
        pITypeLib->Release() ;
        if (FAILED(hr))  
        { 
            trace("Не удалось получить информацию о типе по GUID.", hr) ;
            return hr ;
        }   
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Функция создания, используемая CFactory
//
HRESULT CA::CreateInstance(IUnknown* pUnknownOuter,
                           CUnknown** ppNewComponent ) 
{
    if (pUnknownOuter != NULL)
    {
        // Не разрешать агрегацию (просто так).
        return CLASS_E_NOAGGREGATION ;
    }

    *ppNewComponent = new CA(pUnknownOuter) ;
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Реализация IDispatch
//
HRESULT __stdcall CA::GetTypeInfoCount(UINT* pCountTypeInfo)
{
    trace("Вызов GetTypeInfoCount выполнен.") ;
    *pCountTypeInfo = 1 ;
    return S_OK ;
}

HRESULT __stdcall CA::GetTypeInfo(
    UINT iTypeInfo,
    LCID,          // Этот объект не поддерживает локализацию.
    ITypeInfo** ppITypeInfo)
{    
    *ppITypeInfo = NULL ;

    if(iTypeInfo != 0)
    {
        trace("Вызов GetTypeInfo завершен неудачно -- неверный индекс iTypeInfo.") ;
        return DISP_E_BADINDEX ; 
    }

    trace("Вызов GetTypeInfo выполнен успешно.") ;

    // Вызов AddRef и возврат указателя.
    m_pITypeInfo->AddRef() ; 
    *ppITypeInfo = m_pITypeInfo ;
    return S_OK ;
}

HRESULT __stdcall CA::GetIDsOfNames(  
    const IID& iid,
    OLECHAR** arrayNames,
    UINT countNames,
    LCID,          // Локализация не поддерживается.
    DISPID* arrayDispIDs)
{
    if (iid != IID_NULL)
    {
        trace("Вызов GetIDsOfNames выполнен неудачно -- неверный IID.") ;
        return DISP_E_UNKNOWNINTERFACE ;
    }

    trace("Вызов GetIDsOfNames выполнен успешно.") ;
    HRESULT hr = m_pITypeInfo->GetIDsOfNames(arrayNames,
                                             countNames,
                                             arrayDispIDs) ;
    return hr ;
}

HRESULT __stdcall CA::Invoke(   
      DISPID dispidMember,
      const IID& iid,
      LCID,          // Локализация не поддерживается.
      WORD wFlags,
      DISPPARAMS* pDispParams,
      VARIANT* pvarResult,
      EXCEPINFO* pExcepInfo,
      UINT* pArgErr)
{        
    if (iid != IID_NULL)
    {
        trace("Вызов Invoke выполнен неудачно -- неверный IID.") ;
        return DISP_E_UNKNOWNINTERFACE ;
    }

    ::SetErrorInfo(0, NULL) ;

    trace("Вызов Invoke выполнен успешно.") ;
    HRESULT hr = m_pITypeInfo->Invoke(
        static_cast<IDispatch*>(this),
        dispidMember, wFlags, pDispParams,
        pvarResult, pExcepInfo, pArgErr) ; 
    return hr ;
}
