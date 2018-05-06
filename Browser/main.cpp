#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SecurityManager.h"

#include <memory>

int main(int argc, char *argv[])
{
    int argc2 = 2;
    char *argv2[] = { argv[0], "--remote-debugging-port=9477" };
    BrowserApplication a(argc2, argv2);
    a.getNewWindow();
    int val = a.exec();
    return val;
}
