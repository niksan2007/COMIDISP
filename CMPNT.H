//
// Cmpnt.cpp - Компонент
//

#include "Iface.h"
#include "CUnknown.h" 

///////////////////////////////////////////////////////////
//
// Компонент A
//
class CA : public CUnknown,
           public IX,
           public ISupportErrorInfo
{
public:	
	// Создание
	static HRESULT CreateInstance(IUnknown* pUnknownOuter,
	                              CUnknown** ppNewComponent ) ;

private:
	// Объявление делегирующего IUnknown.
	DECLARE_IUNKNOWN

	// IUnknown
	virtual HRESULT __stdcall NondelegatingQueryInterface(const IID& iid,
	                                                      void** ppv) ;

	// IDispatch
	virtual HRESULT __stdcall GetTypeInfoCount(UINT* pCountTypeInfo) ;

	virtual HRESULT __stdcall GetTypeInfo(
		UINT iTypeInfo,
		LCID,              // Локализация не поддерживается.
		ITypeInfo** ppITypeInfo) ;
	
	virtual HRESULT __stdcall GetIDsOfNames(
		const IID& iid,
		OLECHAR** arrayNames,
		UINT countNames,
		LCID,              // Локализация не поддерживается.
		DISPID* arrayDispIDs) ;

	virtual HRESULT __stdcall Invoke(   
		DISPID dispidMember,
		const IID& iid,
		LCID,              // Локализация не поддерживается.
		WORD wFlags,
		DISPPARAMS* pDispParams,
		VARIANT* pvarResult,
		EXCEPINFO* pExcepInfo,
		UINT* pArgErr) ;

	// Интерфейс IX
	virtual HRESULT  __stdcall Fx() ;
	virtual HRESULT __stdcall FxStringIn(BSTR bstrIn) ; 
	virtual HRESULT __stdcall FxStringOut(BSTR* pbstrOut) ;
	virtual HRESULT __stdcall FxFakeError() ;

	// ISupportErrorInfo
	virtual HRESULT __stdcall InterfaceSupportsErrorInfo(const IID& riid)
	{
		return (riid == IID_IX) ? S_OK : S_FALSE ;
	}

	// Инициализация
 	virtual HRESULT Init() ;

	// Конструктор
	CA(IUnknown* pUnknownOuter) ;

	// Деструктор
	~CA() ;

	// Указатель на информацию о типе.
	ITypeInfo* m_pITypeInfo ;
} ;
