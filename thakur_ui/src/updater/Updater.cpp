

namespace {

constexpr const char* kPrivateVersionUrl =
    "https:

constexpr const char* kGitHubReleasesUrl =
    "https:

constexpr const char* kUserAgent = "ThakurExecutor/1.5.0";

constexpr const char* kOrg         = "Thakur";
constexpr const char* kApp         = "ThakurExecutor";
constexpr const char* kLastVersion = "updater/lastKnownVersion";

QList<int> parseSemVer(const QString& v)
{
    
    QString s = v;
    if (s.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        s = s.mid(1);

    QList<int> parts;
    const QStringList tokens = s.split(QLatin1Char('.'));
    for (const QString& t : tokens) {
        bool ok = false;
        const int n = t.toInt(&ok);
        parts.append(ok ? n : 0);
    }
    
    while (parts.size() < 3) parts.append(0);
    return parts;
}

} 

namespace Thakur {

Updater& Updater::instance()
{
    static Updater s_instance;
    return s_instance;
}

Updater::Updater(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    qRegisterMetaType<Thakur::ReleaseInfo>("Thakur::ReleaseInfo");
}

QString Updater::currentVersion() const
{
    return QCoreApplication::applicationVersion();
}

bool Updater::compareVersions(const QString& a, const QString& b) const
{
    
    const QList<int> va = parseSemVer(a);
    const QList<int> vb = parseSemVer(b);

    const int len = qMax(va.size(), vb.size());
    for (int i = 0; i < len; ++i) {
        const int ia = (i < va.size()) ? va[i] : 0;
        const int ib = (i < vb.size()) ? vb[i] : 0;
        if (ia < ib) return true;
        if (ia > ib) return false;
    }
    return false; 
}

bool Updater::isUpdateAvailable() const
{
    return m_updateAvailable;
}

const ReleaseInfo& Updater::latestRelease() const
{
    return m_latest;
}

void Updater::checkForUpdates()
{
    qInfo() << "[Updater] Checking for updates. Current version:"
            << currentVersion();

    QNetworkRequest req(QUrl(QLatin1String(kPrivateVersionUrl)));
    req.setRawHeader("User-Agent", kUserAgent);
    req.setRawHeader("Accept",     "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onPrivateVersionReply(reply);
    });
}

void Updater::onPrivateVersionReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[Updater] Private server unavailable:"
                   << reply->errorString()
                   << "– falling back to GitHub.";
        
        QNetworkRequest req(QUrl(QLatin1String(kGitHubReleasesUrl)));
        req.setRawHeader("User-Agent", kUserAgent);
        req.setRawHeader("Accept",     "application/vnd.github+json");
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

        QNetworkReply* ghReply = m_nam->get(req);
        connect(ghReply, &QNetworkReply::finished, this, [this, ghReply]() {
            onGitHubReply(ghReply);
        });
        return;
    }

    const QByteArray data = reply->readAll();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);

    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Updater] Private server returned invalid JSON – "
                      "falling back to GitHub.";
        QNetworkRequest req(QUrl(QLatin1String(kGitHubReleasesUrl)));
        req.setRawHeader("User-Agent", kUserAgent);
        req.setRawHeader("Accept",     "application/vnd.github+json");
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

        QNetworkReply* ghReply = m_nam->get(req);
        connect(ghReply, &QNetworkReply::finished, this, [this, ghReply]() {
            onGitHubReply(ghReply);
        });
        return;
    }

    
    
    
    
    const QJsonObject obj = doc.object();

    ReleaseInfo info;
    info.version    = obj.value(QStringLiteral("version")).toString();
    info.tagName    = obj.value(QStringLiteral("tag")).toString(
                          QStringLiteral("v") + info.version);
    info.downloadUrl= obj.value(QStringLiteral("download_url")).toString();
    info.sha256     = obj.value(QStringLiteral("sha256")).toString();
    info.changelog  = obj.value(QStringLiteral("changelog")).toString();
    info.isPrerelease = obj.value(QStringLiteral("prerelease")).toBool(false);

    const QString pubStr =
        obj.value(QStringLiteral("published_at")).toString();
    info.publishedAt = QDateTime::fromString(pubStr, Qt::ISODate);

    if (info.version.isEmpty()) {
        qWarning() << "[Updater] Private server returned empty version.";
        
        QNetworkRequest req2(QUrl(QLatin1String(kGitHubReleasesUrl)));
        req2.setRawHeader("User-Agent", kUserAgent);
        req2.setRawHeader("Accept",     "application/vnd.github+json");
        req2.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* ghReply = m_nam->get(req2);
        connect(ghReply, &QNetworkReply::finished, this, [this, ghReply]() {
            onGitHubReply(ghReply);
        });
        return;
    }

    
    QSettings s(QLatin1String(kOrg), QLatin1String(kApp));
    s.setValue(QLatin1String(kLastVersion), info.version);

    if (compareVersions(currentVersion(), info.version)) {
        m_latest          = info;
        m_updateAvailable = true;
        qInfo() << "[Updater] Update available (private):"
                << info.version;
        emit updateAvailable(m_latest);
    } else {
        m_updateAvailable = false;
        qInfo() << "[Updater] Already up-to-date (private check).";
        emit noUpdateAvailable();
    }
}

