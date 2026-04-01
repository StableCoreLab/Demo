#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_OGREDemo.h"

class OgreWidget;
class OGREDemo : public QMainWindow
{
    Q_OBJECT

public:
    OGREDemo(QWidget *parent = nullptr);
    ~OGREDemo();

private:
    Ui::OGREDemoClass ui;
    OgreWidget* m_ogreWidget = nullptr;
};

