#include "EditorWindow.h"
#include "ui_EditorWindow.h"

#include <QTimer>
#include <RHIInstance.h>
#include <PresentationSurfaceDesc.h>

#include <memory>

//The main function is embedded in here for now
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    auto editorWindow = std::make_unique<EditorWindow>();
    editorWindow->show();
    return app.exec();
}

EditorWindow::EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EditorWindow)
{
    ui->setupUi(this);

    // Wire up common actions
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, QApplication::aboutQt);

    // Create the editor graphics view
    EditorView = new CEditorView();
    QWidget* editorViewContainer = QWidget::createWindowContainer(EditorView);
    editorViewContainer->setFocusPolicy(Qt::TabFocus);
    ui->centralLayout->addWidget(editorViewContainer);

    // Setup idle processing loop
    QTimer* idleTimer = new QTimer(this);
    connect(idleTimer, &QTimer::timeout, this, &EditorWindow::IdleProcess);
    idleTimer->start();
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::InitializeRHI()
{
    using namespace RHI;
    Device = RHI::CInstance::Get().CreateDevice(EDeviceCreateHints::NoHint);
}

void EditorWindow::InitializeRHIResources()
{
    RHI::CPresentationSurfaceDesc surfaceDesc;
    surfaceDesc.Type = RHI::EPresentationSurfaceDescType::MacOS;
    surfaceDesc.MacOS.View = reinterpret_cast<const void*>(EditorView->winId());
    SwapChain = Device->CreateSwapChain(surfaceDesc, RHI::EFormat::R8G8B8A8_UNORM);

    RHI::CImageViewDesc desc;
    desc.Type = RHI::EImageViewType::View2D;
    desc.Format = RHI::EFormat::R8G8B8A8_UNORM;
    desc.Range.Set(0, 1, 0, 1);
    auto swapChainView = Device->CreateImageView(desc, SwapChain->GetImage());

    RHI::CRenderPassDesc screenPassDesc;
    screenPassDesc.Width = EditorView->width();
    screenPassDesc.Height = EditorView->height();
    screenPassDesc.Layers = 1;
    screenPassDesc.Attachments.emplace_back(swapChainView, RHI::CAttachmentLoadOp::Clear, RHI::CAttachmentStoreOp::Store);
    screenPassDesc.Subpasses.resize(1);
    screenPassDesc.Subpasses[0].AddColorAttachment(0);
    ScreenPass = Device->CreateRenderPass(screenPassDesc);
}

void EditorWindow::IdleProcess()
{
    if (!bRHIInitializes)
    {
        InitializeRHI();
        InitializeRHIResources();
        bRHIInitializes = true;
    }

    SwapChain->AcquireNextImage();
    auto ctx = Device->GetImmediateContext();
    ctx->BeginRenderPass(ScreenPass, { RHI::CClearValue(1.0f, 0.0f, 0.0f, 0.0f)});
    ctx->EndRenderPass();

    RHI::CSwapChainPresentInfo presentInfo;
    presentInfo.SrcImage = nullptr;
    SwapChain->Present(presentInfo);
}
