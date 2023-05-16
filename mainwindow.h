#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPixmap>
#include <QSqlDatabase>
#include <QScrollArea>
#include <QVBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void captureScreenshot();
    void startScreenshotProcess();
    void stopScreenshotProcess();

private:
    QTimer *screenshotTimer;
    QPixmap previousScreenshot;
    QSqlDatabase database;


    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *scrollLayout;


    void displayScreenshot(const QPixmap& screenshot, const QByteArray& hash, float similarityPercentage);
    void saveScreenshotToDatabase(const QPixmap& screenshot);
    int countBits(unsigned char value);
    QByteArray calculateHash(const QPixmap& pixmap);
    bool openDatabase();
    void closeDatabase();
    bool createScreenshotsTable();
    bool insertScreenshotToDatabase(const QPixmap& screenshot, const QByteArray& screenshotHash, const double similarityPercentage);
    double calculateSimilarityPercentage(const QImage& image1, const QImage& image2);
    bool insertSimilarityPercentage(double similarityPercentage);
    void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
