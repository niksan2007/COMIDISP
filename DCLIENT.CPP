//
// DClient.cpp - Реализация клиента с использованием диспетчерского интерфейса
//
//   Этот клиент подключается к двойному интерфейсу IX
//   через dispinterface.
//

#include <windows.h>
#include "Util.h"

static inline void trace(char* msg)
	{ Util::Trace("DClient", msg, S_OK) ;}
static inline void trace(char* msg, HRESULT hr)
	{ Util::Trace("DClient", msg, hr) ;}

int main()
{
	cout << "К какому серверу вы хотите подключиться?\r\n"
	     << "1) Сервер в процессе\r\n" 
	     << "2) Локальный сервер.\r\n:" ;
	int i = 0 ;
	cin >> i ;

	DWORD clsctx ;
	if (i == 1)
	{
		clsctx = CLSCTX_INPROC_SERVER ;
		trace("Попытка создать компонент в процессе.") ; //приложение пытается подключиться к COM-серверу, который работает в том же процессе, что и само приложение
	}
	else
	{
		clsctx = CLSCTX_LOCAL_SERVER ;
		trace("Попытка создать локальный компонент.") ; //приложение пытается подключиться к COM-серверу, который запущен отдельно от клиентского приложения в другом процессе
	}

	HRESULT hr = OleInitialize(NULL) ;
	if (FAILED(hr))
	{
		trace("Не удалось инициализировать.", hr) ;
		return 1 ;
	}

	// Получение CLSID приложения.
	wchar_t progid[] = L"InsideCOM.Chap11" ;
	CLSID clsid ;
 	hr = ::CLSIDFromProgID(progid, &clsid) ;
	if(FAILED(hr))
	{
		trace("Не удалось получить CLSID.", hr) ;
		return 1 ;
	}

	// Создание компонента. Работаем с компонентом через IDispatch
	IDispatch* pIDispatch = NULL ;
	hr = ::CoCreateInstance(clsid,
	                        NULL,
	                        clsctx,
	                        IID_IDispatch,
	                        (void**)&pIDispatch) ;
	if (FAILED(hr))
	{
		trace("Создание экземпляра не удалось.", hr) ;
		OleUninitialize() ;
		return 1 ;
	}
	trace("CoCreateInstance успешно.") ;

	// Сначала нам нужно получить идентификаторы для названий функций.
	trace("Получить DispID для функции \"Fx\".") ;

	DISPID dispid ;	
	OLECHAR* name = L"Fx" ;
	hr = pIDispatch->GetIDsOfNames(IID_NULL,
	                               &name,
	                               1,
	                               GetUserDefaultLCID(),
	                               &dispid) ;
	if (FAILED(hr))
	{
		trace("Запрос GetIDsOfNames не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}
	
	// Подготовка аргументов для Fx.
	DISPPARAMS dispparamsNoArgs = {
		NULL, 
		NULL, 
		0,    // Ноль аргументов
		0     // Ноль именованных аргументов
	} ;

	trace("Вызов функции \"Fx\".") ;
	hr = pIDispatch->Invoke(dispid,
	                        IID_NULL,
	                        GetUserDefaultLCID(),
	                        DISPATCH_METHOD,
	                        &dispparamsNoArgs,
	                        NULL, 
	                        NULL, 
	                        NULL) ;
	if (FAILED(hr))
	{
		trace("Вызов не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}

	//
	// Теперь передаем BSTR компоненту.
	//
	trace("Получить DispID для функции \"FxStringIn\".") ;
	name = L"FxStringIn" ;
	hr = pIDispatch->GetIDsOfNames(IID_NULL,
	                               &name,
	                               1,
	                               GetUserDefaultLCID(),
	                               &dispid) ;
	if (FAILED(hr))
	{
		trace("Запрос GetIDsOfNames не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}
	
	// Передаем следующую строку компоненту.
	wchar_t wszIn[] = L"This is the test." ;

	// Преобразуем строку широких символов в BSTR.
	BSTR bstrIn ;
	bstrIn = ::SysAllocString(wszIn) ; // Выделяет новую строку и копирует в нее переданную 

	// Подготовка параметров для вызова invoke.

	// Выделение и инициализация аргумента VARIANT.
	VARIANTARG varg ; //Струтура типов
	::VariantInit(&varg) ;     // Инициализация VARIANT.
	varg.vt = VT_BSTR ;        // Тип данных VARIANT
	varg.bstrVal = bstrIn ;    // Данные для VARIANT

	// Заполнение структуры DISPPARAMS.
	DISPPARAMS param ;
	param.cArgs = 1 ;                 // Количество аргументов
	param.rgvarg = &varg ;            // Аргументы
	param.cNamedArgs = 0 ;            // Количество именованных аргументов
	param.rgdispidNamedArgs = NULL ;  // Именованные аргументы


	trace("Вызов функции \"FxStringIn\".") ;
	hr = pIDispatch->Invoke(dispid,
	                        IID_NULL,
	                        GetUserDefaultLCID(),
	                        DISPATCH_METHOD,
	                        &param,
	                        NULL,
	                        NULL,
	                        NULL) ;
	if (FAILED(hr))
	{
		trace("Вызов не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}

	// Очистка
	::SysFreeString(bstrIn) ;

	//
	// Теперь получаем BSTR от компонента.
	//

	// Получаем dispid.
	trace("Получить DispID для функции \"FxStringOut\".") ;
	name = L"FxStringOut" ;
	hr = pIDispatch->GetIDsOfNames(IID_NULL,
	                               &name,
	                               1,
	                               GetUserDefaultLCID(),
	                               &dispid) ;
	if (FAILED(hr))
	{
		trace("Запрос GetIDsOfNames не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}

	// Выделение варианта для возвращаемого параметра.
	VARIANT varResult ;
	::VariantInit(&varResult) ;

	// Вызов функции.
	trace("Вызов функции \"FxStringOut\".") ;
	hr = pIDispatch->Invoke(dispid,
	                        IID_NULL,
	                        GetUserDefaultLCID(),
	                        DISPATCH_METHOD,
	                        &dispparamsNoArgs, //&param,
	                        &varResult,
	                        NULL,
	                        NULL) ;
	if (FAILED(hr))
	{
		trace("Вызов не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}

	// Отображение возвращенной строки.
	if (varResult.vt == VT_BSTR)
	{
		strstream sout ;
		sout << "Строка, возвращенная от компонента: "
		     << varResult.bstrVal
		     << ends ;
		trace(sout.str()) ;

		// Освобождение строки.
		::SysFreeString(varResult.bstrVal) ;
	}

	//
	// Показать, как обрабатывать функцию, возвращающую EXCEPINFO.
	//
	trace("Получить DispID для функции \"FxFakeError\"") ;
	name = L"FxFakeError" ;
	hr = pIDispatch->GetIDsOfNames(IID_NULL,
	                               &name,
	                               1,
	                               GetUserDefaultLCID(),
	                               &dispid) ;
	if (FAILED(hr))
	{
		trace("Запрос GetIDsOfNames не удался.", hr) ;
		pIDispatch->Release() ;
		return 1 ;
	}

	EXCEPINFO excepinfo ;

	trace("Вызов функции \"FxFakeError\".") ;
	hr = pIDispatch->Invoke(dispid,
	                        IID_NULL,
	                        GetUserDefaultLCID(),
	                        DISPATCH_METHOD,
	                        &dispparamsNoArgs,
	                        NULL,
	                        &excepinfo,
	                        NULL) ;

	if (FAILED(hr))
	{
		trace("FxFakeError не удался.", hr) ;
		if (hr == DISP_E_EXCEPTION)
		{
			trace("У нас есть информация об ошибке от компонента.") ;

			if (excepinfo.pfnDeferredFillIn != NULL)
			{
				(*(excepinfo.pfnDeferredFillIn))(&excepinfo) ;
			}

			strstream sout ;
			sout << "Информация от компонента:  "
			     << excepinfo.bstrSource
			     << ends ;
			trace(sout.str()) ;
		}
	}

	// Освободить диспетчерский интерфейс.
	pIDispatch->Release() ;

	// Деинициализировать библиотеку OLE.
	OleUninitialize() ;
	return 0 ;
}