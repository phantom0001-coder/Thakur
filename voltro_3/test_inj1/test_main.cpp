




#include "Inj1Injector_standalone.h"
#include <iostream>
#include <string>

int main()
{
    std::cout << "=== Inj1Injector Standalone Test ===" << std::endl;
    std::cout << "Testing injection into Roblox..." << std::endl;
    
    
    DWORD robloxPid = Inj1Injector::GetProcessPidByName("RobloxPlayerBeta.exe");
    
    if (robloxPid == 0)
    {
        std::cout << "[!] Roblox process not found! Make sure Roblox is running." << std::endl;
        std::cout << "    Process name searched: RobloxPlayerBeta.exe" << std::endl;
        
        
        robloxPid = Inj1Injector::GetProcessPidByName("Roblox.exe");
        if (robloxPid == 0)
        {
            std::cout << "[!] Also tried: Roblox.exe - Not found." << std::endl;
            std::cout << "    Please start Roblox and try again." << std::endl;
            return 1;
        }
        else
        {
            std::cout << "[+] Found Roblox.exe with PID: " << robloxPid << std::endl;
        }
    }
    else
    {
        std::cout << "[+] Found RobloxPlayerBeta.exe with PID: " << robloxPid << std::endl;
    }
    
    
    std::string dllPath;
    std::cout << "\n[?] Enter path to DLL to inject: ";
    std::getline(std::cin, dllPath);
    
    if (dllPath.empty())
    {
        std::cout << "[!] No DLL path provided!" << std::endl;
        return 1;
    }
    
    
    if (!dllPath.empty() && dllPath.front() == '"' && dllPath.back() == '"')
    {
        dllPath = dllPath.substr(1, dllPath.size() - 2);
    }
    
    std::cout << "[+] Attempting to inject: " << dllPath << std::endl;
    std::cout << "[+] Target PID: " << robloxPid << std::endl;
    
    
    auto* injector = Inj1Injector::Get();
    bool success = injector->Inject(robloxPid, dllPath);
    
    if (success)
    {
        std::cout << "\n[+] SUCCESS! DLL injected into Roblox!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "\n[!] FAILED to inject DLL!" << std::endl;
        std::cout << "    Make sure:" << std::endl;
        std::cout << "    - Roblox is running" << std::endl;
        std::cout << "    - DLL path is valid" << std::endl;
        std::cout << "    - You have administrator privileges" << std::endl;
        return 1;
    }
}