void Updater::onGitHubReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = reply->errorString();
        qWarning() << "[Updater] GitHub API error:" << err;
        emit updateError(QStringLiteral("Failed to reach update server: ") + err);
        return;
    }

    const QByteArray data = reply->readAll();
    const ReleaseInfo info = parseGitHubRelease(data);

    if (info.version.isEmpty()) {
        emit updateError(QStringLiteral("Could not parse GitHub release data."));
        return;
    }

    
    QSettings s(QLatin1String(kOrg), QLatin1String(kApp));
    s.setValue(QLatin1String(kLastVersion), info.version);

    if (compareVersions(currentVersion(), info.version)) {
        m_latest          = info;
        m_updateAvailable = true;
        qInfo() << "[Updater] Update available (GitHub):" << info.version;
        emit updateAvailable(m_latest);
    } else {
        m_updateAvailable = false;
        qInfo() << "[Updater] Already up-to-date (GitHub check).";
        emit noUpdateAvailable();
    }
}

ReleaseInfo Updater::parseGitHubRelease(const QByteArray& json) const
{
    ReleaseInfo info;

    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject())
        return info;

    const QJsonObject obj = doc.object();
    info.tagName      = obj.value(QStringLiteral("tag_name")).toString();
    info.changelog    = obj.value(QStringLiteral("body")).toString();
    info.isPrerelease = obj.value(QStringLiteral("prerelease")).toBool(false);

    
    info.version = info.tagName;
    if (info.version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        info.version = info.version.mid(1);

    const QString pubStr =
        obj.value(QStringLiteral("published_at")).toString();
    info.publishedAt = QDateTime::fromString(pubStr, Qt::ISODate);

    
    const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& v : assets) {
        const QJsonObject asset = v.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (name.endsWith(QLatin1String(".zip"), Qt::CaseInsensitive)) {
            info.downloadUrl =
                asset.value(QStringLiteral("browser_download_url")).toString();
            
            info.sha256 =
                asset.value(QStringLiteral("sha256")).toString();
            break;
        }
    }

    
    if (info.downloadUrl.isEmpty())
        info.downloadUrl =
            obj.value(QStringLiteral("zipball_url")).toString();

    return info;
}

void Updater::downloadUpdate(const ReleaseInfo& release)
{
    if (release.downloadUrl.isEmpty()) {
        emit updateError(QStringLiteral("No download URL available for this release."));
        return;
    }

    const QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_downloadPath = tempDir + QStringLiteral("/thakur_update.zip");

    
    if (QFile::exists(m_downloadPath))
        QFile::remove(m_downloadPath);

    qInfo() << "[Updater] Downloading update from" << release.downloadUrl
            << "-> " << m_downloadPath;

    QNetworkRequest req(QUrl(release.downloadUrl));
    req.setRawHeader("User-Agent", kUserAgent);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_nam->get(req);

    
    QFile* outFile = new QFile(m_downloadPath, reply); 
    if (!outFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit updateError(
            QStringLiteral("Cannot open temp file for writing: ") +
            m_downloadPath);
        reply->abort();
        reply->deleteLater();
        return;
    }

    connect(reply, &QNetworkReply::readyRead, this, [reply, outFile]() {
        outFile->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::downloadProgress,
            this, [this](qint64 received, qint64 total) {
        emit downloadProgress(received, total);
    });

    const QString expectedSha = release.sha256;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, outFile, expectedSha]() {
        outFile->flush();
        outFile->close();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit updateError(
                QStringLiteral("Download failed: ") + reply->errorString());
            QFile::remove(m_downloadPath);
            return;
        }

        qInfo() << "[Updater] Download complete:" << m_downloadPath;

        
        if (!expectedSha.isEmpty()) {
            if (!verifyChecksum(m_downloadPath, expectedSha)) {
                
                QFile::remove(m_downloadPath);
                return;
            }
        }

        emit downloadComplete(m_downloadPath);
    });
}

bool Updater::verifyChecksum(const QString& filePath, const QString& expected)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        emit updateError(
            QStringLiteral("Cannot open file for checksum verification: ") +
            filePath);
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f)) {
        emit updateError(QStringLiteral("Failed to read file for checksum."));
        return false;
    }
    f.close();

    const QString computed =
        QString::fromLatin1(hash.result().toHex()).toLower();
    const QString exp      = expected.trimmed().toLower();

    if (computed != exp) {
        qCritical() << "[Updater] Checksum mismatch!"
                    << "\n  Expected: " << exp
                    << "\n  Computed: " << computed;
        emit updateError(
            QStringLiteral("Checksum mismatch – update package may be corrupt."));
        return false;
    }

    qInfo() << "[Updater] Checksum verified OK:" << computed;
    return true;
}

