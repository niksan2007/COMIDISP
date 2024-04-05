#ifndef __CUnknown_h__
#define __CUnknown_h__

#include <objbase.h>

///////////////////////////////////////////////////////////
//
// Интерфейс Nondelegating IUnknown
//   - Неделегируемая версия интерфейса IUnknown
//
interface INondelegatingUnknown
{
	virtual HRESULT __stdcall 
		NondelegatingQueryInterface(const IID& iid, void** ppv) = 0 ;
	virtual ULONG   __stdcall NondelegatingAddRef() = 0 ;
	virtual ULONG   __stdcall NondelegatingRelease() = 0 ;
} ;


///////////////////////////////////////////////////////////
//
// Объявление CUnknown 
//   - Базовый класс для реализации IUnknown
//

class CUnknown : public INondelegatingUnknown
{
public:
	// Реализация неделегируемого IUnknown
	virtual HRESULT __stdcall NondelegatingQueryInterface(const IID&,
	                                                      void**) ;
	virtual ULONG   __stdcall NondelegatingAddRef() ;
	virtual ULONG   __stdcall NondelegatingRelease() ;

	// Конструктор
	CUnknown(IUnknown* pUnknownOuter) ;

	// Деструктор
	virtual ~CUnknown() ;

	// Инициализация (особенно для агрегатов)
	virtual HRESULT Init() { return S_OK ;}

	// Уведомление производных классов о том, что мы освобождаем ресурсы
	virtual void FinalRelease() ;

	// Количество активных компонентов в данный момент
	static long ActiveComponents() 
		{ return s_cActiveComponents ;}
	
	// Вспомогательная функция
	HRESULT FinishQI(IUnknown* pI, void** ppv) ;

protected:
	// Поддержка делегации
	IUnknown* GetOuterUnknown() const
		{ return m_pUnknownOuter ;}

private:
	// Счетчик ссылок на этот объект
	long m_cRef ;
	
	// Указатель на (внешний) внешний IUnknown
	IUnknown* m_pUnknownOuter ;

	// Количество всех активных экземпляров
	static long s_cActiveComponents ; 
} ;


///////////////////////////////////////////////////////////
//
// Делегируемый IUnknown
//   - Делегирует неделегируемый IUnknown или внешний IUnknown,
//     если компонент агрегируется.
//
#define DECLARE_IUNKNOWN		                             \
	virtual HRESULT __stdcall	                             \
		QueryInterface(const IID& iid, void** ppv)           \
	{	                                                     \
		return GetOuterUnknown()->QueryInterface(iid,ppv) ;  \
	} ;	                                                     \
	virtual ULONG __stdcall AddRef()	                     \
	{	                                                     \
		return GetOuterUnknown()->AddRef() ;                 \
	} ;	                                                     \
	virtual ULONG __stdcall Release()	                     \
	{	                                                     \
		return GetOuterUnknown()->Release() ;                \
	} ;


///////////////////////////////////////////////////////////


#endif
