#ifndef __Util_h__
#define __Util_h__

//
// Util.h - Общие утилиты
//
#include <strstrea.h>

namespace Util
{
	void Trace(char* szLabel, char* szText, HRESULT hr) ;

	void ErrorMessage(HRESULT hr) ;
} ;


//
// Перегруженный оператор вставки для преобразования из
// Unicode (wchar_t) в не-Unicode.
//
ostream& operator<< ( ostream& os, const wchar_t* wsz ) ;

#endif // __Util_h__
