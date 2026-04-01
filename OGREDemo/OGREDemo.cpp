#include "OGREDemo.h"
#include "OgreWidget.h"
OGREDemo::OGREDemo(QWidget* parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    m_ogreWidget = new OgreWidget(this);
    setCentralWidget(m_ogreWidget);
}

OGREDemo::~OGREDemo()
{
}
