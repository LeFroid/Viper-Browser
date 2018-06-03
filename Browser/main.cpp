#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SecurityManager.h"

#include <memory>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

int main(int argc, char *argv[])
{
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    BrowserApplication a(argc, argv);
#else
    int argc2 = 2;
    char *argv2[] = { argv[0], "--remote-debugging-port=9477" };
    BrowserApplication a(argc2, argv2);
#endif
    a.getNewWindow();
    return a.exec();
}
