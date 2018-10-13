#include "Window.h"
#include "Layout.h"
#include "Panel/Panel.h"
#include "Graphics/GraphicsManager.h"

int main(int argc, char **argv)
{
    Panel* windowPanel = Window::Instance().GetWindowPanel();

    Layout::SetUpWindow(*windowPanel);
    GraphicsManager* graphicsManager = windowPanel->GetPanel("Graphics")->GetGraphicsManager();

    bool glfw = true;
    if (glfw)
    {
        Window::Instance().InitializeGlfw(argc, argv);

        //graphicsManager->UseCuda(false);
        graphicsManager->SetUpGLInterop();
        graphicsManager->SetUpCuda();
        graphicsManager->SetUpShaders();

        Window::Instance().GlfwDisplay();
    }
    else{
        Window::Instance().InitializeGLUT(argc, argv);
        Window::Instance().InitializeGL();

        //graphicsManager->UseCuda(false);
        graphicsManager->SetUpGLInterop();
        graphicsManager->SetUpCuda();
        graphicsManager->SetUpShaders();

        Window::Instance().Display();
    }

    return 0;
}