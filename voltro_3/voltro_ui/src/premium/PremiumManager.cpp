// ============================================================
//  PremiumManager.cpp  -  Voltro Executor  (Phase 5)
// ============================================================

#include "PremiumManager.h"

// Qt headers
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDebug>

// Win32 headers (order matters)
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// ----------------------------------------------------------------
// Internal constants
// ----------------------------------------------------------------
namespace {

// QSettings organisation / application identifiers
constexpr const char* kOrg          = "Voltro";
constexpr const char* kApp          = "VoltroExecutor";

// Settings keys
constexpr const char* kKeyStored    = "premium/key";
constexpr const char* kKeyTier      = "premium/tier";
constexpr const char* kKeyExpiry    = "premium/expiry";
constexpr const char* kKeyLastOnline= "premium/lastOnline";

// Network endpoints
constexpr const char* kValidateUrl  = "https://api.voltro.gg/v1/validate";

// The built-in test / trial key
constexpr const char* kTestKey      = "373438";

// Offline grace period in days
constexpr int kGracePeriodDays      = 7;

} // anonymous namespace

// ================================================================
// Singleton
// ================================================================

namespace Voltro {

PremiumManager& PremiumManager::instance()
{
    static PremiumManager s_instance;
    return s_instance;
}

// ----------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------
PremiumManager::PremiumManager(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    // Register PlanInfo for queued connections
    qRegisterMetaType<Voltro::PlanInfo>("Voltro::PlanInfo");

    // Build HWID once and cache it
    m_hwid = generateHWID();

    // Start with a free-tier plan
    m_plan = makePlanInfo(PlanTier::Free);
}

// ================================================================
// HWID generation
// ================================================================

QString PremiumManager::generateHWID() const
{
    // 1. C-drive volume serial number
    DWORD volumeSerial = 0;
    ::GetVolumeInformationW(
        L"C:\\",
        nullptr, 0,          // volume name  (not needed)
        &volumeSerial,
        nullptr,             // max component length (not needed)
        nullptr,             // file system flags   (not needed)
        nullptr, 0           // file system name    (not needed)
    );

    // 2. First non-loopback MAC address via GetAdaptersInfo
    QString macString;
    ULONG   bufLen = 15 * 1024; // start with 15 KiB, typical for most machines
    QByteArray adapterBuf(static_cast<int>(bufLen), '\0');

    DWORD ret = ::GetAdaptersInfo(
        reinterpret_cast<PIP_ADAPTER_INFO>(adapterBuf.data()),
        &bufLen);

    if (ret == ERROR_BUFFER_OVERFLOW) {
        adapterBuf.resize(static_cast<int>(bufLen));
        ret = ::GetAdaptersInfo(
            reinterpret_cast<PIP_ADAPTER_INFO>(adapterBuf.data()),
            &bufLen);
    }

    if (ret == NO_ERROR) {
        const PIP_ADAPTER_INFO pAdapter =
            reinterpret_cast<PIP_ADAPTER_INFO>(adapterBuf.data());

        for (const IP_ADAPTER_INFO* a = pAdapter; a != nullptr; a = a->Next) {
            // Skip loopback adapters (MAC = all zeros)
            bool allZero = true;
            for (UINT i = 0; i < a->AddressLength; ++i) {
                if (a->Address[i] != 0) { allZero = false; break; }
            }
            if (allZero) continue;

            // Format MAC as XX:XX:XX:XX:XX:XX
            QByteArray macBytes;
            for (UINT i = 0; i < a->AddressLength && i < 6; ++i) {
                if (i) macBytes.append(':');
                macBytes.append(
                    QString::asprintf("%02X", a->Address[i]).toLatin1());
            }
            macString = QString::fromLatin1(macBytes);
            break;
        }
    }

    // 3. Combine and SHA-256 hash
    const QString combined = QString::number(volumeSerial, 16).toUpper()
                           + QStringLiteral("|")
                           + macString;

    const QByteArray hash =
        QCryptographicHash::hash(
            combined.toUtf8(),
            QCryptographicHash::Sha256
        ).toHex().toUpper();

    // 4. Format as HWID-XXXX-XXXX-XXXX-XXXX  (first 16 hex chars)
    const QString h = QString::fromLatin1(hash.left(16));
    return QStringLiteral("HWID-")
           + h.mid(0, 4) + QStringLiteral("-")
           + h.mid(4, 4) + QStringLiteral("-")
           + h.mid(8, 4) + QStringLiteral("-")
           + h.mid(12, 4);
}



// NOTE: The above decryptLocal is overridden by a cleaner inline
//       approach.  Implemented properly below by re-using encryptLocal
//       on the raw cipher QByteArray converted through a helper:

namespace {
    QByteArray xorBytes(const QByteArray& data, const QByteArray& keyBytes)
    {
        if (keyBytes.isEmpty()) return data;
        QByteArray out(data.size(), '\0');
        for (int i = 0; i < data.size(); ++i) {
            out[i] = static_cast<char>(
                static_cast<unsigned char>(data[i]) ^
                static_cast<unsigned char>(keyBytes[i % keyBytes.size()]));
        }
        return out;
    }
} // namespace

// ================================================================
// Static plan presets
// ================================================================

/*static*/
PlanInfo PremiumManager::makePlanInfo(PlanTier tier)
{
    PlanInfo p;
    p.tier = tier;

    switch (tier) {
    case PlanTier::Free:
        p.name         = QStringLiteral("Free Trial");
        p.priceRobux   = 0;
        p.durationDays = 7;
        p.features     = {
            QStringLiteral("Script execution (basic)"),
            QStringLiteral("7-day trial access"),
            QStringLiteral("Community support")
        };
        break;

    case PlanTier::Monthly30Day:
        p.name         = QStringLiteral("Monthly");
        p.priceRobux   = 299;
        p.durationDays = 30;
        p.features     = {
            QStringLiteral("Full script execution"),
            QStringLiteral("30-day access"),
            QStringLiteral("Script hub access"),
            QStringLiteral("Priority support")
        };
        break;

    case PlanTier::Quarterly90Day:
        p.name         = QStringLiteral("Quarterly");
        p.priceRobux   = 699;
        p.durationDays = 90;
        p.features     = {
            QStringLiteral("Full script execution"),
            QStringLiteral("90-day access"),
            QStringLiteral("Script hub access"),
            QStringLiteral("Priority support"),
            QStringLiteral("Early feature access")
        };
        break;

    case PlanTier::Annual365Day:
        p.name         = QStringLiteral("Annual");
        p.priceRobux   = 1499;
        p.durationDays = 365;
        p.features     = {
            QStringLiteral("Full script execution"),
            QStringLiteral("365-day access"),
            QStringLiteral("Script hub access"),
            QStringLiteral("Priority support"),
            QStringLiteral("Early feature access"),
            QStringLiteral("Exclusive premium scripts"),
            QStringLiteral("Discord role")
        };
        break;
    }

    return p;
}

// ================================================================
// Storage helpers
// ================================================================

void PremiumManager::saveToStorage(const QString& key, const PlanInfo& plan)
{
    QSettings s((QLatin1String(kOrg)), (QLatin1String(kApp)));

    // Encrypt the key and store as base64
    const QByteArray encKey = xorBytes(key.toUtf8(), m_hwid.toUtf8());
    s.setValue(QLatin1String(kKeyStored),     encKey.toBase64());
    s.setValue(QLatin1String(kKeyTier),       static_cast<int>(plan.tier));
    s.setValue(QLatin1String(kKeyExpiry),     plan.expiresAt.toString(Qt::ISODate));
    s.setValue(QLatin1String(kKeyLastOnline), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    s.sync();
}

bool PremiumManager::checkLocalKey()
{
    QSettings s((QLatin1String(kOrg)), (QLatin1String(kApp)));

    if (!s.contains(QLatin1String(kKeyStored))) return false;

    // Decrypt stored key
    const QByteArray enc =
        QByteArray::fromBase64(s.value(QLatin1String(kKeyStored)).toByteArray());
    const QByteArray decBytes = xorBytes(enc, m_hwid.toUtf8());
    const QString storedKey   = QString::fromUtf8(decBytes);

    if (storedKey.isEmpty()) return false;

    // Check expiry
    const QString expiryStr = s.value(QLatin1String(kKeyExpiry)).toString();
    const QDateTime expiry  = QDateTime::fromString(expiryStr, Qt::ISODate);

    if (!expiry.isValid()) return false;

    const QDateTime now = QDateTime::currentDateTimeUtc();

    // Hard-expired - check grace period
    if (now > expiry) {
        const QString lastOnlineStr =
            s.value(QLatin1String(kKeyLastOnline)).toString();
        const QDateTime lastOnline =
            QDateTime::fromString(lastOnlineStr, Qt::ISODate);

        if (!lastOnline.isValid()) return false;

        // Grace period: allow kGracePeriodDays days after last online check
        const qint64 daysSinceOnline = lastOnline.daysTo(now);
        if (daysSinceOnline > kGracePeriodDays) {
            qInfo() << "[PremiumManager] Key expired and grace period elapsed.";
            emit keyExpired();
            return false;
        }
        qInfo() << "[PremiumManager] Offline grace period active:"
                << (kGracePeriodDays - daysSinceOnline) << "days left.";
    }

    // Restore plan
    int tierInt = s.value(QString::fromLatin1(kKeyTier), QVariant(0)).toInt();
    const PlanTier tier = static_cast<PlanTier>(
        qBound(0, tierInt, static_cast<int>(PlanTier::Annual365Day)));

    m_plan           = makePlanInfo(tier);
    m_plan.expiresAt = expiry;
    m_plan.isActive  = true;
    m_activeKey      = storedKey;

    return true;
}

// ================================================================
// loadFromStorage  -  called on startup
// ================================================================

void PremiumManager::loadFromStorage()
{
    if (checkLocalKey()) {
        qInfo() << "[PremiumManager] Session restored from storage. Tier:"
                << static_cast<int>(m_plan.tier);
        emit keyValidated(true, m_plan);
    } else {
        m_plan      = makePlanInfo(PlanTier::Free);
        m_activeKey = QString();
    }
}

// ================================================================
// clearKey
// ================================================================

void PremiumManager::clearKey()
{
    QSettings s((QLatin1String(kOrg)), (QLatin1String(kApp)));
    s.remove(QStringLiteral("premium"));
    s.sync();

    m_plan      = makePlanInfo(PlanTier::Free);
    m_activeKey = QString();

    qInfo() << "[PremiumManager] Key cleared.";
}

// ================================================================
// validateKey  -  main entry point
// ================================================================

void PremiumManager::validateKey(const QString& key)
{
    if (key.isEmpty()) {
        emit validationError(QStringLiteral("Key cannot be empty."));
        return;
    }

    // ---- Built-in test key "373438" -> Free 7-day trial --------
    if (key.trimmed() == QLatin1String(kTestKey)) {
        PlanInfo plan    = makePlanInfo(PlanTier::Free);
        plan.isActive    = true;
        plan.expiresAt   = QDateTime::currentDateTimeUtc().addDays(7);
        m_plan           = plan;
        m_activeKey      = key.trimmed();

        saveToStorage(m_activeKey, m_plan);
        qInfo() << "[PremiumManager] Test key accepted – Free 7-day trial.";
        emit keyValidated(true, m_plan);
        return;
    }

    // ---- Check local cache first --------------------------------
    // If the stored key matches what was entered, use the cache.
    {
        QSettings s((QLatin1String(kOrg)), (QLatin1String(kApp)));
        if (s.contains(QLatin1String(kKeyStored))) {
            const QByteArray enc =
                QByteArray::fromBase64(
                    s.value(QLatin1String(kKeyStored)).toByteArray());
            const QString storedKey =
                QString::fromUtf8(xorBytes(enc, m_hwid.toUtf8()));

            if (storedKey == key.trimmed() && checkLocalKey()) {
                qInfo() << "[PremiumManager] Validated from local cache.";
                emit keyValidated(true, m_plan);
                return;
            }
        }
    }

    // ---- Remote validation -------------------------------------
    QUrl url = QUrl(QLatin1String(kValidateUrl));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    req.setRawHeader("User-Agent", "VoltroExecutor/1.5.0");

    QJsonObject body;
    body[QStringLiteral("key")]  = key.trimmed();
    body[QStringLiteral("hwid")] = m_hwid;

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = m_nam->post(req, payload);

    connect(reply, &QNetworkReply::finished, this, [this, reply, key]() {
        onValidationReply(reply, key.trimmed());
    });
}

// ----------------------------------------------------------------
// onValidationReply
// ----------------------------------------------------------------
void PremiumManager::onValidationReply(QNetworkReply* reply, const QString& key)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString errStr = reply->errorString();
        qWarning() << "[PremiumManager] Network error:" << errStr;

        // Fall back to local cache with grace period
        if (checkLocalKey() && m_activeKey == key) {
            qInfo() << "[PremiumManager] Using grace-period local cache.";
            emit keyValidated(true, m_plan);
        } else {
            emit validationError(
                QStringLiteral("Network error: ") + errStr);
        }
        return;
    }

