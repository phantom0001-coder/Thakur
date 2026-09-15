

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <wininet.h>
#include <regex>
#include "script_hub.h"

namespace Thakur {

static std::vector<HubSource> s_hubSources = {
    {"all", "⭐ SELECT ALL HUBS", "*", "internal://all"},
    {"robloxscripts", "RobloxScripts.com", "robloxscripts.com", "https://robloxscripts.com"},
    {"scripthub", "ScriptHub.com", "scripthub.com", "https://scripthub.com"},
    {"rscripts", "Rscripts.net", "rscripts.net", "https://rscripts.net"},
    {"scriptblox", "ScriptBlox.com", "scriptblox.com", "https://scriptblox.com"}
};

std::vector<HubSource>& ScriptEngine::getHubSources() {
    return s_hubSources;
}

void ScriptEngine::addHubSource(const HubSource& source) {
    s_hubSources.push_back(source);
}

void ScriptEngine::removeHubSource(const std::string& id) {
    s_hubSources.erase(
        std::remove_if(s_hubSources.begin(), s_hubSources.end(),
            [&id](const HubSource& h) { return h.id == id && h.id != "all"; }),
        s_hubSources.end()
    );
}

static std::string s_rscriptsApiKey = "";

void ScriptEngine::setRscriptsApiKey(const std::string& apiKey) {
    s_rscriptsApiKey = apiKey;
}

std::string ScriptEngine::getRscriptsApiKey() {
    return s_rscriptsApiKey;
}

std::vector<ScriptItem> ScriptEngine::queryRscriptsApi(const std::string& query, int limit, int page) {
    std::vector<ScriptItem> results;
    if (query.empty()) return results;

    std::string encodedQuery = "";
    for (char c : query) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedQuery += c;
        } else if (c == ' ') {
            encodedQuery += "%20";
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
            encodedQuery += buf;
        }
    }

    std::string endpoint = "/v1/search?q=" + encodedQuery + "&index=scripts&limit=" + std::to_string(limit) + "&page=" + std::to_string(page);

    HINTERNET hInternet = InternetOpenA("Thakur/1.5.0 (Windows NT 10.0; Win64; x64) NativeEngine", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return results;

    HINTERNET hConnect = InternetConnectA(hInternet, "api.rscripts.net", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return results;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", endpoint.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return results;
    }

    
    std::string headers = "Accept: application/json\r\n";
    if (!s_rscriptsApiKey.empty()) {
        headers += "Authorization: Bearer " + s_rscriptsApiKey + "\r\n";
    }

    BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), NULL, 0);
    if (!sent) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return results;
    }

    std::string responseBody;
    char buffer[16384];
    DWORD bytesRead = 0;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseBody.append(buffer, bytesRead);
        if (responseBody.size() > 4 * 1024 * 1024) break; 
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (responseBody.empty() || responseBody.find("\"success\":true") == std::string::npos) {
        return results;
    }

    
    std::regex scriptItemRegex("\\{\\s*\"id\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"title\"\\s*:\\s*\"([^\"]+)\"[^}]*?\"views\"\\s*:\\s*([0-9]+)[^}]*?\"rawScript\"\\s*:\\s*\"([^\"]+)\"");
    std::string searchBuf = responseBody;
    std::smatch match;

    while (std::regex_search(searchBuf, match, scriptItemRegex)) {
        ScriptItem item;
        item.id = match[1].str();
        item.name = match[2].str();
        item.hub = "rscripts.net";
        item.gameName = "Universal";
        item.category = "Exploits / Utility";
        try { item.views = std::stoi(match[3].str()); } catch (...) { item.views = 0; }
        item.downloadUrl = match[4].str();
        item.code = "loadstring(game:HttpGet('" + item.downloadUrl + "'))()";
        item.isUniversal = true;
        item.isPatched = false;
        item.isFavorite = false;
        results.push_back(item);

        searchBuf = match.suffix().str();
    }

    return results;
}

