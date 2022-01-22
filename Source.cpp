#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <Windows.h>
#include <Iphlpapi.h>
#include <winnetwk.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "mpr.lib")

char* getMACAddress();
bool EnumerateFunc(LPNETRESOURCE lpnr);
void DisplayStruct(int i, LPNETRESOURCE lpnr);

int main()
{
    char* pMacAddress = getMACAddress();
    free(pMacAddress);

    LPNETRESOURCE lpnr = NULL;
    if (EnumerateFunc(lpnr) == FALSE)
        printf("Call to EnumerateFunc failed\n");

    getchar();
    return 0;
}

//Функция получения MAC-адресов всех сетевых адаптеров на компьютере
char* getMACAddress()
{
    PIP_ADAPTER_INFO AdapterInfo;   //Связанный список структур, содержащих информацию об адаптерах
    DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);   //размер буфера для хранения структуры AdapterInfo
    char* mac_addr = (char*)malloc(18);     //MAC-адрес

    //Выделение памяти под структуру AdapterInfo
    AdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (AdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        free(mac_addr);
        return NULL;
    }

    //Первоначальный вызов функции GetAdaptersInfo для инициализации размера буфера и повторного выделения
    //необходимого количества памяти
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(AdapterInfo);

        //Выделение памяти под структуру AdapterInfo
        AdapterInfo = (IP_ADAPTER_INFO*)malloc(dwBufLen);
        if (AdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            free(mac_addr);
            return NULL;
        }
    }

    //Счетчик порядковых номеров адаптеров
    int counter = 1;

    //Получения списка сетевых адаптеров
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR)
    {
        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
        //Пока список адаптеров не пуст, выводим инфу о текущем адаптере
        while (pAdapterInfo)
        {
            sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
                pAdapterInfo->Address[0], pAdapterInfo->Address[1],
                pAdapterInfo->Address[2], pAdapterInfo->Address[3],
                pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
            printf("%d.\n", counter++);
            printf("Adapter name: %s.\n", pAdapterInfo->AdapterName);
            printf("Description: %s.\n", pAdapterInfo->Description);
            printf("Address: %s, mac: %s\n", pAdapterInfo->IpAddressList.IpAddress.String, mac_addr);

            printf("\n");
            pAdapterInfo = pAdapterInfo->Next;
        }
    }

    free(AdapterInfo);
    return mac_addr;
}

bool EnumerateFunc(LPNETRESOURCE lpnr)
{
    DWORD dwResult, dwResultEnum;
    HANDLE hEnum;
    DWORD cbBuffer = 16384;
    DWORD cEntries = -1; // Искать все объекты
    LPNETRESOURCE lpnrLocal;
    DWORD i;

    // Вызов функции WNetOpenEnum для начала перечисления компьютеров.
    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // все сетевые ресурсы
        RESOURCETYPE_ANY, // все типы ресурсов
        0, // перечислить все ресурсы
        lpnr, // равно NULL при первом вызове функции
        &hEnum); // дескриптор ресурса

    if (dwResult != NO_ERROR)
    {
        // Обработка ошибок.
        printf("WNetOpenEnum error %d!\n", dwResult);
        return false;
    }

    // Вызвов функции GlobalAlloc для выделения ресурсов.
    lpnrLocal = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL)
        return FALSE;

    do
    {
        // Инициализируем буфер.
        ZeroMemory(lpnrLocal, cbBuffer);
        // Вызов функции WNetEnumResource для продолжения перечисления
        // доступных ресурсов сети.
        dwResultEnum = WNetEnumResource(hEnum,
            &cEntries, // Определено выше как -1
            lpnrLocal,
            &cbBuffer); // размер буфера

            // Если вызов был успешен, то структуры обрабатываются циклом.
        if (dwResultEnum == NO_ERROR)
        {
            for (i = 0; i < cEntries; i++)
            {
                // Вызов определенной в приложении функции для отображения
                // содержимого структур NETRESOURCE.
                DisplayStruct(i, &lpnrLocal[i]);
                // Если структура NETRESOURCE является контейнером, то
                // функци\ EnumerateFunc вызывается рекурсивно.
                if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
                    & RESOURCEUSAGE_CONTAINER))
                    if (!EnumerateFunc(&lpnrLocal[i]))
                        printf("EnumerateFunc returned FALSE!\n");
            }
        }
        // Обработка ошибок.
        else if (dwResultEnum != ERROR_NO_MORE_ITEMS)
        {
            printf("WNetEnumResource error %d!\n", dwResultEnum);
            break;
        }
    } while (dwResultEnum != ERROR_NO_MORE_ITEMS);

    // Вызов функции GlobalFree для очистки ресурсов.
    GlobalFree((HGLOBAL)lpnrLocal);
    // Вызов WNetCloseEnum для остановки перечисления.
    dwResult = WNetCloseEnum(hEnum);
    if (dwResult != NO_ERROR)
    {
        // Обработка ошибок.
        printf("WNetCloseEnum error %d!\n", dwResult);
        return false;
    }
    return true;
}

void DisplayStruct(int i, LPNETRESOURCE lpnrLocal)
{
    printf("NETRESOURCE[%d] Scope: ", i);
    switch (lpnrLocal->dwScope) {
    case (RESOURCE_CONNECTED):
        printf("connected\n");
        break;
    case (RESOURCE_GLOBALNET):
        printf("all resources\n");
        break;
    case (RESOURCE_REMEMBERED):
        printf("remembered\n");
        break;
    default:
        printf("unknown scope %d\n", lpnrLocal->dwScope);
        break;
    }

    printf("NETRESOURCE[%d] Type: ", i);
    switch (lpnrLocal->dwType) {
    case (RESOURCETYPE_ANY):
        printf("any\n");
        break;
    case (RESOURCETYPE_DISK):
        printf("disk\n");
        break;
    case (RESOURCETYPE_PRINT):
        printf("print\n");
        break;
    default:
        printf("unknown type %d\n", lpnrLocal->dwType);
        break;
    }

    printf("NETRESOURCE[%d] DisplayType: ", i);
    switch (lpnrLocal->dwDisplayType) {
    case (RESOURCEDISPLAYTYPE_GENERIC):
        printf("generic\n");
        break;
    case (RESOURCEDISPLAYTYPE_DOMAIN):
        printf("domain\n");
        break;
    case (RESOURCEDISPLAYTYPE_SERVER):
        printf("server\n");
        break;
    case (RESOURCEDISPLAYTYPE_SHARE):
        printf("share\n");
        break;
    case (RESOURCEDISPLAYTYPE_FILE):
        printf("file\n");
        break;
    case (RESOURCEDISPLAYTYPE_GROUP):
        printf("group\n");
        break;
    case (RESOURCEDISPLAYTYPE_NETWORK):
        printf("network\n");
        break;
    default:
        printf("unknown display type %d\n", lpnrLocal->dwDisplayType);
        break;
    }

    printf("NETRESOURCE[%d] Usage: 0x%x = ", i, lpnrLocal->dwUsage);
    if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONNECTABLE)
        printf("connectable ");
    if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONTAINER)
        printf("container ");
    printf("\n");

    printf("NETRESOURCE[%d] Localname: %S\n", i, lpnrLocal->lpLocalName);
    printf("NETRESOURCE[%d] Remotename: %S\n", i, lpnrLocal->lpRemoteName);
    printf("NETRESOURCE[%d] Comment: %S\n", i, lpnrLocal->lpComment);
    printf("NETRESOURCE[%d] Provider: %S\n", i, lpnrLocal->lpProvider);
    printf("\n");
}