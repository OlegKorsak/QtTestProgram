#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>

class Screenshot
{
public:
    QPixmap getScreenshot();
};

#endif // SCREENSHOT_H
