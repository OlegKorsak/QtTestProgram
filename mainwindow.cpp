#include "mainwindow.h"
#include "screenshot.h"
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QPixmap>
#include <QCryptographicHash>
#include <QBuffer>
#include <qmath.h>

bool IsProcessGoing = false;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    screenshotTimer = new QTimer(this);
    screenshotTimer->setInterval(60000);
    connect(screenshotTimer, SIGNAL(timeout()), this, SLOT(captureScreenshot()));

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QGridLayout *layout = new QGridLayout(centralWidget);
    QLabel *screenshotLabel = new QLabel(this);
    layout->addWidget(screenshotLabel, 0, 0);

    QPushButton *startButton = new QPushButton("Start", this);
    connect(startButton, SIGNAL(clicked()), this, SLOT(startScreenshotProcess()));
    layout->addWidget(startButton, 1, 0);

    QPushButton *stopButton = new QPushButton("Stop", this);
    connect(stopButton, SIGNAL(clicked()), this, SLOT(stopScreenshotProcess()));
    layout->addWidget(stopButton, 1, 1);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    layout->addWidget(scrollArea, 2, 0, 1, 2);

    scrollWidget = new QWidget(this);
    scrollLayout = new QVBoxLayout(scrollWidget);
    scrollArea->setWidget(scrollWidget);

    resize(800, 600);
}

bool MainWindow::openDatabase()
{
    QString databaseFilePath = "C:/Users/LENOVO/Desktop/TestProgram.db";

    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
    }

    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(databaseFilePath);

    if (!database.open()) {
        qDebug() << "Failed to open the database:" << database.lastError().text();
        return false;
    };

    if (!database.tables().contains("screenshots")) {
        QSqlQuery createTableQuery(database);
        QString createTableSQL = "CREATE TABLE IF NOT EXISTS screenshots ("
                                 "id INTEGER PRIMARY KEY,"
                                 "screenshot BLOB,"
                                 "hash TEXT,"
                                 "similarity_percentage REAL)";
        if (!createTableQuery.exec(createTableSQL)) {
            qDebug() << "Failed to create screenshot table:" << createTableQuery.lastError().text();
            database.close();
            return false;
        } else {
            qDebug() << "Screenshot table created successfully";
        }
    }

    return true;
}

void MainWindow::closeDatabase()
{
    QSqlDatabase database = QSqlDatabase::database();
    database.close();

    if (database.isOpen()) {
        qDebug() << "Failed to close the database";
    } else {
        qDebug() << "Database closed successfully";
    }
}

double MainWindow::calculateSimilarityPercentage(const QImage& image1, const QImage& image2)
{
    int width = image1.width();
    int height = image1.height();

    double mse = 0.0;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            QRgb pixel1 = image1.pixel(x, y);
            QRgb pixel2 = image2.pixel(x, y);

            double diff = qRed(pixel1) - qRed(pixel2);
            mse += diff * diff;
        }
    }

    mse /= static_cast<double>(width * height);
    double rmse = qSqrt(mse);

    double similarityPercentage = 0;

    if((100.0 - rmse) > 0)
        similarityPercentage = 100.0 - rmse;


    return similarityPercentage;
}

void MainWindow::displayScreenshot(const QPixmap& screenshot, const QByteArray& hash, float similarityPercentage)
{
    QWidget* screenshotWidget = new QWidget(this);

    QHBoxLayout* layout = new QHBoxLayout(screenshotWidget);

    QLabel* screenshotLabel = new QLabel(this);
    screenshotLabel->setPixmap(screenshot.scaled(screenshot.width() / 4, screenshot.height() / 4, Qt::KeepAspectRatio));
    layout->addWidget(screenshotLabel);

    QString hexHash = QString::fromLatin1(hash.toHex());

    QLabel* hashLabel = new QLabel(hexHash, this);
    layout->addWidget(hashLabel);

    QLabel* similarityLabel = new QLabel(QString::number(similarityPercentage, 'f', 2) + "%", this);
    layout->addWidget(similarityLabel);

    scrollLayout->addWidget(screenshotWidget);
}

void MainWindow::captureScreenshot()
{
    Screenshot screenshot;
    QPixmap currentScreenshot = screenshot.getScreenshot();
    if (currentScreenshot.isNull()) {
        return;
    }

    QByteArray screenshotHash = calculateHash(currentScreenshot);

    if (!previousScreenshot.isNull())
    {
        double similarityPercentage = calculateSimilarityPercentage(previousScreenshot.toImage(), currentScreenshot.toImage());

        displayScreenshot(currentScreenshot, screenshotHash, similarityPercentage);

        if (openDatabase())
            insertScreenshotToDatabase(currentScreenshot, screenshotHash, similarityPercentage);
    }
    else
    {
        displayScreenshot(currentScreenshot, screenshotHash, 0);
    }

    previousScreenshot = currentScreenshot;
}


QByteArray MainWindow::calculateHash(const QPixmap& screenshot)
{
    QByteArray hash;
    QCryptographicHash cryptoHash(QCryptographicHash::Md5);
    QBuffer buffer(&hash);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "PNG");
    cryptoHash.addData(hash);
    return cryptoHash.result();
}

bool MainWindow::insertScreenshotToDatabase(const QPixmap& screenshot, const QByteArray& screenshotHash, const double similarityPercentage)
{
    QSqlQuery query;
    query.prepare("INSERT INTO screenshots (screenshot, hash, similarity_percentage) VALUES (:screenshot, :hash, :percentage)");

    QByteArray screenshotData;
    QBuffer buffer(&screenshotData);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "PNG");
    query.bindValue(":screenshot", screenshotData);
    query.bindValue(":hash", screenshotHash);
    query.bindValue(":percentage", similarityPercentage);

    if (!query.exec()) {
        qDebug() << "Failed to execute insert query:" << query.lastError().text();
        return false;
    }

    QSqlDatabase::database().transaction();

    if (!QSqlDatabase::database().commit()) {
        qDebug() << "Failed to commit the transaction:" << QSqlDatabase::database().lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }

    return true;
}

void MainWindow::startScreenshotProcess()
{
    if (!IsProcessGoing) {
        screenshotTimer->start();
        IsProcessGoing = true;

        if (openDatabase()) {
            QSqlDatabase::database().transaction();

            captureScreenshot();

            bool success = QSqlDatabase::database().commit();
            if (!success) {
                QSqlDatabase::database().rollback();
            }
        }
    }
}

void MainWindow::stopScreenshotProcess()
{
    if (IsProcessGoing){
        qDebug() << "Stoped";
        screenshotTimer->stop();
        IsProcessGoing = !IsProcessGoing;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    closeDatabase();
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    screenshotTimer->stop();
    delete screenshotTimer;
}
