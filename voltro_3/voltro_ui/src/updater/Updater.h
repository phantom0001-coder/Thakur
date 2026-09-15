#pragma once

// ============================================================
//  Updater.h  -  Voltro Executor  (Phase 6)
//  Auto-updater: checks GitHub releases and a private update
//  server, downloads, verifies (SHA-256), and applies updates.
// ============================================================

#ifndef VOLTRO_UPDATER_H
#define VOLTRO_UPDATER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Voltro {

// ----------------------------------------------------------------
// Metadata for a single release
// ----------------------------------------------------------------
struct ReleaseInfo {
    QString   version;        ///< Semver string, e.g. "1.5.1"
    QString   tagName;        ///< Git tag,        e.g. "v1.5.1"
    QString   changelog;      ///< Release body (Markdown)
    QString   downloadUrl;    ///< Direct URL to the zip asset
    QString   sha256;         ///< Expected hex-encoded SHA-256
    QDateTime publishedAt;
    bool      isPrerelease { false };
};

// ----------------------------------------------------------------
// Updater
// ----------------------------------------------------------------
class Updater : public QObject
{
    Q_OBJECT

public:
    /// Singleton access - main thread only.
    static Updater& instance();

    Updater(const Updater&)            = delete;
    Updater& operator=(const Updater&) = delete;

    // -- Public API -----------------------------------------------

    /// Asynchronously query the update servers.
    /// Emits updateAvailable() or noUpdateAvailable().
    void checkForUpdates();

    /// Begin downloading the zip for release.
    /// Emits downloadProgress() and downloadComplete().
    void downloadUpdate(const ReleaseInfo& release);

    /// Verify the downloaded archive, back up the current
    /// executable, extract, and schedule a relaunch.
    /// Emits updateApplied() or updateError().
    void applyUpdate();

    /// True if the last checkForUpdates() found a newer version.
    bool isUpdateAvailable() const;

    /// Metadata for the latest release found.
    const ReleaseInfo& latestRelease() const;

    /// The version string compiled into this binary
    /// (QApplication::applicationVersion()).
    QString currentVersion() const;

    /// Returns true when a is strictly older than b.
    /// Compares dotted semver components as integers.
    bool compareVersions(const QString& a, const QString& b) const;

signals:
    void updateAvailable(const Voltro::ReleaseInfo& release);
    void noUpdateAvailable();
    void downloadProgress(qint64 received, qint64 total);
    void downloadComplete(const QString& localPath);
    void updateApplied();
    void updateError(const QString& error);

private:
    explicit Updater(QObject* parent = nullptr);
    ~Updater() override = default;

    // -- Network helpers ------------------------------------------

    /// Called with the JSON body from the private version endpoint.
    /// Falls back to GitHub on failure.
    void onPrivateVersionReply(QNetworkReply* reply);

    /// Called with the JSON body from GitHub releases/latest.
    void onGitHubReply(QNetworkReply* reply);

    /// Parse a GitHub release JSON object into a ReleaseInfo.
    ReleaseInfo parseGitHubRelease(const QByteArray& json) const;

    // -- Download helpers -----------------------------------------

    /// Compute SHA-256 of filePath and compare against expected.
    /// Emits updateError() and returns false on mismatch.
    bool verifyChecksum(const QString& filePath,
                        const QString& expected);

    /// Backup current exe, PowerShell-extract the zip, replace the
    /// binary.  Restores the backup automatically on failure.
    void extractAndReplace(const QString& zipPath);

    // -- Member data ----------------------------------------------

    QNetworkAccessManager* m_nam             { nullptr };
    ReleaseInfo            m_latest;
    bool                   m_updateAvailable { false };
    QString                m_downloadPath;   ///< Full path to downloaded zip
};

} // namespace Voltro

Q_DECLARE_METATYPE(Voltro::ReleaseInfo)

#endif // VOLTRO_UPDATER_H
