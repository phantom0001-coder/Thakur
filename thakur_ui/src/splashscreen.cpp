

namespace Thakur {

SplashScreen::SplashScreen(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SplashScreen)
    , m_timer(new QTimer(this))
{
    ui->setupUi(this);

    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    ui->progressBar->setValue(0);

    connect(m_timer, &QTimer::timeout, this, &SplashScreen::updateProgress);
    m_timer->start(25); 
}

SplashScreen::~SplashScreen() {
    delete ui;
}

void SplashScreen::updateProgress() {
    m_progress += 2;
    ui->progressBar->setValue(m_progress);

    if (m_progress == 20) {
        ui->lblStatus->setText("Checking Luau Virtual Sandbox...");
    } else if (m_progress == 50) {
        ui->lblStatus->setText("Initializing Multi-Hub Script Aggregator APIs...");
    } else if (m_progress == 80) {
        ui->lblStatus->setText("Securing Process VM Space & Page Guards...");
    } else if (m_progress >= 100) {
        m_timer->stop();
        ui->lblStatus->setText("Thakur Engine Ready!");
        emit initializationFinished();
    }
}

} 
