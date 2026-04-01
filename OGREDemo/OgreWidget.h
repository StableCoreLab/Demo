#pragma once

#include <QPoint>
#include <QTimer>
#include <QWidget>

#include <OgreAbiUtils.h>
#include <OgreCamera.h>
#include <OgreCompositorManager2.h>
#include <OgreConfigFile.h>
#include <OgreEntity.h>
#include <OgreItem.h>
#include <OgreLight.h>
#include <OgreManualObject.h>
#include <OgrePrerequisites.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreStringConverter.h>
#include <OgreWindow.h>
#include <OgreWindowEventUtilities.h>

class OgreWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OgreWidget(QWidget* parent = nullptr);
    ~OgreWidget() override;

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    QPaintEngine* paintEngine() const override
    {
        return nullptr;
    }

private slots:
    void OnRenderFrame();

private:
    void InitializeIfNeeded();
    void InitializeOgre();
    void ShutdownOgre();

    bool SelectRenderSystem();
    void SetupResources();
    void SetupCamera();
    void SetupCompositor();
    void UpdateCameraAspectRatio();
    void CreateScene();

private:
    Ogre::Root* m_root = nullptr;
    Ogre::Window* m_renderWindow = nullptr;
    Ogre::SceneManager* m_sceneManager = nullptr;
    Ogre::Camera* m_camera = nullptr;
    Ogre::CompositorWorkspace* m_workspace = nullptr;

    QTimer* m_renderTimer = nullptr;

    bool m_initialized = false;
    QPoint m_lastMousePos;
};