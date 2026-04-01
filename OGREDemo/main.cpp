#include "OGREDemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    OGREDemo window;
    window.show();
    return app.exec();
}