    const QByteArray responseData = reply->readAll();
    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseErr);

    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit validationError(
            QStringLiteral("Invalid server response: ") + parseErr.errorString());
        return;
    }

    const QJsonObject obj = doc.object();
    const bool valid = obj.value(QStringLiteral("valid")).toBool(false);

    if (!valid) {
        const QString reason =
            obj.value(QStringLiteral("message")).toString(
                QStringLiteral("Key rejected by server."));
        emit validationError(reason);
        return;
    }

    // Parse tier
    const int tierInt =
        obj.value(QStringLiteral("tier")).toInt(0);
    const PlanTier tier = static_cast<PlanTier>(
        qBound(0, tierInt, static_cast<int>(PlanTier::Annual365Day)));

    // Parse expiry
    const QString expiryStr =
        obj.value(QStringLiteral("expires_at")).toString();
    QDateTime expiry = QDateTime::fromString(expiryStr, Qt::ISODate);
    if (!expiry.isValid()) {
        // Compute from duration if server does not provide absolute date
        const PlanInfo preset = makePlanInfo(tier);
        expiry = QDateTime::currentDateTimeUtc().addDays(preset.durationDays);
    }

    m_plan           = makePlanInfo(tier);
    m_plan.isActive  = true;
    m_plan.expiresAt = expiry;
    m_activeKey      = key;

    saveToStorage(m_activeKey, m_plan);

    qInfo() << "[PremiumManager] Key validated successfully. Tier:"
            << static_cast<int>(tier)
            << "Expires:" << expiry.toString(Qt::ISODate);

    emit keyValidated(true, m_plan);
}

