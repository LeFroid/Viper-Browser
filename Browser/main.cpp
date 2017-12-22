#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SecurityManager.h"

#include <memory>

int main(int argc, char *argv[])
{
    BrowserApplication a(argc, argv);
    a.getNewWindow();
    int val = a.exec();
    return val;
}
