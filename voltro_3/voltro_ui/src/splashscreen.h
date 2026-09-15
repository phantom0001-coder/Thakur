#pragma once

#include <QWidget>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class SplashScreen; }
QT_END_NAMESPACE

namespace Voltro {

class SplashScreen : public QWidget {
    Q_OBJECT

public:
    explicit SplashScreen(QWidget *parent = nullptr);
    ~SplashScreen();

signals:
    void initializationFinished();

private slots:
    void updateProgress();

private:
    Ui::SplashScreen *ui;
    QTimer *m_timer;
    int m_progress = 0;
};

} // namespace Voltro
