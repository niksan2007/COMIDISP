#include "CFactory.h"
#include "Iface.h"
#include "Cmpnt.h"

///////////////////////////////////////////////////////////
//
// Server.cpp
//
//
// Следующий массив содержит данные, используемые CFactory
// для создания компонентов. Каждый элемент в массиве содержит
// CLSID, указатель на функцию создания и имя
// компонента для размещения в реестре.
//
CFactoryData g_FactoryDataArray[] =
{
    {&CLSID_Component, CA::CreateInstance, 
        "Пример из книги 'Inside COM', глава 11", // Дружественное имя
        "InsideCOM.Chap11",        // Программный идентификатор (ProgID)
        "InsideCOM.Chap11.1",      // Независимый от версии Программный идентификатор
        &LIBID_ServerLib,          // Идентификатор библиотеки типов (Type Library ID)
        NULL, 0}
};
int g_cFactoryDataEntries
    = sizeof(g_FactoryDataArray) / sizeof(CFactoryData);