void Updater::extractAndReplace(const QString& zipPath)
{
    const QString exePath = QCoreApplication::applicationFilePath();
    const QFileInfo exeInfo(exePath);
    const QString exeDir   = exeInfo.absolutePath();
    const QString exeName  = exeInfo.fileName();
    const QString backupPath = exeDir + QStringLiteral("/thakur_backup.exe");

    
    if (QFile::exists(backupPath))
        QFile::remove(backupPath);

    
    if (!QFile::copy(exePath, backupPath)) {
        emit updateError(
            QStringLiteral("Failed to create backup of current executable."));
        return;
    }
    qInfo() << "[Updater] Backup created:" << backupPath;

    
    
    const QString extractDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        QStringLiteral("/thakur_extracted");

    
    QDir(extractDir).removeRecursively();

    QProcess psProc;
    psProc.setProgram(QStringLiteral("powershell.exe"));
    psProc.setArguments({
        QStringLiteral("-NoProfile"),
        QStringLiteral("-NonInteractive"),
        QStringLiteral("-Command"),
        QString(QStringLiteral(
            "Expand-Archive -Force -LiteralPath '%1' -DestinationPath '%2'"))
            .arg(zipPath, extractDir)
    });

    psProc.start();
    if (!psProc.waitForFinished(60000)) { 
        psProc.kill();
        
        QFile::copy(backupPath, exePath);
        emit updateError(QStringLiteral("Extraction timed out."));
        return;
    }

    if (psProc.exitCode() != 0) {
        const QString psErr =
            QString::fromLocal8Bit(psProc.readAllStandardError());
        qCritical() << "[Updater] Expand-Archive failed:" << psErr;
        
        QFile::copy(backupPath, exePath);
        emit updateError(
            QStringLiteral("Extraction failed: ") + psErr);
        return;
    }

    
    
    QString newExePath;
    QDirIterator it(extractDir,
                    QStringList{exeName},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    if (it.hasNext())
        newExePath = it.next();

    if (newExePath.isEmpty()) {
        
        QDirIterator it2(extractDir,
                         QStringList{QStringLiteral("*.exe")},
                         QDir::Files,
                         QDirIterator::Subdirectories);
        if (it2.hasNext())
            newExePath = it2.next();
    }

    if (newExePath.isEmpty()) {
        qCritical() << "[Updater] New executable not found in extracted archive.";
        QFile::copy(backupPath, exePath);
        emit updateError(
            QStringLiteral("New executable not found in the update archive."));
        return;
    }

    
    
    
    

    
    
    const QString helperScript =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        QStringLiteral("/thakur_updater_helper.ps1");

    const QString scriptContent = QString(
        QStringLiteral(
            "$pid_to_wait = %1\n"
            "$maxWait = 30\n"
            "for ($i = 0; $i -lt $maxWait; $i++) {\n"
            "    $proc = Get-Process -Id $pid_to_wait -ErrorAction SilentlyContinue\n"
            "    if (-not $proc) { break }\n"
            "    Start-Sleep -Seconds 1\n"
            "}\n"
            "Copy-Item -Force -LiteralPath '%2' -Destination '%3'\n"
            "Start-Process -FilePath '%3'\n"
        ))
        .arg(QCoreApplication::applicationPid())
        .arg(newExePath, exePath);

    {
        QFile hf(helperScript);
        if (!hf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QFile::copy(backupPath, exePath);
            emit updateError(
                QStringLiteral("Failed to write updater helper script."));
            return;
        }
        hf.write(scriptContent.toUtf8());
        hf.close();
    }

    qInfo() << "[Updater] Replacement scheduled via helper script:" << helperScript;
    qInfo() << "[Updater] New exe source:" << newExePath;

    
    m_downloadPath = helperScript; 
}

void Updater::applyUpdate()
{
    if (m_downloadPath.isEmpty()) {
        emit updateError(QStringLiteral(
            "No update package ready. Call downloadUpdate() first."));
        return;
    }

    
    if (m_downloadPath.endsWith(QLatin1String(".zip"), Qt::CaseInsensitive)) {
        extractAndReplace(m_downloadPath);
        
        if (m_downloadPath.endsWith(QLatin1String(".zip"))) {
            
            return;
        }
    }

    
    const bool launched = QProcess::startDetached(
        QStringLiteral("powershell.exe"),
        {
            QStringLiteral("-NoProfile"),
            QStringLiteral("-NonInteractive"),
            QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
            QStringLiteral("-File"), m_downloadPath
        });

    if (!launched) {
        emit updateError(QStringLiteral("Failed to launch updater helper process."));
        return;
    }

    qInfo() << "[Updater] Helper launched. Exiting application for update.";
    emit updateApplied();

    
    QTimer::singleShot(500, qApp, []() {
        QCoreApplication::exit(0);
    });
}

} 
