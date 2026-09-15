// ============================================================
//  HubManager.cpp - Voltro Script Hub (Phase 3)
//  Full Qt6 multi-hub aggregator implementation.
// ============================================================

#include "HubManager.h"

#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

namespace Voltro {

// ============================================================
//  Singleton
// ============================================================
HubManager& HubManager::instance()
{
    static HubManager s_instance;
    return s_instance;
}

// ============================================================
//  Constructor
// ============================================================
HubManager::HubManager(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_nam->setTransferTimeout(15000); // 15 s hard timeout

    setupDefaultHubs();
    loadFavorites();
}

// ============================================================
//  Default hubs
// ============================================================
void HubManager::setupDefaultHubs()
{
    m_hubs = {
        { "rscripts",      "Rscripts.net",        "rscripts.net",
          "https://api.rscripts.net/v1",          true,  QColor("#00ff66") },
        { "scripthub",     "ScriptHub.net",        "scripthub.net",
          "https://scripthub.net/api/v2",         true,  QColor("#4fc3f7") },
        { "robloxscripts", "RobloxScripts.com",   "robloxscripts.com",
          "https://robloxscripts.com/api",        true,  QColor("#ff6b6b") },
        { "scriptblox",    "ScriptBlox.com",      "scriptblox.com",
          "https://scriptblox.com/api",           true,  QColor("#ce93d8") },
    };
}

// ============================================================
//  Hub management
// ============================================================
void HubManager::addHub(const HubSource& hub)
{
    for (auto& h : m_hubs) {
        if (h.id == hub.id) { h = hub; return; }
    }
    m_hubs.append(hub);
}

void HubManager::removeHub(const QString& id)
{
    m_hubs.removeIf([&](const HubSource& h){ return h.id == id; });
}

const QList<HubSource>& HubManager::hubs() const { return m_hubs; }

void HubManager::setHubEnabled(const QString& id, bool enabled)
{
    for (auto& h : m_hubs) {
        if (h.id == id) { h.enabled = enabled; return; }
    }
}

HubSource HubManager::hubById(const QString& id) const
{
    for (const auto& h : m_hubs)
        if (h.id == id) return h;
    return {};
}

const QList<ScriptMeta>& HubManager::cachedResults() const { return m_cache; }

// ============================================================
//  searchScripts
// ============================================================
void HubManager::searchScripts(const QString& query, const QString& hubId)
{
    if (query.trimmed().isEmpty()) return;

    m_currentQuery     = query.trimmed();
    m_currentHubFilter = hubId;
    m_pending.clear();
    m_activeFetches.store(0);

    emit searchStarted();

    if (hubId == QLatin1String("all")) {
        for (const auto& h : m_hubs) {
            if (!h.enabled) continue;
            if (h.id == QLatin1String("rscripts"))      { m_activeFetches++; queryRscripts(m_currentQuery); }
            else if (h.id == QLatin1String("scripthub"))     { m_activeFetches++; queryScripthub(m_currentQuery); }
            else if (h.id == QLatin1String("robloxscripts")) { m_activeFetches++; queryRobloxscripts(m_currentQuery); }
            else if (h.id == QLatin1String("scriptblox"))    { m_activeFetches++; queryScriptblox(m_currentQuery); }
        }
    } else {
        m_activeFetches.store(1);
        if      (hubId == QLatin1String("rscripts"))      queryRscripts(m_currentQuery);
        else if (hubId == QLatin1String("scripthub"))     queryScripthub(m_currentQuery);
        else if (hubId == QLatin1String("robloxscripts")) queryRobloxscripts(m_currentQuery);
        else if (hubId == QLatin1String("scriptblox"))    queryScriptblox(m_currentQuery);
        else {
            m_activeFetches.store(0);
            emit hubError(hubId, QStringLiteral("Unknown hub id: ") + hubId);
        }
    }

    if (m_activeFetches.load() == 0)
        emit resultsReady({});
}

// ============================================================
//  Network request builder
// ============================================================
QNetworkRequest HubManager::buildRequest(const QUrl& url) const
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("Voltro/3.0 (Qt6; Windows) ScriptHub/1.0"));
    req.setRawHeader("Accept", "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}

// ============================================================
//  queryRscripts  ->  GET https://api.rscripts.net/v1/search?q=...
// ============================================================
void HubManager::queryRscripts(const QString& q)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), q);
    query.addQueryItem(QStringLiteral("index"), QStringLiteral("scripts"));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("20"));

    QUrl url(QStringLiteral("https://api.rscripts.net/v1/search"));
    url.setQuery(query);

    auto* reply = m_nam->get(buildRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QString hubId = QStringLiteral("rscripts");

        if (reply->error() != QNetworkReply::NoError) {
            emit hubError(hubId, reply->errorString());
            onPartialResultsReady(hubId, {});
            return;
        }

        const auto doc  = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        const auto arr  = root.value(QStringLiteral("result")).toArray();

        QList<ScriptMeta> items;
        items.reserve(arr.size());
        for (const auto& val : arr) {
            auto meta = parseRscriptsItem(val.toObject());
            meta = runHeuristics(std::move(meta));
            meta.favorited = m_favorites.contains(meta.id);
            items.append(std::move(meta));
        }

        onPartialResultsReady(hubId, items);
    });
}

