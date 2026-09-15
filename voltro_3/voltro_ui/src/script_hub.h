#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Voltro {

// Represents an individual script item in the hub
struct ScriptItem {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::string hub;          // e.g. "robloxscripts.com", "scripthub.com", "rscripts.com", "scriptblox.com"
    std::string gameName;
    std::string category;     // Exploits, Admin, Utility, Combat, Visuals, Autofarm
    std::vector<std::string> tags;
    std::string code;
    std::string downloadUrl;
    int views = 0;
    bool isPatched = false;
    bool isUniversal = true;
    bool isFavorite = false;
};

// Security scan heuristics for exploit & script integrity
struct SecurityReport {
    std::string verdict; // CLEAN, NETWORK ACTIVITY, OBFUSCATED PAYLOAD, DANGEROUS PAYLOAD
    std::string verdictClass;
    std::vector<std::string> detectedFlags;
    bool containsDiscordWebhook = false;
    bool containsNetworkCall = false;
    bool isObfuscated = false;
};

// Represents a configurable script hub source/API provider
struct LegacyHubSource {
    std::string id;            // e.g. "all", "robloxscripts", "rscripts", "scripthub", "scriptblox"
    std::string displayName;   // e.g. "SELECT ALL", "RobloxScripts.com", "Rscripts.net", "ScriptHub.com"
    std::string domain;        // e.g. "robloxscripts.com"
    std::string apiEndpoint;   // e.g. "https://api.robloxscripts.com/v1/scripts"
    bool isEnabled = true;
};

class ScriptEngine {
public:
    static SecurityReport scanHeuristics(const std::string& code);
    static bool validateUrl(const std::string& url);
    static std::string fetchRemoteScript(const std::string& url);

    // Hub Providers Management (dynamic & customizable)
    static std::vector<LegacyHubSource>& getHubSources();
    static void addHubSource(const LegacyHubSource& source);
    static void removeHubSource(const std::string& id);

    // Rscripts.net Live API Configuration & Integration
    static void setRscriptsApiKey(const std::string& apiKey);
    static std::string getRscriptsApiKey();
    static std::vector<ScriptItem> queryRscriptsApi(const std::string& query, int limit = 15, int page = 1);

    // Fetch / search scripts across one or ALL hubs
    static std::vector<ScriptItem> queryScripts(const std::string& query, const std::string& hubId = "all");
};

} // namespace Voltro
