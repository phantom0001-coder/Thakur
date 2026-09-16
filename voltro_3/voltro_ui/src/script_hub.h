#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Voltro {


struct ScriptItem {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::string hub;          
    std::string gameName;
    std::string category;     
    std::vector<std::string> tags;
    std::string code;
    std::string downloadUrl;
    int views = 0;
    bool isPatched = false;
    bool isUniversal = true;
    bool isFavorite = false;
};


struct SecurityReport {
    std::string verdict; 
    std::string verdictClass;
    std::vector<std::string> detectedFlags;
    bool containsDiscordWebhook = false;
    bool containsNetworkCall = false;
    bool isObfuscated = false;
};


struct LegacyHubSource {
    std::string id;            
    std::string displayName;   
    std::string domain;        
    std::string apiEndpoint;   
    bool isEnabled = true;
};

class ScriptEngine {
public:
    static SecurityReport scanHeuristics(const std::string& code);
    static bool validateUrl(const std::string& url);
    static std::string fetchRemoteScript(const std::string& url);

    
    static std::vector<LegacyHubSource>& getHubSources();
    static void addHubSource(const LegacyHubSource& source);
    static void removeHubSource(const std::string& id);

    
    static void setRscriptsApiKey(const std::string& apiKey);
    static std::string getRscriptsApiKey();
    static std::vector<ScriptItem> queryRscriptsApi(const std::string& query, int limit = 15, int page = 1);

    
    static std::vector<ScriptItem> queryScripts(const std::string& query, const std::string& hubId = "all");
};

} 
