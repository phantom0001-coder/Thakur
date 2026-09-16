#pragma once







#ifndef VOLTRO_UPDATER_H
#define VOLTRO_UPDATER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Voltro {




struct ReleaseInfo {
    QString   version;        
    QString   tagName;        
    QString   changelog;      
    QString   downloadUrl;    
    QString   sha256;         
    QDateTime publishedAt;
    bool      isPrerelease { false };
};




class Updater : public QObject
{
    Q_OBJECT

public:
    
    static Updater& instance();

    Updater(const Updater&)            = delete;
    Updater& operator=(const Updater&) = delete;

    

    
    
    void checkForUpdates();

    
    
    void downloadUpdate(const ReleaseInfo& release);

    
    
    
    void applyUpdate();

    
    bool isUpdateAvailable() const;

    
    const ReleaseInfo& latestRelease() const;

    
    
    QString currentVersion() const;

    
    
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

    

    
    
    void onPrivateVersionReply(QNetworkReply* reply);

    
    void onGitHubReply(QNetworkReply* reply);

    
    ReleaseInfo parseGitHubRelease(const QByteArray& json) const;

    

    
    
    bool verifyChecksum(const QString& filePath,
                        const QString& expected);

    
    
    void extractAndReplace(const QString& zipPath);

    

    QNetworkAccessManager* m_nam             { nullptr };
    ReleaseInfo            m_latest;
    bool                   m_updateAvailable { false };
    QString                m_downloadPath;   
};

} 

Q_DECLARE_METATYPE(Voltro::ReleaseInfo)

#endif 
