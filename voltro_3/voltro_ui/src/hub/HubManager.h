#pragma once

// ============================================================
//  HubManager.h - Voltro Script Hub (Phase 3)
//  Qt6 QNetworkAccessManager-based multi-hub aggregator.
//  Replaces the WinInet implementation in script_hub.cpp.
// ============================================================

#include <QObject>
#include <QColor>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <atomic>

namespace Voltro {

// ---------------------------------------------------------------
//  HubSource - describes a single remote script hub provider
// ---------------------------------------------------------------
struct HubSource {
    QString id;           // "rscripts" | "scripthub" | "robloxscripts" | "scriptblox"
    QString displayName;  // Human-readable label shown in UI
    QString domain;       // e.g. "rscripts.net"
    QString apiUrl;       // Full base API URL
    bool    enabled = true;
    QColor  iconColor;    // Accent colour for hub badge

    bool operator==(const HubSource& o) const { return id == o.id; }
};

// ---------------------------------------------------------------
//  SafetyVerdict - result of runHeuristics()
// ---------------------------------------------------------------
enum class SafetyVerdict {
    Clean,
    Caution,
    Danger,
    Obfuscated
};

// ---------------------------------------------------------------
//  ScriptMeta - Qt-idiomatic replacement for ScriptItem
// ---------------------------------------------------------------
struct ScriptMeta {
    QString       id;
    QString       name;
    QString       description;
    QString       author;
    QString       category;
    QString       hub;          // HubSource::id that produced this result
    QString       gameName;
    QString       downloadUrl;
    QString       code;         // populated lazily via fetchScriptContent()
    int           views    = 0;
    float         rating   = 0.0f;
    bool          premium  = false;
    bool          favorited = false;
    bool          isPatched = false;
    bool          isUniversal = true;
    SafetyVerdict safetyVerdict = SafetyVerdict::Clean;
    QStringList   tags;
};

// ---------------------------------------------------------------
//  HubManager - singleton, Qt6 QNAM-based
// ---------------------------------------------------------------
class HubManager : public QObject
{
    Q_OBJECT

public:
    // Singleton accessor
    static HubManager& instance();

    // Disable copy / move
    HubManager(const HubManager&)            = delete;
    HubManager& operator=(const HubManager&) = delete;

    // -- Hub management --------------------------------------------------
    void                    addHub(const HubSource& hub);
    void                    removeHub(const QString& id);
    const QList<HubSource>& hubs() const;
    void                    setHubEnabled(const QString& id, bool enabled);
    HubSource               hubById(const QString& id) const;

    // -- Async search ----------------------------------------------------
    //  hubId == "all"  -> query every enabled hub in parallel
    //  hubId == <id>   -> query only that hub
    void searchScripts(const QString& query, const QString& hubId = QStringLiteral("all"));

    // -- Async content fetch ---------------------------------------------
    //  Downloads raw script source from downloadUrl.
    //  Emits contentReady(scriptId, code) when done.
    void fetchScriptContent(const QString& url, const QString& scriptId);

    // -- Favorites (persisted via QSettings) -----------------------------
    void              setFavorite(const QString& scriptId, bool fav);
    QList<ScriptMeta> favorites() const;
    bool              isFavorite(const QString& scriptId) const;

    // -- Cached results from last search ---------------------------------
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

    // -- Per-hub query helpers -------------------------------------------
    void queryRscripts(const QString& q);
    void queryScripthub(const QString& q);
    void queryRobloxscripts(const QString& q);
    void queryScriptblox(const QString& q);

    // -- Parsers ---------------------------------------------------------
    ScriptMeta parseRscriptsItem(const QJsonObject& obj);
    ScriptMeta parseScripthubItem(const QJsonObject& obj);
    ScriptMeta parseRobloxscriptsItem(const QJsonObject& obj);
    ScriptMeta parseScriptbloxItem(const QJsonObject& obj);

    // -- Heuristic safety scanner ----------------------------------------
    ScriptMeta runHeuristics(ScriptMeta meta);

    // -- Internal helpers ------------------------------------------------
    QNetworkRequest buildRequest(const QUrl& url) const;
    void            onPartialResultsReady(const QString& hubId,
                                          const QList<ScriptMeta>& partial);
    void            checkAllDone();
    void            mergeAndEmit();

    // -- State -----------------------------------------------------------
    QNetworkAccessManager* m_nam            = nullptr;
    QList<HubSource>       m_hubs;
    QList<ScriptMeta>      m_cache;      // merged results from last search
    QSet<QString>          m_favorites;  // set of favorited script IDs

    // Search-in-progress bookkeeping
    QList<ScriptMeta>      m_pending;         // accumulates partial results
    std::atomic<int>       m_activeFetches{0};
    QString                m_currentQuery;
    QString                m_currentHubFilter;

    static constexpr const char* kFavKey = "hub/favorites";
};

} // namespace Voltro