// ============================================================
//  queryScripthub  ->  GET https://scripthub.net/api/v2/scripts?search=...
// ============================================================
void HubManager::queryScripthub(const QString& q)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("search"), q);
    query.addQueryItem(QStringLiteral("limit"),  QStringLiteral("20"));

    QUrl url(QStringLiteral("https://scripthub.net/api/v2/scripts"));
    url.setQuery(query);

    auto* reply = m_nam->get(buildRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QString hubId = QStringLiteral("scripthub");

        if (reply->error() != QNetworkReply::NoError) {
            emit hubError(hubId, reply->errorString());
            onPartialResultsReady(hubId, {});
            return;
        }

        const auto doc = QJsonDocument::fromJson(reply->readAll());
        QList<ScriptMeta> items;

        // API may return array at root or { scripts: [...] }
        if (doc.isArray()) {
            const auto arr = doc.array();
            items.reserve(arr.size());
            for (const auto& val : arr) {
                auto meta = parseScripthubItem(val.toObject());
                meta = runHeuristics(std::move(meta));
                meta.favorited = m_favorites.contains(meta.id);
                items.append(std::move(meta));
            }
        } else {
            const auto arr = doc.object().value(QStringLiteral("scripts")).toArray();
            items.reserve(arr.size());
            for (const auto& val : arr) {
                auto meta = parseScripthubItem(val.toObject());
                meta = runHeuristics(std::move(meta));
                meta.favorited = m_favorites.contains(meta.id);
                items.append(std::move(meta));
            }
        }

        onPartialResultsReady(hubId, items);
    });
}

// ============================================================
//  queryRobloxscripts  ->  GET https://robloxscripts.com/api/scripts?q=...
// ============================================================
void HubManager::queryRobloxscripts(const QString& q)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), q);

    QUrl url(QStringLiteral("https://robloxscripts.com/api/scripts"));
    url.setQuery(query);

    auto* reply = m_nam->get(buildRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QString hubId = QStringLiteral("robloxscripts");

        if (reply->error() != QNetworkReply::NoError) {
            emit hubError(hubId, reply->errorString());
            onPartialResultsReady(hubId, {});
            return;
        }

        const auto doc = QJsonDocument::fromJson(reply->readAll());
        QList<ScriptMeta> items;

        // Typical response: { scripts: [...] } or root array
        auto tryArray = [&](const QJsonArray& arr) {
            for (const auto& val : arr) {
                auto meta = parseRobloxscriptsItem(val.toObject());
                meta = runHeuristics(std::move(meta));
                meta.favorited = m_favorites.contains(meta.id);
                items.append(std::move(meta));
            }
        };

        if (doc.isArray()) {
            tryArray(doc.array());
        } else {
            const auto obj = doc.object();
            for (const auto& key : { "scripts", "data", "result" }) {
                auto val = obj.value(QLatin1String(key));
                if (val.isArray()) { tryArray(val.toArray()); break; }
            }
        }

        onPartialResultsReady(hubId, items);
    });
}

