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

//������� ��������� MAC-������� ���� ������� ��������� �� ����������
char* getMACAddress()
{
    PIP_ADAPTER_INFO AdapterInfo;   //��������� ������ ��������, ���������� ���������� �� ���������
    DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);   //������ ������ ��� �������� ��������� AdapterInfo
    char* mac_addr = (char*)malloc(18);     //MAC-�����

    //��������� ������ ��� ��������� AdapterInfo
    AdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (AdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        free(mac_addr);
        return NULL;
    }

    //�������������� ����� ������� GetAdaptersInfo ��� ������������� ������� ������ � ���������� ���������
    //������������ ���������� ������
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(AdapterInfo);

        //��������� ������ ��� ��������� AdapterInfo
        AdapterInfo = (IP_ADAPTER_INFO*)malloc(dwBufLen);
        if (AdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            free(mac_addr);
            return NULL;
        }
    }

    //������� ���������� ������� ���������
    int counter = 1;

    //��������� ������ ������� ���������
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR)
    {
        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
        //���� ������ ��������� �� ����, ������� ���� � ������� ��������
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
    DWORD cEntries = -1; // ������ ��� �������
    LPNETRESOURCE lpnrLocal;
    DWORD i;

    // ����� ������� WNetOpenEnum ��� ������ ������������ �����������.
    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // ��� ������� �������
        RESOURCETYPE_ANY, // ��� ���� ��������
        0, // ����������� ��� �������
        lpnr, // ����� NULL ��� ������ ������ �������
        &hEnum); // ���������� �������

    if (dwResult != NO_ERROR)
    {
        // ��������� ������.
        printf("WNetOpenEnum error %d!\n", dwResult);
        return false;
    }

    // ������ ������� GlobalAlloc ��� ��������� ��������.
    lpnrLocal = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL)
        return FALSE;

    do
    {
        // �������������� �����.
        ZeroMemory(lpnrLocal, cbBuffer);
        // ����� ������� WNetEnumResource ��� ����������� ������������
        // ��������� �������� ����.
        dwResultEnum = WNetEnumResource(hEnum,
            &cEntries, // ���������� ���� ��� -1
            lpnrLocal,
            &cbBuffer); // ������ ������

            // ���� ����� ��� �������, �� ��������� �������������� ������.
        if (dwResultEnum == NO_ERROR)
        {
            for (i = 0; i < cEntries; i++)
            {
                // ����� ������������ � ���������� ������� ��� �����������
                // ����������� �������� NETRESOURCE.
                DisplayStruct(i, &lpnrLocal[i]);
                // ���� ��������� NETRESOURCE �������� �����������, ��
                // ������\ EnumerateFunc ���������� ����������.
                if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
                    & RESOURCEUSAGE_CONTAINER))
                    if (!EnumerateFunc(&lpnrLocal[i]))
                        printf("EnumerateFunc returned FALSE!\n");
            }
        }
        // ��������� ������.
        else if (dwResultEnum != ERROR_NO_MORE_ITEMS)
        {
            printf("WNetEnumResource error %d!\n", dwResultEnum);
            break;
        }
    } while (dwResultEnum != ERROR_NO_MORE_ITEMS);

    // ����� ������� GlobalFree ��� ������� ��������.
    GlobalFree((HGLOBAL)lpnrLocal);
    // ����� WNetCloseEnum ��� ��������� ������������.
    dwResult = WNetCloseEnum(hEnum);
    if (dwResult != NO_ERROR)
    {
        // ��������� ������.
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