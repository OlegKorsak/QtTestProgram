#include "screenshot.h"

QPixmap Screenshot::getScreenshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        return screen->grabWindow(0);
    }
    return QPixmap();
}