// ============================================================
//  queryScriptblox  ->  GET https://scriptblox.com/api/script/search?q=...
// ============================================================
void HubManager::queryScriptblox(const QString& q)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"),   q);
    query.addQueryItem(QStringLiteral("max"), QStringLiteral("20"));

    QUrl url(QStringLiteral("https://scriptblox.com/api/script/search"));
    url.setQuery(query);

    auto* reply = m_nam->get(buildRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QString hubId = QStringLiteral("scriptblox");

        if (reply->error() != QNetworkReply::NoError) {
            emit hubError(hubId, reply->errorString());
            onPartialResultsReady(hubId, {});
            return;
        }

        const auto doc  = QJsonDocument::fromJson(reply->readAll());
        // { result: { scripts: [...] } }
        const auto arr  = doc.object()
                             .value(QStringLiteral("result")).toObject()
                             .value(QStringLiteral("scripts")).toArray();

        QList<ScriptMeta> items;
        items.reserve(arr.size());
        for (const auto& val : arr) {
            auto meta = parseScriptbloxItem(val.toObject());
            meta = runHeuristics(std::move(meta));
            meta.favorited = m_favorites.contains(meta.id);
            items.append(std::move(meta));
        }

        onPartialResultsReady(hubId, items);
    });
}

// ============================================================
//  Parsers
// ============================================================
ScriptMeta HubManager::parseRscriptsItem(const QJsonObject& obj)
{
    ScriptMeta m;
    m.hub         = QStringLiteral("rscripts");
    m.id          = QStringLiteral("rscripts-") + obj.value(QStringLiteral("_id")).toString();
    m.name        = obj.value(QStringLiteral("title")).toString();
    m.description = obj.value(QStringLiteral("description")).toString();
    m.author      = obj.value(QStringLiteral("owner")).toObject()
                          .value(QStringLiteral("username")).toString();
    m.gameName    = obj.value(QStringLiteral("game")).toObject()
                          .value(QStringLiteral("name")).toString();
    m.category    = obj.value(QStringLiteral("scriptType")).toString();
    m.views       = obj.value(QStringLiteral("views")).toInt();
    m.downloadUrl = obj.value(QStringLiteral("rawUrl")).toString();
    if (m.downloadUrl.isEmpty())
        m.downloadUrl = obj.value(QStringLiteral("scriptUrl")).toString();
    m.premium     = obj.value(QStringLiteral("premium")).toBool();
    m.isPatched   = obj.value(QStringLiteral("patched")).toBool();
    m.isUniversal = obj.value(QStringLiteral("isUniversal")).toBool(true);

    const auto tagsArr = obj.value(QStringLiteral("tags")).toArray();
    for (const auto& t : tagsArr)
        m.tags.append(t.toString());

    return m;
}

ScriptMeta HubManager::parseScripthubItem(const QJsonObject& obj)
{
    ScriptMeta m;
    m.hub         = QStringLiteral("scripthub");
    m.id          = QStringLiteral("scripthub-") + QString::number(obj.value(QStringLiteral("id")).toInt());
    m.name        = obj.value(QStringLiteral("name")).toString();
    m.description = obj.value(QStringLiteral("description")).toString();
    m.author      = obj.value(QStringLiteral("author")).toString();
    m.gameName    = obj.value(QStringLiteral("game")).toString();
    m.category    = obj.value(QStringLiteral("category")).toString();
    m.views       = obj.value(QStringLiteral("views")).toInt();
    m.downloadUrl = obj.value(QStringLiteral("script_url")).toString();
    if (m.downloadUrl.isEmpty())
        m.downloadUrl = obj.value(QStringLiteral("download_url")).toString();
    m.rating      = static_cast<float>(obj.value(QStringLiteral("rating")).toDouble());

    const auto tagsArr = obj.value(QStringLiteral("tags")).toArray();
    for (const auto& t : tagsArr)
        m.tags.append(t.toString());

    return m;
}

