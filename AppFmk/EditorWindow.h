#pragma once
#include "EditorView.h"
#include <QMainWindow>
#include <RHIInstance.h>

namespace Ui
{
class EditorWindow;
}

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);
    ~EditorWindow();

protected:
    void InitializeRHI();
    void InitializeRHIResources();
    void IdleProcess();

private:
    Ui::EditorWindow* ui;
    CEditorView* EditorView;

    // RHI objects
    bool bRHIInitializes = false;
    RHI::CDevice::Ref Device;
    RHI::CSwapChain::Ref SwapChain;
    RHI::CRenderPass::Ref ScreenPass;
};
