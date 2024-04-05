//
//
// util.cpp - Общие утилиты для вывода сообщений
//
//
#include <objbase.h>
#include <stdio.h>    //sprintf
#include <stdlib.h>
#include <assert.h>
// #include <tchar.h>

#include "util.h"

#ifdef _OUTPROC_SERVER_ 
// Мы создаем локальный или удаленный сервер.
	// Дескриптор окна списка
	extern HWND g_hWndListBox ;

	static inline void output(const char* sz)
	{
		::SendMessage(g_hWndListBox, LB_ADDSTRING, 0, (LPARAM)sz) ;
	}

#else
// Мы создаем внутренний сервер.
#include <iostream.h>
	static inline void output(const char* sz)
	{
		cout << sz << endl ;
	}
#endif //_OUTPROC_SERVER_

//
// Утилиты
//
namespace Util
{

//
// Вывести сообщение с меткой.
//
void Trace(char* szLabel, char* szText, HRESULT hr)
{
	char buf[256] ;
	sprintf(buf, "%s: \t%s", szLabel, szText) ;
	output(buf) ;

	if (FAILED(hr))
	{
		ErrorMessage(hr) ;
	}
}

//
// Вывести строку ошибки COM/OLE для HRESULT.
//
void ErrorMessage(HRESULT hr)
{
	void* pMsgBuf ;
 
	::FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Язык по умолчанию
		(LPTSTR)&pMsgBuf,
		0,
		NULL 
	) ;

	char buf[256] ;
	sprintf(buf, "Ошибка (%x): %s", hr, (char*)pMsgBuf) ;
	output(buf) ;
		
	// Освободить буфер.
	LocalFree(pMsgBuf) ;
}

} ; // Конец пространства имен Util


//
// Перегруженный оператор вставки потока ostream
// Преобразует из wchar_t в char
//
ostream& operator<< ( ostream& os, const wchar_t* wsz )
{
	// Длина входной строки
	int iLength = wcslen(wsz)+1 ;

	// Выделить буфер для преобразованной строки.
	char* psz = new char[iLength] ;

	// Преобразовать из wchar_t в char.
	wcstombs(psz, wsz, iLength) ;

	// Отправить его.
	os << psz ;

	// Очистка
	delete [] psz ;
	return os ;
}
