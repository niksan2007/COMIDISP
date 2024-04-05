///////////////////////////////////////////////////////////
//
// CUnknown.cpp 
//
// Реализация базового класса IUnknown
//
#include "CUnknown.h"
#include "CFactory.h"
#include "Util.h"

static inline void trace(char* msg)
	{Util::Trace("CUnknown", msg, S_OK) ;} 
static inline void trace(char* msg, HRESULT hr)
	{Util::Trace("CUnknown", msg, hr) ;}

///////////////////////////////////////////////////////////
//
// Счетчик активных объектов
//   - Используется для определения, можно ли выгрузить DLL.
//
long CUnknown::s_cActiveComponents = 0 ;


///////////////////////////////////////////////////////////
//
// Конструктор
//'nj
CUnknown::CUnknown(IUnknown* pUnknownOuter)
: m_cRef(1)
{
	// Установка указателя m_pUnknownOuter.
	if (pUnknownOuter == NULL)
	{
		trace("Не агрегируется; делегирование не делегируемому IUnknown.") ;
		m_pUnknownOuter = reinterpret_cast<IUnknown*>
		                     (static_cast<INondelegatingUnknown*>
		                     (this)) ;  // заметьте приведение
	}
	else
	{
		trace("Агрегируется; делегирование внешнему IUnknown.") ;
		m_pUnknownOuter = pUnknownOuter ;
	}

	// Инкремент счетчика активных компонентов.
	::InterlockedIncrement(&s_cActiveComponents) ;
}

//
// Деструктор
//
CUnknown::~CUnknown()
{
	::InterlockedDecrement(&s_cActiveComponents) ;

	// Если это сервер EXE, завершить его.
	CFactory::CloseExe() ;
}

//
// FinalRelease - вызывается Release перед удалением компонента
//
void CUnknown::FinalRelease()
{
	trace("Инкремент счетчика ссылок для окончательного освобождения.") ;
	m_cRef = 1 ;
}

//
// Неделегируемый IUnknown
//   - Переопределение для обработки пользовательских интерфейсов.
//
HRESULT __stdcall 
	CUnknown::NondelegatingQueryInterface(const IID& iid, void** ppv)
{
	// CUnknown поддерживает только IUnknown.
	if (iid == IID_IUnknown)
	{
		return FinishQI(reinterpret_cast<IUnknown*>
		                   (static_cast<INondelegatingUnknown*>(this)),
		                ppv) ;
	}	
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
}

//
// AddRef
//
ULONG __stdcall CUnknown::NondelegatingAddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

//
// Release
//
ULONG __stdcall CUnknown::NondelegatingRelease()
{
	InterlockedDecrement(&m_cRef) ;
	if (m_cRef == 0)
	{
		FinalRelease() ;
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}

//
// FinishQI
//   - Вспомогательная функция для упрощения переопределения
//     NondelegatingQueryInterface
//
HRESULT CUnknown::FinishQI(IUnknown* pI, void** ppv) 
{
	*ppv = pI ;
	pI->AddRef() ;
	return S_OK ;
}
