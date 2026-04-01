#include "OgreWidget.h"

#include <Ogre.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreMovableObject.h>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QWheelEvent>
#include <QDebug>

#include <algorithm>
#include <stdexcept>

namespace {
    const Ogre::String kWorkspaceName = "QtOgreWorkspace";
}

OgreWidget::OgreWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &OgreWidget::OnRenderFrame);
}

OgreWidget::~OgreWidget()
{
    ShutdownOgre();
}

void OgreWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    InitializeIfNeeded();
}

void OgreWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (!m_renderWindow || !m_camera)
        return;

    const Ogre::uint32 w = static_cast<Ogre::uint32>(std::max(1, width()));
    const Ogre::uint32 h = static_cast<Ogre::uint32>(std::max(1, height()));

    m_renderWindow->requestResolution(w, h);
    m_renderWindow->windowMovedOrResized();

    UpdateCameraAspectRatio();
}

void OgreWidget::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    QWidget::mousePressEvent(event);
}

void OgreWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_camera) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
        const float yawDegree = -delta.x() * 0.2f;
        const float pitchDegree = -delta.y() * 0.2f;

        m_camera->yaw(Ogre::Degree(yawDegree));
        m_camera->pitch(Ogre::Degree(pitchDegree));
    } else if (event->buttons() & Qt::RightButton) {
        const float moveScale = 0.2f;
        m_camera->moveRelative(
            Ogre::Vector3(-delta.x() * moveScale, delta.y() * moveScale, 0.0f));
    }

    QWidget::mouseMoveEvent(event);
}

void OgreWidget::wheelEvent(QWheelEvent* event)
{
    if (m_camera) {
        const QPoint numDegrees = event->angleDelta() / 8;
        const QPoint numSteps = numDegrees / 15;

        const float zoomScale = 10.0f;
        m_camera->moveRelative(
            Ogre::Vector3(0.0f, 0.0f, -numSteps.y() * zoomScale));
    }

    QWidget::wheelEvent(event);
}

void OgreWidget::keyPressEvent(QKeyEvent* event)
{
    if (!m_camera) {
        QWidget::keyPressEvent(event);
        return;
    }

    const float step = 5.0f;

    switch (event->key()) {
        case Qt::Key_W:
            m_camera->moveRelative(Ogre::Vector3(0.0f, 0.0f, -step));
            break;
        case Qt::Key_S:
            m_camera->moveRelative(Ogre::Vector3(0.0f, 0.0f, step));
            break;
        case Qt::Key_A:
            m_camera->moveRelative(Ogre::Vector3(-step, 0.0f, 0.0f));
            break;
        case Qt::Key_D:
            m_camera->moveRelative(Ogre::Vector3(step, 0.0f, 0.0f));
            break;
        case Qt::Key_Q:
            m_camera->moveRelative(Ogre::Vector3(0.0f, step, 0.0f));
            break;
        case Qt::Key_E:
            m_camera->moveRelative(Ogre::Vector3(0.0f, -step, 0.0f));
            break;
        default:
            break;
    }

    QWidget::keyPressEvent(event);
}

void OgreWidget::OnRenderFrame()
{
    if (!m_initialized || !m_root || !m_renderWindow || isMinimized())
        return;

    Ogre::WindowEventUtilities::messagePump();
    m_root->renderOneFrame();
}

void OgreWidget::InitializeIfNeeded()
{
    if (m_initialized)
        return;

    try {
        InitializeOgre();
        m_initialized = true;
        m_renderTimer->start(16);
    } catch (const std::exception& ex) {
        qCritical() << "Initialize OGRE-Next failed:" << ex.what();
    } catch (...) {
        qCritical() << "Initialize OGRE-Next failed: unknown exception";
    }
}

