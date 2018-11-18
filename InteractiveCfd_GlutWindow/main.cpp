#include "Window.h"
#include "Graphics/GraphicsManager.h"

int main(int argc, char **argv)
{
    Rect<int> windowSize = Rect<int>(800, 600);
    GraphicsManager* graphicsManager = &(GraphicsManager());

    graphicsManager->SetViewport(windowSize);
    graphicsManager->SetContourVar(ContourVariable::WATER_RENDERING);

    Window::Instance().SetGraphicsManager(*graphicsManager);
    Window::Instance().RegisterCommands();

    Window::Instance().Resize(windowSize);
    Window::Instance().InitializeGlfw();
    Window::Instance().InitializeImGui();

    //graphicsManager->UseCuda(false);
    graphicsManager->SetUpGLInterop();
    graphicsManager->SetUpCuda();
    graphicsManager->SetUpShaders();

    Window::Instance().GlfwDisplay();

    return 0;
}