#pragma once







#ifndef VOLTRO_PREMIUMMANAGER_H
#define VOLTRO_PREMIUMMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QByteArray>
#include <QNetworkAccessManager>

namespace Voltro {




enum class PlanTier : int {
    Free           = 0,  
    Monthly30Day   = 1,  
    Quarterly90Day = 2,  
    Annual365Day   = 3   
};




struct PlanInfo {
    PlanTier    tier         { PlanTier::Free };
    QString     name;
    int         priceRobux   { 0 };
    int         durationDays { 0 };
    QStringList features;
    bool        isActive     { false };
    QDateTime   expiresAt;          
};




class PremiumManager : public QObject
{
    Q_OBJECT

public:
    
    static PremiumManager& instance();

    
    PremiumManager(const PremiumManager&)            = delete;
    PremiumManager& operator=(const PremiumManager&) = delete;

    

    
    
    void validateKey(const QString& key);

    
    bool     isKeyValid()   const;

    
    PlanTier currentTier()  const;

    
    const PlanInfo& currentPlan() const;

    
    int      daysRemaining() const;

    
    QString  hwid()          const;

    
    
    void     loadFromStorage();

    
    void     clearKey();

signals:
    
    void keyValidated(bool valid, const Voltro::PlanInfo& plan);

    
    void keyExpired();

    
    void validationError(const QString& msg);

private:
    explicit PremiumManager(QObject* parent = nullptr);
    ~PremiumManager() override = default;

    

    
    
    
    QString generateHWID() const;

    

    
    void saveToStorage(const QString& key, const PlanInfo& plan);

    
    QString    decryptLocal(const QByteArray& data) const;

    
    QByteArray encryptLocal(const QString& data)    const;

    
    
    
    bool checkLocalKey();

    

    
    void onValidationReply(class QNetworkReply* reply,
                           const QString&       key);

    

    
    static PlanInfo makePlanInfo(PlanTier tier);

    

    QNetworkAccessManager* m_nam      { nullptr };
    PlanInfo               m_plan;           
    QString                m_hwid;           
    QString                m_activeKey;      
};

} 


Q_DECLARE_METATYPE(Voltro::PlanInfo)

#endif 