void OgreWidget::InitializeOgre()
{
    const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();

    // 推荐先用 plugins.cfg，避免手工 loadPlugin 的路径/后缀问题
    m_root = new Ogre::Root(&abiCookie, "plugins.cfg", "", "OgreQt.log");

    if (!SelectRenderSystem())
        throw std::runtime_error("No available OGRE-Next render system found.");

    m_root->initialise(false);

    Ogre::NameValuePairList params;

#ifdef _WIN32
    const size_t nativeHandle = static_cast<size_t>(winId());
    params["externalWindowHandle"] =
        Ogre::StringConverter::toString(nativeHandle);
#endif

    const Ogre::uint32 renderWidth =
        static_cast<Ogre::uint32>(std::max(1, width()));
    const Ogre::uint32 renderHeight =
        static_cast<Ogre::uint32>(std::max(1, height()));

    m_renderWindow = m_root->createRenderWindow("OgreInQtWidget", renderWidth,
                                                renderHeight, false, &params);

    if (!m_renderWindow)
        throw std::runtime_error("Failed to create OGRE-Next Window.");

    SetupResources();

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(
        false);

    const size_t numThreads = 1u;
    m_sceneManager = m_root->createSceneManager(Ogre::ST_INTERIOR, numThreads,
                                                "MainSceneManager");

    if (!m_sceneManager)
        throw std::runtime_error("Failed to create SceneManager.");

    SetupCamera();
    SetupCompositor();
    CreateScene();
}

void OgreWidget::ShutdownOgre()
{
    if (m_renderTimer)
        m_renderTimer->stop();

    m_workspace = nullptr;
    m_camera = nullptr;
    m_sceneManager = nullptr;
    m_renderWindow = nullptr;

    delete m_root;
    m_root = nullptr;

    m_initialized = false;
}

bool OgreWidget::SelectRenderSystem()
{
    if (!m_root)
        return false;

    const Ogre::RenderSystemList& renderers = m_root->getAvailableRenderers();
    if (renderers.empty())
        return false;

    Ogre::RenderSystem* selected = nullptr;

    for (auto* rs : renderers) {
        const Ogre::String name = rs->getName();

        if (name.find("Direct3D11") != Ogre::String::npos ||
            name.find("Direct3D 11") != Ogre::String::npos) {
            selected = rs;
            break;
        }
    }

    if (!selected) {
        for (auto* rs : renderers) {
            const Ogre::String name = rs->getName();

            if (name.find("Vulkan") != Ogre::String::npos ||
                name.find("OpenGL") != Ogre::String::npos ||
                name.find("GL3Plus") != Ogre::String::npos) {
                selected = rs;
                break;
            }
        }
    }

    if (!selected)
        selected = renderers.front();

    if (!selected)
        return false;

    m_root->setRenderSystem(selected);
    return true;
}

void OgreWidget::SetupResources()
{
    auto& rgm = Ogre::ResourceGroupManager::getSingleton();

    try {
        rgm.addResourceLocation("./media", "FileSystem", "General");
        rgm.addResourceLocation("./media/models", "FileSystem", "General");
        rgm.addResourceLocation("./media/materials", "FileSystem", "General");
        rgm.addResourceLocation("./media/materials/scripts", "FileSystem",
                                "General");
        rgm.addResourceLocation("./media/materials/textures", "FileSystem",
                                "General");
    } catch (...) {
        // 资源目录先不强制要求存在
    }
}

void OgreWidget::SetupCamera()
{
    m_camera = m_sceneManager->createCamera("MainCamera");
    if (!m_camera)
        throw std::runtime_error("Failed to create camera.");

    m_camera->setNearClipDistance(0.2f);
    m_camera->setFarClipDistance(10000.0f);
    m_camera->setAutoAspectRatio(false);

    m_camera->setPosition(0.0f, 5.0f, 15.0f);
    m_camera->lookAt(Ogre::Vector3(0.0f, 0.0f, 0.0f));

    UpdateCameraAspectRatio();
}

void OgreWidget::SetupCompositor()
{
    Ogre::CompositorManager2* compositorManager =
        m_root->getCompositorManager2();
    if (!compositorManager)
        throw std::runtime_error("Failed to get CompositorManager2.");

    const Ogre::ColourValue backgroundColour(0.15f, 0.18f, 0.22f);

    if (!compositorManager->hasWorkspaceDefinition(kWorkspaceName)) {
        compositorManager->createBasicWorkspaceDef(
            kWorkspaceName, backgroundColour, Ogre::IdString());
    }

    m_workspace = compositorManager->addWorkspace(
        m_sceneManager, m_renderWindow->getTexture(), m_camera, kWorkspaceName,
        true);

    if (!m_workspace)
        throw std::runtime_error("Failed to create compositor workspace.");
}