static const std::vector<ScriptItem> s_mockScriptDatabase = {
    
    {
        "rs_1", "Infinite Yield v5.9 (Universal Admin)",
        "Top-rated universal admin commands script suite with 300+ commands.",
        "EdgeIY", "robloxscripts.com", "Universal", "Exploits / Admin",
        {"admin", "universal", "fly", "noclip", "speed"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        84210, false, true, true
    },
    {
        "rs_2", "Blox Fruits Auto-Farm & ESP",
        "Fast chest collection, mob auto-farm, mastery, and Devil Fruit sniper.",
        "XenonDev", "robloxscripts.com", "Blox Fruits", "Autofarm",
        {"bloxfruits", "autofarm", "esp", "teleport"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        51200, false, false, false
    },
    
    {
        "sh_1", "Dex Explorer v4 (High-Level Memory)",
        "Ultimate game explorer, remote spy, and property inspector for Luau.",
        "Moon", "scripthub.com", "Universal", "Utility",
        {"dex", "explorer", "debug", "remotespy"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        99400, false, true, true
    },
    {
        "sh_2", "Universal Aimbot & ESP Box (Wallhack)",
        "Silent aim, FOV circle customizer, chams, and distance ESP for all FPS games.",
        "ViperRBLX", "scripthub.com", "Universal FPS", "Combat / Visuals",
        {"aimbot", "esp", "wallhack", "chams"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        63200, false, true, false
    },
    
    {
        "rc_1", "SimpleSpy v2.2 (Remote Event Logger)",
        "Inter-client remote event & remote function logger with script generation.",
        "exxtremestuffs", "rscripts.com", "Universal", "Utility",
        {"remotespy", "network", "logger", "security"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        41800, false, true, false
    },
    {
        "rc_2", "Arsenal Kill-All & Silent Aim",
        "Gun mods, infinite ammo, speed bypass, and auto-headshot for Arsenal.",
        "DarkHub", "rscripts.com", "Arsenal", "Exploits / Combat",
        {"arsenal", "killall", "silentaim", "gunmods"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        38900, false, false, false
    },
    
    {
        "sb_1", "Fly & Noclip GUI v3",
        "Toggleable smooth flight, bypass ground collision, anti-fall damage.",
        "GhostExec", "scriptblox.com", "Universal", "Exploits",
        {"fly", "noclip", "bypass", "movement"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        33500, false, true, false
    },
    {
        "sb_2", "Pet Simulator 99 Infinite Coins & Dupe",
        "Auto tap, gem collector, egg opener with speed bypass.",
        "ZapHub", "scriptblox.com", "Pet Simulator 99", "Autofarm",
        {"petsim99", "autofarm", "gems", "coins"},
        "loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",
        "https://example.com/script.lua",
        47100, false, false, false
    }
};

std::vector<ScriptItem> ScriptEngine::queryScripts(const std::string& query, const std::string& hubId) {
    std::vector<ScriptItem> results;

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    
    std::string targetDomain = "";
    if (hubId != "all" && !hubId.empty()) {
        for (const auto& h : s_hubSources) {
            if (h.id == hubId || h.domain == hubId) {
                targetDomain = h.domain;
                break;
            }
        }
    }

    for (const auto& item : s_mockScriptDatabase) {
        
        if (!targetDomain.empty() && targetDomain != "*" && item.hub != targetDomain) {
            continue;
        }

        
        if (!lowerQuery.empty()) {
            std::string nameLower = item.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            std::string descLower = item.description;
            std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);

            std::string gameLower = item.gameName;
            std::transform(gameLower.begin(), gameLower.end(), gameLower.begin(), ::tolower);

            bool matchesTag = false;
            for (const auto& tag : item.tags) {
                std::string tagLower = tag;
                std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
                if (tagLower.find(lowerQuery) != std::string::npos) {
                    matchesTag = true;
                    break;
                }
            }

            if (nameLower.find(lowerQuery) == std::string::npos &&
                descLower.find(lowerQuery) == std::string::npos &&
                gameLower.find(lowerQuery) == std::string::npos &&
                !matchesTag) {
                continue;
            }
        }

        results.push_back(item);
    }

    
    if ((targetDomain == "*" || targetDomain == "rscripts.net" || hubId == "rscripts") && !query.empty()) {
        std::vector<ScriptItem> liveResults = queryRscriptsApi(query, 12, 1);
        for (const auto& liveItem : liveResults) {
            results.push_back(liveItem);
        }
    }

    return results;
}

SecurityReport ScriptEngine::scanHeuristics(const std::string& code) {
    SecurityReport report;
    report.verdict = "CLEAN";
    report.verdictClass = "clean";

    
    std::regex webhookRegex(R"(https?:\/\/(?:canary\.|ptb\.)?discord(?:app)?\.com\/api\/webhooks\/\d+\/[A-Za-z0-9_-]+)", std::regex::icase);
    if (std::regex_search(code, webhookRegex)) {
        report.containsDiscordWebhook = true;
        report.detectedFlags.push_back("Discord Webhook detected (Potential data exfiltration endpoint)");
    }

    
    std::regex netRegex(R"((?:game(?:\.|:GetService\("HttpService"\)|:HttpGet|:HttpPost)|syn\.request|http\.request|request\s*\(|httpget\s*\())", std::regex::icase);
    if (std::regex_search(code, netRegex)) {
        report.containsNetworkCall = true;
        report.detectedFlags.push_back("External network requests detected (HttpGet/request remote calls)");
    }

    
    std::regex hexRegex(R"(\\(?:x[0-9a-fA-F]{2}|[0-9]{2,3}))");
    auto hexBegin = std::sregex_iterator(code.begin(), code.end(), hexRegex);
    auto hexEnd = std::sregex_iterator();
    ptrdiff_t hexCount = std::distance(hexBegin, hexEnd);
    if (hexCount > 8) {
        report.isObfuscated = true;
        report.detectedFlags.push_back("Bytecode packing / hex escape sequence obfuscation detected");
    }

    if (report.containsDiscordWebhook) {
        report.verdict = "DANGEROUS PAYLOAD";
        report.verdictClass = "danger";
    } else if (report.isObfuscated) {
        report.verdict = "OBFUSCATED PAYLOAD";
        report.verdictClass = "obf";
    } else if (report.containsNetworkCall) {
        report.verdict = "NETWORK ACTIVITY";
        report.verdictClass = "caution";
    } else {
        report.detectedFlags.push_back("Payload passed heuristic integrity checks.");
    }

    return report;
}

bool ScriptEngine::validateUrl(const std::string& url) {
    const std::vector<std::string> whitelist = {
        "raw.githubusercontent.com",
        "pastebin.com",
        "hastebin.com",
        "gist.githubusercontent.com",
        "robloxscripts.com",
        "scripthub.com",
        "rscripts.net",
        "scriptblox.com"
    };

    for (const auto& domain : whitelist) {
        if (url.find(domain) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string ScriptEngine::fetchRemoteScript(const std::string& url) {
    if (url.empty()) return "";

    HINTERNET hInternet = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36 Thakur/1.5.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        return "-- [ERROR] Failed to initialize internet session";
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (url.find("https://") == 0) {
        flags |= INTERNET_FLAG_SECURE;
    }

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, flags, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return "-- [ERROR] Failed to connect to remote script URL: " + url;
    }

    std::string content;
    char buffer[65536];
    DWORD bytesRead = 0;
    
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        content.append(buffer, bytesRead);
        if (content.size() > 64 * 1024 * 1024) {
            break; 
        }
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (content.empty()) {
        return "-- [WARNING] Empty response received from: " + url;
    }
    return content;
}

} 
