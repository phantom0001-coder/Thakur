

BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM lParam) {
    DWORD targetPid = *reinterpret_cast<DWORD*>(lParam);
    DWORD procId = 0;
    GetWindowThreadProcessId(hWnd, &procId);
    if (procId == targetPid) {
        RECT r = {0};
        GetWindowRect(hWnd, &r);
        char title[256] = {0};
        GetWindowTextA(hWnd, title, 256);
        char cls[256] = {0};
        GetClassNameA(hWnd, cls, 256);
        std::cout << "HWND: " << hWnd 
                  << " Visible: " << IsWindowVisible(hWnd)
                  << " Rect: [" << r.left << "," << r.top << "," << r.right << "," << r.bottom << "]"
                  << " Class: " << cls
                  << " Title: " << title << std::endl;
        if (IsWindowVisible(hWnd) && (r.right - r.left) > 60 && (r.bottom - r.top) > 60) {
            *reinterpret_cast<DWORD*>(lParam) = 0;
            return FALSE;
        }
    }
    return TRUE;
}

int main() {
    DWORD pid = 7552;
    DWORD checkPid = pid;
    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&checkPid));
    std::cout << "hasActiveGuiWindow: " << (checkPid == 0 ? "TRUE" : "FALSE") << std::endl;
    return 0;
}
