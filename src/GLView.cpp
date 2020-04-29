#include "GLView.h"

#include <Corrade/Containers/Optional.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Trade/MeshData3D.h>

/* If your application is using anything from QtGui, you might get warnings
   about GLEW and errors regarding GLclampf. If that's the case, uncomment the
   following and place it as early as possible (but again *after* including
   Magnum GL headers) */
#if 0
typedef GLfloat GLclampf;
#undef __glew_h__ /* shh, Qt, shh */
#undef __GLEW_H__
#include <QtGui/qopenglfunctions.h>
#endif

using namespace Magnum;
using namespace Math::Literals;

GLView::GLView(Platform::GLContext& context, QWidget* parent)
    : QOpenGLWidget(parent)
    , m_context(context) {

}

void GLView::initializeGL() {
    m_context.create();

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    m_mesh = std::make_unique<GL::Mesh>(MeshTools::compile(Primitives::cubeSolid()));
    m_shader = std::make_unique<Shaders::Phong>();

    m_transformation = Matrix4::rotationX(30.0_degf)*Matrix4::rotationY(40.0_degf);
    const float aspectRatio = Vector2(width(), height()).aspectRatio();
    m_projection = Matrix4::perspectiveProjection(35.0_degf, aspectRatio, 0.01f, 100.0f) * Matrix4::translation(Vector3::zAxis(-10.0f));
    m_color = Color3::fromHsv({35.0_degf, 1.0f, 1.0f});

    /* Clean up Magnum state when giving control back to Qt */
    GL::Context::current().resetState(GL::Context::State::EnterExternal);
}

void GLView::paintGL() {
    /* Reset state to avoid Qt affecting Magnum */
    GL::Context::current().resetState(GL::Context::State::ExitExternal);

    /* Using framebuffer provided by Qt as default framebuffer */
    auto qtDefaultFramebuffer = GL::Framebuffer::wrap(defaultFramebufferObject(), {{}, {width(), height()}});

    qtDefaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    m_shader->setLightPosition({7.0f, 5.0f, 2.5f})
             .setLightColor(Color3{1.0f})
             .setDiffuseColor(m_color)
             .setAmbientColor(Color3::fromHsv({m_color.hue(), 1.0f, 0.3f}))
             .setTransformationMatrix(m_transformation)
             .setNormalMatrix(m_transformation.normalMatrix())
             .setProjectionMatrix(m_projection);
    m_mesh->draw(*m_shader);

    /* Clean up Magnum state when giving control back to Qt */
    GL::Context::current().resetState(GL::Context::State::EnterExternal);
}

void GLView::resizeGL(int w, int h)
{
    const float aspectRatio = Vector2(w, h).aspectRatio();
    m_projection = Matrix4::perspectiveProjection(35.0_degf, aspectRatio, 0.01f, 100.0f) * Matrix4::translation(Vector3::zAxis(-10.0f));
}

void GLView::mousePressEvent(QMouseEvent* event) {
    if (!event->buttons().testFlag(Qt::LeftButton))
        return;

    m_prevPos = Vector2(event->localPos().x(), event->localPos().y());
}

void GLView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        return;

    m_color = Color3::fromHsv({m_color.hue() + 50.0_degf, 1.0f, 1.0f});

    update();
}

void GLView::mouseMoveEvent(QMouseEvent* event) {
    if (!event->buttons().testFlag(Qt::LeftButton))
        return;

    Vector2 curPos(event->localPos().x(), event->localPos().y());
    Vector2 delta = 3.0f * (curPos - m_prevPos) / Vector2(width(), height());

    m_prevPos = curPos;
    m_transformation = Matrix4::rotationX(Rad{delta.y()}) * m_transformation * Matrix4::rotationY(Rad{delta.x()});

    update();
}