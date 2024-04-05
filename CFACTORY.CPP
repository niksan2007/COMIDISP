///////////////////////////////////////////////////////////
//
// CFactory
//   - Базовый класс для повторного использования единственной фабрики классов для всех компонентов в DLL
//
#include <objbase.h>

#include "Registry.h"
#include "CFactory.h"

///////////////////////////////////////////////////////////
//
// Статические переменные
//
LONG CFactory::s_cServerLocks = 0 ;    // Счетчик блокировок

HMODULE CFactory::s_hModule = NULL ;   // Дескриптор модуля DLL

#ifdef _OUTPROC_SERVER_
DWORD CFactory::s_dwThreadID = 0 ;
#endif

///////////////////////////////////////////////////////////
//
// Реализация CFactory
//

CFactory::CFactory(const CFactoryData* pFactoryData)
: m_cRef(1)
{
	m_pFactoryData = pFactoryData ;
}

//
// Реализация IUnknown
//
HRESULT __stdcall CFactory::QueryInterface(REFIID iid, void** ppv)
{ 	
	IUnknown* pI ;
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		pI = this ; 
	}
	else
	{
	   *ppv = NULL;
		return E_NOINTERFACE;
	}
	pI->AddRef() ;
	*ppv = pI ;
	return S_OK;
}

ULONG __stdcall CFactory::AddRef() 
{ 
	return ::InterlockedIncrement(&m_cRef) ; 
}

ULONG __stdcall CFactory::Release() 
{
	if (::InterlockedDecrement(&m_cRef) == 0) 
	{
		delete this; 
		return 0 ;
	}   
	return m_cRef;
}

//
// Реализация IClassFactory
//

HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv) 
{

	// Агрегация только если запрошенный IID - IID_IUnknown.
	if ((pUnknownOuter != NULL) && (iid != IID_IUnknown))
	{
		return CLASS_E_NOAGGREGATION ;
	}

	// Создание компонента.
	CUnknown* pNewComponent ;
	HRESULT hr = m_pFactoryData->CreateInstance(pUnknownOuter,
	                                            &pNewComponent) ;
	if (FAILED(hr))
	{
		return hr ;
	}

	// Инициализация компонента.
	hr = pNewComponent->Init();
	if (FAILED(hr))
	{
		// Инициализация не удалась. Освобождение компонента.
		pNewComponent->NondelegatingRelease() ;
		return hr ;
	}
	
	// Получение запрошенного интерфейса.
	hr = pNewComponent->NondelegatingQueryInterface(iid, ppv) ;

	// Освобождение ссылки, удерживаемой фабрикой классов.
	pNewComponent->NondelegatingRelease() ;
	return hr ;   
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock) 
{
	if (bLock) 
	{
		::InterlockedIncrement(&s_cServerLocks) ; 
	}
	else
	{
		::InterlockedDecrement(&s_cServerLocks) ;
	}
	// Если это сервер, работающий вне процесса, проверяем, необходимо ли завершить работу.
	CloseExe() ;  //@local

	return S_OK ;
}


///////////////////////////////////////////////////////////
//
// GetClassObject
//   - Создание фабрики классов на основе CLSID.
//
HRESULT CFactory::GetClassObject(const CLSID& clsid, 
                                 const IID& iid, 
                                 void** ppv)
{
	if ((iid != IID_IUnknown) && (iid != IID_IClassFactory))
	{
		return E_NOINTERFACE ;
	}

	// Обход массива данных в поисках этого идентификатора класса.
	for (int i = 0; i < g_cFactoryDataEntries; i++)
	{
		const CFactoryData* pData = &g_FactoryDataArray[i] ;
		if (pData->IsClassID(clsid))
		{

			// Найден ClassID в массиве компонентов, которые можно создать.
			// Так что создаем фабрику классов для этого компонента.
			// Передаем структуру CFactoryData в фабрику классов,
			// чтобы она знала, какие компоненты создавать.
			*ppv = (IUnknown*) new CFactory(pData) ;
			if (*ppv == NULL)
			{
				return E_OUTOFMEMORY ;
			}
			return NOERROR ;
		}
	}
	return CLASS_E_CLASSNOTAVAILABLE ;
}

