#pragma once

// ============================================================
//  PremiumManager.h  -  Voltro Executor  (Phase 5)
//  Manages licence key validation, plan tiers, HWID binding,
//  and encrypted local caching of the active subscription.
// ============================================================

#ifndef VOLTRO_PREMIUMMANAGER_H
#define VOLTRO_PREMIUMMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QByteArray>
#include <QNetworkAccessManager>

namespace Voltro {

// ----------------------------------------------------------------
// Plan tier identifiers
// ----------------------------------------------------------------
enum class PlanTier : int {
    Free           = 0,  ///< No paid subscription / trial key
    Monthly30Day   = 1,  ///< 30-day plan  (299 Robux)
    Quarterly90Day = 2,  ///< 90-day plan  (699 Robux)
    Annual365Day   = 3   ///< 365-day plan (1 499 Robux)
};

// ----------------------------------------------------------------
// Per-plan metadata
// ----------------------------------------------------------------
struct PlanInfo {
    PlanTier    tier         { PlanTier::Free };
    QString     name;
    int         priceRobux   { 0 };
    int         durationDays { 0 };
    QStringList features;
    bool        isActive     { false };
    QDateTime   expiresAt;          ///< Null/invalid = never set
};

// ----------------------------------------------------------------
// PremiumManager
// ----------------------------------------------------------------
class PremiumManager : public QObject
{
    Q_OBJECT

public:
    /// Singleton access - must only be used from the main thread.
    static PremiumManager& instance();

    // Deleted copy / move to enforce singleton semantics
    PremiumManager(const PremiumManager&)            = delete;
    PremiumManager& operator=(const PremiumManager&) = delete;

    // -- Public API -----------------------------------------------

    /// Asynchronously validate key against the remote API (or local cache).
    /// Result is delivered via keyValidated().
    void validateKey(const QString& key);

    /// Returns true when a valid, non-expired key is held.
    bool     isKeyValid()   const;

    /// The tier of the currently active plan.
    PlanTier currentTier()  const;

    /// Full metadata for the currently active plan.
    const PlanInfo& currentPlan() const;

    /// Remaining subscription days (0 if Free / expired).
    int      daysRemaining() const;

    /// The hardware identifier bound to this installation.
    QString  hwid()          const;

    /// Restore a previously validated session from persistent
    /// storage.  Call this once during application start-up.
    void     loadFromStorage();

    /// Wipe the stored key and reset to the Free tier.
    void     clearKey();

signals:
    /// Emitted when remote (or cached) validation completes.
    void keyValidated(bool valid, const Voltro::PlanInfo& plan);

    /// Emitted when a previously valid key has passed its expiry.
    void keyExpired();

    /// Emitted when a network or protocol error prevents validation.
    void validationError(const QString& msg);

private:
    explicit PremiumManager(QObject* parent = nullptr);
    ~PremiumManager() override = default;

    // -- HWID generation (Win32) ----------------------------------

    /// Build a deterministic HWID from the C-drive serial number and
    /// the first non-loopback MAC address, SHA-256 hashed and
    /// formatted as "HWID-XXXX-XXXX-XXXX-XXXX".
    QString generateHWID() const;

    // -- Persistent storage helpers --------------------------------

    /// Persist key and plan to QSettings (XOR-encrypted).
    void saveToStorage(const QString& key, const PlanInfo& plan);

    /// XOR-decrypt data using the HWID as the repeating key.
    QString    decryptLocal(const QByteArray& data) const;

    /// XOR-encrypt data using the HWID as the repeating key.
    QByteArray encryptLocal(const QString& data)    const;

    /// Read the stored key and check its expiry.
    /// Returns true and populates m_plan if still valid.
    /// Implements a 7-day offline grace period.
    bool checkLocalKey();

    // -- Network helpers ------------------------------------------

    /// Called when the remote API replies to a validation request.
    void onValidationReply(class QNetworkReply* reply,
                           const QString&       key);

    // -- Static plan presets --------------------------------------

    /// Returns a PlanInfo pre-filled with canonical values for tier.
    static PlanInfo makePlanInfo(PlanTier tier);

    // -- Member data ----------------------------------------------

    QNetworkAccessManager* m_nam      { nullptr };
    PlanInfo               m_plan;           ///< Currently active plan
    QString                m_hwid;           ///< Cached HWID string
    QString                m_activeKey;      ///< Currently held key
};

} // namespace Voltro

// Make PlanInfo usable in queued signal/slot connections
Q_DECLARE_METATYPE(Voltro::PlanInfo)

#endif // VOLTRO_PREMIUMMANAGER_H
