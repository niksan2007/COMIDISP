#include <windows.h>
#include <stdlib.h>

#include "CUnknown.h"
#include "CFactory.h"
#include "Resource.h"

///////////////////////////////////////////////////////////
//
// Outproc.cpp
//   - сервер компонентов
//

HWND g_hWndListBox = NULL ; // Дескриптор списка, используемый для вывода информации.

BOOL InitWindow(int nCmdShow) ; // Прототип функции инициализации окна.

extern "C" LONG APIENTRY MainWndProc(HWND hWnd,
                                     UINT message,
                                     UINT wParam,
                                     LONG lParam) ;

//
// WinMain procedure
//
extern "C" int WINAPI WinMain(HINSTANCE hInstance, 
                              HINSTANCE hPrevInstance,
                              LPSTR lpCmdLine, 
                              int nCmdShow)
{
	// Контролирует, показывается ли пользовательский интерфейс.
	BOOL bUI = TRUE ;

	// Если TRUE, то не выполнять цикл.
	BOOL bExit = FALSE ;

	// Инициализация библиотеки OLE.
	HRESULT hr = ::OleInitialize(NULL) ; // Автоматизация OLE.
	if (FAILED(hr))
	{
		return 0 ;
	}
   
   
	// Получение ID потока.
	CFactory::s_dwThreadID = ::GetCurrentThreadId() ;
	CFactory::s_hModule = hInstance ;

	// Чтение командной строки.
	char szTokens[] = "-/" ;

	char* szToken = strtok(lpCmdLine, szTokens) ; 
	while (szToken != NULL)
	{
		if (_stricmp(szToken, "UnregServer") == 0)
		{
			CFactory::UnregisterAll() ;
			// Завершение работы, так как регистрация завершена.
			bExit = TRUE ;
			bUI = FALSE ;
		}
		else if (_stricmp(szToken, "RegServer") == 0)
		{
			CFactory::RegisterAll() ;
			// Завершение работы, так как регистрация завершена.
			bExit = TRUE ;
			bUI = FALSE ;
		}
		else if (_stricmp(szToken, "Embedding") == 0)
		{
			// Не показывать окно, если процесс встраивания.
			bUI = FALSE ;
			break ;
		}
		szToken = strtok(NULL, szTokens) ;
	}

	// Если пользователь запустил программу, показать пользовательский интерфейс.
	if (bUI)
	{
		if (!InitWindow(nCmdShow))
		{
			// Завершить, так как невозможно показать интерфейс.
			bExit = TRUE ;
		}
		else
		{
			::InterlockedIncrement(&CFactory::s_cServerLocks) ;
		}
	}

	if (!bExit)
	{
		// Зарегистрировать все фабрики классов.
		CFactory::StartFactories() ;

		// Ожидание завершения работы.
		MSG msg ;
		while (::GetMessage(&msg, 0, 0, 0))
		{
			::DispatchMessage(&msg) ;
		}

		// Снять регистрацию фабрик классов.
		CFactory::StopFactories() ;
	}

	// Деинициализация библиотеки OLE.
	::OleUninitialize() ;
	return 0 ;
}


//
// Инициализация окна
//
BOOL InitWindow(int nCmdShow) 
{
	// Заполнение структуры класса окна параметрами главного окна.
	WNDCLASS wcListview ;
	wcListview.style = 0 ;                     
	wcListview.lpfnWndProc = (WNDPROC)MainWndProc ; 
	wcListview.cbClsExtra = 0 ;              
	wcListview.cbWndExtra = 0 ;              
	wcListview.hInstance = CFactory::s_hModule ;
	wcListview.hIcon = ::LoadIcon(CFactory::s_hModule,
	                              MAKEINTRESOURCE(IDC_ICON)) ;
	wcListview.hCursor = ::LoadCursor(NULL, IDC_ARROW) ;
	wcListview.hbrBackground = ::GetStockObject(WHITE_BRUSH) ; 
	wcListview.lpszMenuName = NULL ;  
	wcListview.lpszClassName = "MyServerWinClass" ;

	BOOL bResult = ::RegisterClass(&wcListview) ;
	if (!bResult)
	{
		return bResult ;
	}

	HWND hWndMain ;

	hWndMain = ::CreateWindow(
		"MyServerWinClass",
		"Component Server", 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,               
		NULL,               
		CFactory::s_hModule,          
		NULL) ;

	// Если окно не было создано, вернуть "неудачу".
	if (!hWndMain)
	{
		return FALSE ;
	}

	// Сделать окно видимым, обновить его клиентскую область и вернуть "успех".
	::ShowWindow(hWndMain, nCmdShow) ;
	::UpdateWindow(hWndMain) ;
	return TRUE ;
}

//
// Основная процедура окна
//
extern "C" LONG APIENTRY MainWndProc(
	HWND hWnd,                // дескриптор окна
	UINT message,             // тип сообщения
	UINT wParam,              // дополнительная информация
	LONG lParam)              // дополнительная информация
{
	DWORD dwStyle ;

	switch (message) 
	{
	case WM_CREATE:
		{
			// Получить размеры главного окна
			CREATESTRUCT* pcs = (CREATESTRUCT*) lParam ;

			// Создать список для вывода.
			g_hWndListBox = ::CreateWindow(
				"LISTBOX",
				NULL, 
				WS_CHILD | WS_VISIBLE | LBS_USETABSTOPS
					| WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
				0, 0, pcs->cx, pcs->cy,
				hWnd,               
				NULL,               
				CFactory::s_hModule,          
				NULL) ;
			if (g_hWndListBox  == NULL)
			{
				// Список не создан.
				::MessageBox(NULL,
				             "Listbox not created!",
				             NULL,
				             MB_OK) ;
				return -1 ;
			}
		}
		break ;

	case WM_SIZE:
		::MoveWindow(g_hWndListBox, 0, 0,
			LOWORD(lParam), HIWORD(lParam), TRUE) ;
		break;

	case WM_DESTROY:          // сообщение: окно уничтожается
		if (CFactory::CanUnloadNow() == S_OK)
		{
			// Послать сообщение о завершении работы, если никто не использует программу.
			::PostQuitMessage(0) ;
		}
		break ;

	case WM_CLOSE:
		// Уменьшить счетчик блокировки.
		::InterlockedDecrement(&CFactory::s_cServerLocks) ;

		// Список исчезает.
		g_hWndListBox = NULL ;

		// Перейти к выполнению ниже.
		// Не использовать break, чтобы позволить следующему кейсу выполниться.
	default:
		return (DefWindowProc(hWnd, message, wParam, lParam)) ;
	}
	return 0 ;
}
