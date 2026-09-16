#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <windows.h>

namespace Voltro {


class VoltroEnvironment {
public:
    static VoltroEnvironment& getInstance() {
        static VoltroEnvironment instance;
        return instance;
    }

    
    void initializeEnvironment() {
        
        m_globals["identifyexecutor"] = [](const std::vector<std::string>& args) -> std::string {
            return "Voltro, v1.5.0";
        };
        m_globals["getexecutorname"] = [](const std::vector<std::string>& args) -> std::string {
            return "Voltro";
        };
        m_globals["checkcaller"] = [](const std::vector<std::string>& args) -> std::string {
            return "true";
        };

        
        m_globals["setclipboard"] = [this](const std::vector<std::string>& args) -> std::string {
            if (!args.empty()) {
                this->setClipboardText(args[0]);
            }
            return "true";
        };

        
        m_globals["writefile"] = [this](const std::vector<std::string>& args) -> std::string {
            if (args.size() >= 2) {
                m_virtualFs[args[0]] = args[1];
                return "true";
            }
            return "false";
        };

        m_globals["readfile"] = [this](const std::vector<std::string>& args) -> std::string {
            if (!args.empty() && m_virtualFs.count(args[0])) {
                return m_virtualFs[args[0]];
            }
            return "[Error: File not found]";
        };

        m_globals["isfile"] = [this](const std::vector<std::string>& args) -> std::string {
            return (!args.empty() && m_virtualFs.count(args[0])) ? "true" : "false";
        };

        m_globals["delfile"] = [this](const std::vector<std::string>& args) -> std::string {
            if (!args.empty()) {
                m_virtualFs.erase(args[0]);
                return "true";
            }
            return "false";
        };

        
        m_globals["setfpscap"] = [this](const std::vector<std::string>& args) -> std::string {
            if (!args.empty()) {
                try {
                    int fps = std::stoi(args[0]);
                    this->m_fpsCap = fps;
                    return std::to_string(fps);
                } catch (...) {}
            }
            return "60";
        };

        m_globals["getfpscap"] = [this](const std::vector<std::string>& args) -> std::string {
            return std::to_string(this->m_fpsCap);
        };
    }

    
    bool setClipboardText(const std::string& text) {
        if (!OpenClipboard(NULL)) return false;
        EmptyClipboard();
        HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, text.size() + 1);
        if (!hGlob) {
            CloseClipboard();
            return false;
        }
        memcpy(hGlob, text.c_str(), text.size() + 1);
        SetClipboardData(CF_TEXT, hGlob);
        CloseClipboard();
        return true;
    }

    bool hasGlobal(const std::string& name) const {
        return m_globals.count(name) > 0;
    }

    std::string invokeGlobal(const std::string& name, const std::vector<std::string>& args) {
        if (m_globals.count(name)) {
            return m_globals[name](args);
        }
        return "";
    }

    
    static std::string generateFpsBoostScript(int targetFps = 240) {
        return 
            "-- [[ Voltro Universal In-Game FPS Booster & Optimizer ]]\n"
            "local targetFPS = " + std::to_string(targetFps) + "\n"
            "if setfpscap then setfpscap(targetFPS) end\n"
            "local Lighting = game:GetService('Lighting')\n"
            "local Terrain = workspace:FindFirstChildOfClass('Terrain')\n"
            "if Terrain then\n"
            "    Terrain.WaterWaveSize = 0\n"
            "    Terrain.WaterWaveSpeed = 0\n"
            "    Terrain.WaterReflectance = 0\n"
            "end\n"
            "Lighting.GlobalShadows = false\n"
            "Lighting.FogEnd = 9e9\n"
            "Lighting.Brightness = 1\n"
            "for _, v in ipairs(game:GetDescendants()) do\n"
            "    if v:IsA('PostEffect') or v:IsA('BloomEffect') or v:IsA('ColorCorrectionEffect') or v:IsA('SunRaysEffect') or v:IsA('BlurEffect') then\n"
            "        v.Enabled = false\n"
            "    elseif v:IsA('ParticleEmitter') or v:IsA('Trail') or v:IsA('Smoke') or v:IsA('Fire') or v:IsA('Sparkles') then\n"
            "        v.Enabled = false\n"
            "    elseif v:IsA('Explosion') then\n"
            "        v.Visible = false\n"
            "    end\n"
            "end\n"
            "print('[VOLTRO FPS BOOST] Render optimizations applied. Target FPS: ' .. tostring(targetFPS))\n";
    }

private:
    VoltroEnvironment() : m_fpsCap(240) { initializeEnvironment(); }
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> m_globals;
    std::unordered_map<std::string, std::string> m_virtualFs;
    int m_fpsCap = 240;
};

} 