ScriptMeta HubManager::parseRobloxscriptsItem(const QJsonObject& obj)
{
    ScriptMeta m;
    m.hub         = QStringLiteral("robloxscripts");
    m.id          = QStringLiteral("robloxscripts-") + obj.value(QStringLiteral("id")).toString();
    if (m.id == QLatin1String("robloxscripts-"))
        m.id = QStringLiteral("robloxscripts-") + QString::number(obj.value(QStringLiteral("id")).toInt());
    m.name        = obj.value(QStringLiteral("title")).toString();
    if (m.name.isEmpty())
        m.name    = obj.value(QStringLiteral("name")).toString();
    m.description = obj.value(QStringLiteral("description")).toString();
    m.author      = obj.value(QStringLiteral("author")).toString();
    m.gameName    = obj.value(QStringLiteral("game")).toString();
    m.views       = obj.value(QStringLiteral("views")).toInt();
    m.downloadUrl = obj.value(QStringLiteral("script_url")).toString();
    if (m.downloadUrl.isEmpty())
        m.downloadUrl = obj.value(QStringLiteral("url")).toString();

    const auto tagsArr = obj.value(QStringLiteral("tags")).toArray();
    for (const auto& t : tagsArr)
        m.tags.append(t.toString());

    return m;
}

ScriptMeta HubManager::parseScriptbloxItem(const QJsonObject& obj)
{
    ScriptMeta m;
    m.hub         = QStringLiteral("scriptblox");
    m.id          = QStringLiteral("scriptblox-") + obj.value(QStringLiteral("_id")).toString();
    m.name        = obj.value(QStringLiteral("title")).toString();
    m.description = obj.value(QStringLiteral("description")).toString();
    m.author      = obj.value(QStringLiteral("owner")).toObject()
                          .value(QStringLiteral("username")).toString();
    m.gameName    = obj.value(QStringLiteral("game")).toObject()
                          .value(QStringLiteral("name")).toString();
    m.views       = obj.value(QStringLiteral("views")).toInt();
    m.premium     = obj.value(QStringLiteral("isPaid")).toBool();
    m.isPatched   = obj.value(QStringLiteral("patched")).toBool();
    m.isUniversal = obj.value(QStringLiteral("isUniversal")).toBool();
    m.downloadUrl = obj.value(QStringLiteral("rawScript")).toString();
    if (m.downloadUrl.isEmpty())
        m.downloadUrl = QStringLiteral("https://rawscripts.net/raw/")
                        + obj.value(QStringLiteral("slug")).toString();

    const auto tagsArr = obj.value(QStringLiteral("tags")).toArray();
    for (const auto& t : tagsArr)
        m.tags.append(t.toString());

    return m;
}

// ============================================================
//  runHeuristics - static-like safety scanner
// ============================================================
ScriptMeta HubManager::runHeuristics(ScriptMeta meta)
{
    const QString& code = meta.code;
    if (code.isEmpty()) {
        // No code yet - verdict deferred, stays Clean
        return meta;
    }

    // --- Danger patterns (red) ---
    static const QStringList dangerPatterns = {
        QStringLiteral("getrawmetatable"),
        QStringLiteral("setrawmetatable"),
        QStringLiteral("debug.getupvalue"),
        QStringLiteral("debug.setupvalue"),
        QStringLiteral("ffi.cdef"),
        QStringLiteral("ffi.load"),
        QStringLiteral("os.execute"),
        QStringLiteral("io.popen"),
        QStringLiteral("require(\"socket\")"),
        QStringLiteral("discord.com/api/webhooks"),
        QStringLiteral("discordapp.com/api/webhooks"),
    };

    // --- Caution patterns (yellow) ---
    static const QStringList cautionPatterns = {
        QStringLiteral("HttpGet"),
        QStringLiteral("HttpPost"),
        QStringLiteral("request({"),
        QStringLiteral("syn.request"),
        QStringLiteral("http.request"),
        QStringLiteral("game:HttpGet"),
        QStringLiteral("game:HttpPost"),
        QStringLiteral("loadstring"),
        QStringLiteral("pcall(loadstring"),
    };

    // --- Obfuscation indicators (purple) ---
    static const QStringList obfuscPatterns = {
        QStringLiteral("\\x"),
        QStringLiteral("\\u00"),
        QStringLiteral("string.byte"),
        QStringLiteral("string.char"),
        QStringLiteral("bit32.bxor"),
        QStringLiteral("local _={}"),
        // Extremely long single-line strings are a strong obfuscation hint
    };

    bool isDanger    = false;
    bool isCaution   = false;
    bool isObfuscated = false;

    for (const auto& pat : dangerPatterns) {
        if (code.contains(pat, Qt::CaseInsensitive)) { isDanger = true; break; }
    }

    if (!isDanger) {
        for (const auto& pat : cautionPatterns) {
            if (code.contains(pat, Qt::CaseSensitive)) { isCaution = true; break; }
        }
    }

    // Obfuscation: check patterns + line length heuristic
    int obfuscScore = 0;
    for (const auto& pat : obfuscPatterns) {
        if (code.contains(pat, Qt::CaseSensitive)) obfuscScore++;
    }
    // If any single line exceeds 1000 chars, flag as possibly obfuscated
    const QStringList lines = code.split(QLatin1Char('\n'));
    for (const auto& line : lines) {
        if (line.size() > 1000) { obfuscScore += 3; break; }
    }
    if (obfuscScore >= 3) isObfuscated = true;

    if (isDanger)
        meta.safetyVerdict = SafetyVerdict::Danger;
    else if (isObfuscated)
        meta.safetyVerdict = SafetyVerdict::Obfuscated;
    else if (isCaution)
        meta.safetyVerdict = SafetyVerdict::Caution;
    else
        meta.safetyVerdict = SafetyVerdict::Clean;

    return meta;
}