// ================================================================
// Queries
// ================================================================

bool PremiumManager::isKeyValid() const
{
    if (!m_plan.isActive) return false;
    if (m_plan.tier == PlanTier::Free) {
        // Free trial validity is still tied to expiresAt
        if (m_plan.expiresAt.isValid())
            return QDateTime::currentDateTimeUtc() <= m_plan.expiresAt;
        return false;
    }
    if (!m_plan.expiresAt.isValid()) return false;
    return QDateTime::currentDateTimeUtc() <= m_plan.expiresAt;
}

PlanTier PremiumManager::currentTier() const
{
    return m_plan.tier;
}

const PlanInfo& PremiumManager::currentPlan() const
{
    return m_plan;
}

int PremiumManager::daysRemaining() const
{
    if (m_plan.tier == PlanTier::Free && !m_plan.expiresAt.isValid())
        return 0;
    if (!m_plan.expiresAt.isValid()) return 0;

    const qint64 days =
        QDateTime::currentDateTimeUtc().daysTo(m_plan.expiresAt);
    return static_cast<int>(qMax<qint64>(0, days));
}

QString PremiumManager::hwid() const
{
    return m_hwid;
}

// Suppress the erroneous decryptLocal stub - the real implementation
// uses xorBytes() directly in checkLocalKey() / saveToStorage().
// decryptLocal is retained for API completeness:

QString PremiumManager::decryptLocal(const QByteArray& data) const
{
    const QByteArray enc = QByteArray::fromBase64(data);
    return QString::fromUtf8(xorBytes(enc, m_hwid.toUtf8()));
}

// encryptLocal - thin wrapper around xorBytes + base64
QByteArray PremiumManager::encryptLocal(const QString& data) const
{
    return xorBytes(data.toUtf8(), m_hwid.toUtf8()).toBase64();
}

} // namespace Voltro
