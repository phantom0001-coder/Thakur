

namespace Thakur {

struct HubSource {
    QString id;           
    QString displayName;  
    QString domain;       
    QString apiUrl;       
    bool    enabled = true;
    QColor  iconColor;    

    bool operator==(const HubSource& o) const { return id == o.id; }
};

enum class SafetyVerdict {
    Clean,
    Caution,
    Danger,
    Obfuscated
};

struct ScriptMeta {
    QString       id;
    QString       name;
    QString       description;
    QString       author;
    QString       category;
    QString       hub;          
    QString       gameName;
    QString       downloadUrl;
    QString       code;         
    int           views    = 0;
    float         rating   = 0.0f;
    bool          premium  = false;
    bool          favorited = false;
    bool          isPatched = false;
    bool          isUniversal = true;
    SafetyVerdict safetyVerdict = SafetyVerdict::Clean;
    QStringList   tags;
};

class HubManager : public QObject
{
    Q_OBJECT

public:
    
    static HubManager& instance();

    
    HubManager(const HubManager&)            = delete;
    HubManager& operator=(const HubManager&) = delete;

    
    void                    addHub(const HubSource& hub);
    void                    removeHub(const QString& id);
    const QList<HubSource>& hubs() const;
    void                    setHubEnabled(const QString& id, bool enabled);
    HubSource               hubById(const QString& id) const;

    
    
    
    void searchScripts(const QString& query, const QString& hubId = QStringLiteral("all"));

    
    
    
    void fetchScriptContent(const QString& url, const QString& scriptId);

    
    void              setFavorite(const QString& scriptId, bool fav);
    QList<ScriptMeta> favorites() const;
    bool              isFavorite(const QString& scriptId) const;

    
    const QList<ScriptMeta>& cachedResults() const;

signals:
    void searchStarted();
    void resultsReady(const QList<ScriptMeta>& results);
    void contentReady(const QString& scriptId, const QString& code);
    void hubError(const QString& hubId, const QString& error);

private:
    explicit HubManager(QObject* parent = nullptr);
    ~HubManager() override = default;

    void setupDefaultHubs();
    void loadFavorites();
    void saveFavorites();

    
    void queryRscripts(const QString& q);
    void queryScripthub(const QString& q);
    void queryRobloxscripts(const QString& q);
    void queryScriptblox(const QString& q);

    
    ScriptMeta parseRscriptsItem(const QJsonObject& obj);
    ScriptMeta parseScripthubItem(const QJsonObject& obj);
    ScriptMeta parseRobloxscriptsItem(const QJsonObject& obj);
    ScriptMeta parseScriptbloxItem(const QJsonObject& obj);

    
    ScriptMeta runHeuristics(ScriptMeta meta);

    
    QNetworkRequest buildRequest(const QUrl& url) const;
    void            onPartialResultsReady(const QString& hubId,
                                          const QList<ScriptMeta>& partial);
    void            checkAllDone();
    void            mergeAndEmit();

    
    QNetworkAccessManager* m_nam            = nullptr;
    QList<HubSource>       m_hubs;
    QList<ScriptMeta>      m_cache;      
    QSet<QString>          m_favorites;  

    
    QList<ScriptMeta>      m_pending;         
    std::atomic<int>       m_activeFetches{0};
    QString                m_currentQuery;
    QString                m_currentHubFilter;

    static constexpr const char* kFavKey = "hub/favorites";
};

} 