// ============================================================
//  fetchScriptContent
// ============================================================
void HubManager::fetchScriptContent(const QString& url, const QString& scriptId)
{
    if (url.isEmpty()) {
        emit contentReady(scriptId, {});
        return;
    }

    auto* reply = m_nam->get(buildRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, scriptId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[HubManager] fetchScriptContent error:" << reply->errorString();
            emit contentReady(scriptId, {});
            return;
        }

        const QString code = QString::fromUtf8(reply->readAll());

        // Update cache entry if present
        for (auto& cached : m_cache) {
            if (cached.id == scriptId) {
                cached.code = code;
                // Re-run heuristics now that we have code
                cached = runHeuristics(cached);
                break;
            }
        }

        emit contentReady(scriptId, code);
    });
}

// ============================================================
//  Favorites
// ============================================================
void HubManager::setFavorite(const QString& scriptId, bool fav)
{
    if (fav)
        m_favorites.insert(scriptId);
    else
        m_favorites.remove(scriptId);

    // Keep cache in sync
    for (auto& m : m_cache)
        if (m.id == scriptId) m.favorited = fav;
    for (auto& m : m_pending)
        if (m.id == scriptId) m.favorited = fav;

    saveFavorites();
}

bool HubManager::isFavorite(const QString& scriptId) const
{
    return m_favorites.contains(scriptId);
}

QList<ScriptMeta> HubManager::favorites() const
{
    QList<ScriptMeta> result;
    for (const auto& m : m_cache)
        if (m_favorites.contains(m.id)) result.append(m);
    return result;
}

void HubManager::loadFavorites()
{
    QSettings s;
    const QStringList ids = s.value(QLatin1String(kFavKey)).toStringList();
    m_favorites = QSet<QString>(ids.begin(), ids.end());
}

void HubManager::saveFavorites()
{
    QSettings s;
    s.setValue(QLatin1String(kFavKey), QStringList(m_favorites.begin(), m_favorites.end()));
}

// ============================================================
//  Internal aggregation helpers
// ============================================================
void HubManager::onPartialResultsReady(const QString& hubId,
                                        const QList<ScriptMeta>& partial)
{
    Q_UNUSED(hubId)
    m_pending.append(partial);

    if (--m_activeFetches <= 0)
        mergeAndEmit();
}

void HubManager::mergeAndEmit()
{
    // Sort by views descending
    std::sort(m_pending.begin(), m_pending.end(),
              [](const ScriptMeta& a, const ScriptMeta& b) {
                  return a.views > b.views;
              });

    m_cache = std::move(m_pending);
    m_pending.clear();

    emit resultsReady(m_cache);
}

} // namespace Voltro