//
// Определение возможности выгрузки компонента.
//
HRESULT CFactory::CanUnloadNow()
{
	if (CUnknown::ActiveComponents() || IsLocked())
	{
		return S_FALSE ;
	}
	else
	{
		return S_OK ;
	}
}

//
// Регистрация всех компонентов.
//
HRESULT CFactory::RegisterAll()
{
	for(int i = 0 ; i < g_cFactoryDataEntries ; i++)
	{
		RegisterServer(s_hModule,
		               *(g_FactoryDataArray[i].m_pCLSID),
		               g_FactoryDataArray[i].m_RegistryName,
		               g_FactoryDataArray[i].m_szVerIndProgID, 
		               g_FactoryDataArray[i].m_szProgID,
		               *(g_FactoryDataArray[i].m_pLIBID)) ;
	}
	return S_OK ;
}   
	
HRESULT CFactory::UnregisterAll()
{
	for(int i = 0 ; i < g_cFactoryDataEntries ; i++)   
	{
		UnregisterServer(*(g_FactoryDataArray[i].m_pCLSID),
		                 g_FactoryDataArray[i].m_szVerIndProgID, 
		                 g_FactoryDataArray[i].m_szProgID) ;
	}
	return S_OK ;
}

#ifndef _OUTPROC_SERVER_
//////////////////////////////////////////////////////////
//
// Экспортируемые функции
//

STDAPI DllCanUnloadNow()
{
	return CFactory::CanUnloadNow() ; 
}

//
// Получить фабрику классов
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv) 
{
	return CFactory::GetClassObject(clsid, iid, ppv) ;
}

//
// Регистрация сервера
//
STDAPI DllRegisterServer()
{
	return CFactory::RegisterAll() ;
}


STDAPI DllUnregisterServer()
{
	return CFactory::UnregisterAll() ;
}

///////////////////////////////////////////////////////////
//
// Информация о модуле DLL
//
BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD dwReason, 
                      void* lpReserved )
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CFactory::s_hModule = hModule ;
	}
	return TRUE ;
}

#else
//////////////////////////////////////////////////////////
//
// Поддержка сервера вне процесса
//

//
// Запуск фабрик
//
BOOL CFactory::StartFactories()
{
	CFactoryData* pStart = &g_FactoryDataArray[0] ;
	const CFactoryData* pEnd =
		&g_FactoryDataArray[g_cFactoryDataEntries - 1] ;

	for(CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
	{
		// Инициализация указателя и куки класса фабрики.
		pData->m_pIClassFactory = NULL ;
		pData->m_dwRegister = NULL ;

		// Создание фабрики классов для этого компонента.
		IClassFactory* pIFactory = new CFactory(pData) ;

		// Регистрация фабрики классов.
		DWORD dwRegister ;
		HRESULT hr = ::CoRegisterClassObject(
		                  *pData->m_pCLSID,
		                  static_cast<IUnknown*>(pIFactory),
		                  CLSCTX_LOCAL_SERVER,
		                  REGCLS_MULTIPLEUSE,
		                  // REGCLS_MULTI_SEPARATE, //@Multi
		                  &dwRegister) ;
		if (FAILED(hr))
		{
			pIFactory->Release() ;
			return FALSE ;
		}

		// Установка данных.
		pData->m_pIClassFactory = pIFactory ;
		pData->m_dwRegister = dwRegister ;
	}
	return TRUE ;
}

//
// Остановка фабрик
//
void CFactory::StopFactories()
{
	CFactoryData* pStart = &g_FactoryDataArray[0] ;
	const CFactoryData* pEnd =
		&g_FactoryDataArray[g_cFactoryDataEntries - 1] ;

	for (CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
	{
		// Получение магического куки и остановка работы фабрики.
		DWORD dwRegister = pData->m_dwRegister ;
		if (dwRegister != 0) 
		{
			::CoRevokeClassObject(dwRegister) ;
		}

		// Освобождение фабрики классов.
		IClassFactory* pIFactory  = pData->m_pIClassFactory ;
		if (pIFactory != NULL) 
		{
			pIFactory->Release() ;
		}
	}
}

#endif //_OUTPROC_SERVER_