void OgreWidget::UpdateCameraAspectRatio()
{
    if (!m_camera || !m_renderWindow)
        return;

    const unsigned int w = std::max(1u, m_renderWindow->getWidth());
    const unsigned int h = std::max(1u, m_renderWindow->getHeight());

    m_camera->setAspectRatio(static_cast<Ogre::Real>(w) /
                             static_cast<Ogre::Real>(h));
}

void OgreWidget::CreateScene()
{
    m_sceneManager->setAmbientLight(Ogre::ColourValue(0.4f, 0.4f, 0.4f),
                                    Ogre::ColourValue(0.2f, 0.2f, 0.2f),
                                    Ogre::Vector3::UNIT_Y);

    Ogre::Light* light = m_sceneManager->createLight();
    light->setType(Ogre::Light::LT_POINT);

    Ogre::SceneNode* lightNode =
        m_sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)
            ->createChildSceneNode(Ogre::SCENE_DYNAMIC);

    lightNode->setPosition(20.0f, 80.0f, 50.0f);
    lightNode->attachObject(light);

    // Create a simple cube using ManualObject
    Ogre::v1::ManualObject* manual = m_sceneManager->createManualObject();
    manual->begin("BaseWhite", Ogre::OT_TRIANGLE_LIST);

    // Define vertices for a cube
    manual->position(-1, -1, 1);
    manual->normal(0, 0, 1);
    manual->position(1, -1, 1);
    manual->normal(0, 0, 1);
    manual->position(1, 1, 1);
    manual->normal(0, 0, 1);
    manual->position(-1, 1, 1);
    manual->normal(0, 0, 1);

    manual->position(-1, -1, -1);
    manual->normal(0, 0, -1);
    manual->position(-1, 1, -1);
    manual->normal(0, 0, -1);
    manual->position(1, 1, -1);
    manual->normal(0, 0, -1);
    manual->position(1, -1, -1);
    manual->normal(0, 0, -1);

    manual->position(-1, 1, -1);
    manual->normal(0, 1, 0);
    manual->position(-1, 1, 1);
    manual->normal(0, 1, 0);
    manual->position(1, 1, 1);
    manual->normal(0, 1, 0);
    manual->position(1, 1, -1);
    manual->normal(0, 1, 0);

    manual->position(-1, -1, -1);
    manual->normal(0, -1, 0);
    manual->position(1, -1, -1);
    manual->normal(0, -1, 0);
    manual->position(1, -1, 1);
    manual->normal(0, -1, 0);
    manual->position(-1, -1, 1);
    manual->normal(0, -1, 0);

    manual->position(1, -1, -1);
    manual->normal(1, 0, 0);
    manual->position(1, 1, -1);
    manual->normal(1, 0, 0);
    manual->position(1, 1, 1);
    manual->normal(1, 0, 0);
    manual->position(1, -1, 1);
    manual->normal(1, 0, 0);

    manual->position(-1, -1, -1);
    manual->normal(-1, 0, 0);
    manual->position(-1, -1, 1);
    manual->normal(-1, 0, 0);
    manual->position(-1, 1, 1);
    manual->normal(-1, 0, 0);
    manual->position(-1, 1, -1);
    manual->normal(-1, 0, 0);

    // Define triangles
    // Front face
    manual->triangle(0, 1, 2);
    manual->triangle(2, 3, 0);

    // Back face
    manual->triangle(4, 5, 6);
    manual->triangle(6, 7, 4);

    // Top face
    manual->triangle(8, 9, 10);
    manual->triangle(10, 11, 8);

    // Bottom face
    manual->triangle(12, 13, 14);
    manual->triangle(14, 15, 12);

    // Right face
    manual->triangle(16, 17, 18);
    manual->triangle(18, 19, 16);

    // Left face
    manual->triangle(20, 21, 22);
    manual->triangle(22, 23, 20);

    manual->end();

    Ogre::SceneNode* node =
        m_sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)
            ->createChildSceneNode(Ogre::SCENE_DYNAMIC);

    node->attachObject(manual);
}