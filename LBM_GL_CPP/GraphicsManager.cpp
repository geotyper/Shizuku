#include "GraphicsManager.h"
#include "Mouse.h"
#include "kernel.h"


extern Obstruction g_obstructions[MAXOBSTS];
extern Domain g_simDomain;
extern cudaGraphicsResource *g_cudaSolutionField;


CudaLbm::CudaLbm()
{
}

CudaLbm::CudaLbm(int maxX, int maxY)
{
    m_maxX = maxX;
    m_maxY = maxY;
}

float* CudaLbm::GetFA()
{
    return m_fA_d;
}

float* CudaLbm::GetFB()
{
    return m_fB_d;
}

int* CudaLbm::GetImage()
{
    return m_Im_d;
}

float* CudaLbm::GetFloorTemp()
{
    return m_FloorTemp_d;
}

Obstruction* CudaLbm::GetObst()
{
    return m_obst_d;
}

cudaGraphicsResource* CudaLbm::GetCudaSolutionGraphicsResource()
{
    return m_cudaSolutionGraphicsResource;
}

void CudaLbm::AllocateDeviceMemory()
{
    size_t memsize_lbm, memsize_int, memsize_float, memsize_inputs;

    int domainSize = ceil(MAX_XDIM / BLOCKSIZEX)*BLOCKSIZEX*ceil(MAX_YDIM / BLOCKSIZEY)*BLOCKSIZEY;
    memsize_lbm = domainSize*sizeof(float)*9;
    memsize_int = domainSize*sizeof(int);
    memsize_float = domainSize*sizeof(float);
    memsize_inputs = sizeof(g_obstructions);

    float* fA_h = new float[domainSize * 9];
    float* fB_h = new float[domainSize * 9];
    float* floor_h = new float[domainSize];
    int* im_h = new int[domainSize];

    cudaMalloc((void **)&m_fA_d, memsize_lbm);
    cudaMalloc((void **)&m_fB_d, memsize_lbm);
    cudaMalloc((void **)&m_FloorTemp_d, memsize_float);
    cudaMalloc((void **)&m_Im_d, memsize_int);
    cudaMalloc((void **)&m_obst_d, memsize_inputs);
}

void CudaLbm::DeallocateDeviceMemory()
{
    cudaFree(m_fA_d);
    cudaFree(m_fB_d);
    cudaFree(m_Im_d);
    cudaFree(m_FloorTemp_d);
    cudaFree(m_obst_d);
}

void CudaLbm::InitializeDeviceMemory()
{
    int domainSize = ceil(MAX_XDIM / BLOCKSIZEX)*BLOCKSIZEX*ceil(MAX_YDIM / BLOCKSIZEY)*BLOCKSIZEY;
    size_t memsize_lbm, memsize_float, memsize_inputs;
    memsize_lbm = domainSize*sizeof(float)*9;
    memsize_float = domainSize*sizeof(float);

    float* f_h = new float[domainSize*9];
    for (int i = 0; i < domainSize * 9; i++)
    {
        f_h[i] = 0;
    }
    cudaMemcpy(m_fA_d, f_h, memsize_lbm, cudaMemcpyHostToDevice);
    cudaMemcpy(m_fB_d, f_h, memsize_lbm, cudaMemcpyHostToDevice);
    delete[] f_h;
    float* floor_h = new float[domainSize];
    for (int i = 0; i < domainSize; i++)
    {
        floor_h[i] = 0;
    }
    cudaMemcpy(m_FloorTemp_d, floor_h, memsize_float, cudaMemcpyHostToDevice);
    delete[] floor_h;

    UpdateDeviceImage();

    for (int i = 0; i < MAXOBSTS; i++)
    {
        g_obstructions[i].r1 = 0;
        g_obstructions[i].x = 0;
        g_obstructions[i].y = -1000;
        g_obstructions[i].state = Obstruction::REMOVED;
    }	
    g_obstructions[0].r1 = 6.5;
    g_obstructions[0].x = 30;// g_xDim*0.2f;
    g_obstructions[0].y = 42;// g_yDim*0.3f;
    g_obstructions[0].u = 0;// g_yDim*0.3f;
    g_obstructions[0].v = 0;// g_yDim*0.3f;
    g_obstructions[0].shape = Obstruction::SQUARE;
    g_obstructions[0].state = Obstruction::NEW;

    g_obstructions[1].r1 = 4.5;
    g_obstructions[1].x = 30;// g_xDim*0.2f;
    g_obstructions[1].y = 100;// g_yDim*0.3f;
    g_obstructions[1].u = 0;// g_yDim*0.3f;
    g_obstructions[1].v = 0;// g_yDim*0.3f;
    g_obstructions[1].shape = Obstruction::VERTICAL_LINE;
    g_obstructions[1].state = Obstruction::NEW;

    memsize_inputs = sizeof(g_obstructions);
    cudaMemcpy(m_obst_d, g_obstructions, memsize_inputs, cudaMemcpyHostToDevice);
}

void CudaLbm::UpdateDeviceImage()
{
    int domainSize = ceil(MAX_XDIM / BLOCKSIZEX)*BLOCKSIZEX*ceil(MAX_YDIM / BLOCKSIZEY)*BLOCKSIZEY;
    int* im_h = new int[domainSize];
    for (int i = 0; i < domainSize; i++)
    {
        int x = i%MAX_XDIM;
        int y = i/MAX_XDIM;
        im_h[i] = ImageFcn(x, y);
    }
    size_t memsize_int = domainSize*sizeof(int);
    cudaMemcpy(m_Im_d, im_h, memsize_int, cudaMemcpyHostToDevice);
    delete[] im_h;
}

int CudaLbm::ImageFcn(const int x, const int y){
    int xDim = g_simDomain.GetXDim();
    int yDim = g_simDomain.GetYDim();
    if (x < 0.1f)
        return 3;//west
    else if ((xDim - x) < 1.1f)
        return 2;//east
    else if ((yDim - y) < 1.1f)
        return 11;//11;//xsymmetry top
    else if (y < 0.1f)
        return 12;//12;//xsymmetry bottom
    return 0;
}

void CudaLbm::SetUpGLInterOp()
{
    cudaGLSetGLDevice(gpuGetMaxGflopsDeviceId());
    GenerateIndexListForSurfaceAndFloor(m_elementArrayIndexBuffer);
    unsigned int solutionMemorySize = MAX_XDIM*MAX_YDIM * 4 * sizeof(float);
    unsigned int floorSize = MAX_XDIM*MAX_YDIM * 4 * sizeof(float);
    CreateVBO(&m_vboSolutionField, &m_cudaSolutionGraphicsResource, solutionMemorySize+floorSize,
        cudaGraphicsMapFlagsWriteDiscard);
}




GraphicsManager::GraphicsManager(Panel* panel)
{
    m_parent = panel;
}

float3 GraphicsManager::GetRotationTransforms()
{
    return float3{ m_rotate_x, m_rotate_y, m_rotate_z };
}

float3 GraphicsManager::GetTranslationTransforms()
{
    return float3{ m_translate_x, m_translate_y, m_translate_z };
}

void GraphicsManager::SetCurrentObstSize(const float size)
{
    m_currentObstSize = size;
}

void GraphicsManager::SetCurrentObstShape(const Obstruction::Shape shape)
{
    m_currentObstShape = shape;
}

ViewMode GraphicsManager::GetViewMode()
{
    return m_viewMode;
}

void GraphicsManager::SetViewMode(const ViewMode viewMode)
{
    m_viewMode = viewMode;
}

ContourVariable GraphicsManager::GetContourVar()
{
    return m_contourVar;
}

void GraphicsManager::SetContourVar(const ContourVariable contourVar)
{
    m_contourVar = contourVar;
}


Obstruction::Shape GraphicsManager::GetCurrentObstShape()
{
    return m_currentObstShape;
}

void GraphicsManager::SetObstructionsPointer(Obstruction* obst)
{
    m_obstructions = obst;
}

bool GraphicsManager::IsPaused()
{
    return m_paused;
}

void GraphicsManager::TogglePausedState()
{
    m_paused = !m_paused;
}

float GraphicsManager::GetScaleFactor()
{
    return m_scaleFactor;
}

void GraphicsManager::SetScaleFactor(const float scaleFactor)
{
    m_scaleFactor = scaleFactor;
}

CudaLbm* GraphicsManager::GetCudaLbm()
{
    return &m_cudaLbm;
}

void GraphicsManager::GetSimCoordFromMouseCoord(int &xOut, int &yOut, Mouse mouse)
{
    int xDimVisible = g_simDomain.GetXDimVisible();
    int yDimVisible = g_simDomain.GetYDimVisible();
    float xf = intCoordToFloatCoord(mouse.m_x, mouse.m_winW);
    float yf = intCoordToFloatCoord(mouse.m_y, mouse.m_winH);
    RectFloat coordsInRelFloat = RectFloat(xf, yf, 1.f, 1.f) / m_parent->GetRectFloatAbs();
    float graphicsToSimDomainScalingFactorX = static_cast<float>(xDimVisible) /
        std::min(static_cast<float>(m_parent->GetRectIntAbs().m_w), MAX_XDIM*m_scaleFactor);
    float graphicsToSimDomainScalingFactorY = static_cast<float>(yDimVisible) /
        std::min(static_cast<float>(m_parent->GetRectIntAbs().m_h), MAX_YDIM*m_scaleFactor);
    xOut = floatCoordToIntCoord(coordsInRelFloat.m_x, m_parent->GetRectIntAbs().m_w)*
        graphicsToSimDomainScalingFactorX;
    yOut = floatCoordToIntCoord(coordsInRelFloat.m_y, m_parent->GetRectIntAbs().m_h)*
        graphicsToSimDomainScalingFactorY;
}

void GraphicsManager::GetSimCoordFromFloatCoord(int &xOut, int &yOut, 
    const float xf, const float yf)
{
    int xDimVisible = g_simDomain.GetXDimVisible();
    int yDimVisible = g_simDomain.GetYDimVisible();
    RectFloat coordsInRelFloat = RectFloat(xf, yf, 1.f, 1.f) / m_parent->GetRectFloatAbs();
    float graphicsToSimDomainScalingFactorX = static_cast<float>(xDimVisible) /
        std::min(static_cast<float>(m_parent->GetRectIntAbs().m_w), MAX_XDIM*m_scaleFactor);
    float graphicsToSimDomainScalingFactorY = static_cast<float>(yDimVisible) /
        std::min(static_cast<float>(m_parent->GetRectIntAbs().m_h), MAX_YDIM*m_scaleFactor);
    xOut = floatCoordToIntCoord(coordsInRelFloat.m_x, m_parent->GetRectIntAbs().m_w)*
        graphicsToSimDomainScalingFactorX;
    yOut = floatCoordToIntCoord(coordsInRelFloat.m_y, m_parent->GetRectIntAbs().m_h)*
        graphicsToSimDomainScalingFactorY;
}

void GraphicsManager::GetMouseRay(float3 &rayOrigin, float3 &rayDir,
    const int mouseX, const int mouseY)
{
    double x, y, z;
    gluUnProject(mouseX, mouseY, 0.0f, m_modelMatrix, m_projectionMatrix, m_viewport, &x, &y, &z);
    //printf("Origin: %f, %f, %f\n", x, y, z);
    rayOrigin.x = x;
    rayOrigin.y = y;
    rayOrigin.z = z;
    gluUnProject(mouseX, mouseY, 1.0f, m_modelMatrix, m_projectionMatrix, m_viewport, &x, &y, &z);
    rayDir.x = x-rayOrigin.x;
    rayDir.y = y-rayOrigin.y;
    rayDir.z = z-rayOrigin.z;
    float mag = sqrt(rayDir.x*rayDir.x + rayDir.y*rayDir.y + rayDir.z*rayDir.z);
    rayDir.x /= mag;
    rayDir.y /= mag;
    rayDir.z /= mag;
}

int GraphicsManager::GetSimCoordFrom3DMouseClickOnObstruction(int &xOut, int &yOut,
    Mouse mouse)
{
    int xDimVisible = g_simDomain.GetXDimVisible();
    int yDimVisible = g_simDomain.GetYDimVisible();
    float3 rayOrigin, rayDir;
    GetMouseRay(rayOrigin, rayDir, mouse.GetX(), mouse.GetY());
    int returnVal = 0;

    // map OpenGL buffer object for writing from CUDA
    float4 *dptr;
    cudaGraphicsMapResources(1, &g_cudaSolutionField, 0);
    size_t num_bytes;
    cudaGraphicsResourceGetMappedPointer((void **)&dptr, &num_bytes, g_cudaSolutionField);

    float3 selectedCoordF;
    Obstruction* obst_d = GetCudaLbm()->GetObst();
    int rayCastResult = RayCastMouseClick(selectedCoordF, dptr, m_rayCastIntersect_d, 
        rayOrigin, rayDir, obst_d, g_simDomain);
    if (rayCastResult == 0)
    {
        m_currentZ = selectedCoordF.z;

        xOut = (selectedCoordF.x + 1.f)*0.5f*xDimVisible;
        yOut = (selectedCoordF.y + 1.f)*0.5f*xDimVisible;
    }
    else
    {
        returnVal = 1;
    }

    cudaGraphicsUnmapResources(1, &g_cudaSolutionField, 0);

    return returnVal;
}

// get simulation coordinates from mouse ray casting on plane on m_currentZ
void GraphicsManager::GetSimCoordFrom2DMouseRay(int &xOut, int &yOut, Mouse mouse)
{
    int xDimVisible = g_simDomain.GetXDimVisible();
    float3 rayOrigin, rayDir;
    GetMouseRay(rayOrigin, rayDir, mouse.GetX(), mouse.GetY());

    float t = (m_currentZ - rayOrigin.z)/rayDir.z;
    float xf = rayOrigin.x + t*rayDir.x;
    float yf = rayOrigin.y + t*rayDir.y;

    xOut = (xf + 1.f)*0.5f*xDimVisible;
    yOut = (yf + 1.f)*0.5f*xDimVisible;
}

void GraphicsManager::GetSimCoordFrom2DMouseRay(int &xOut, int &yOut, 
    const int mouseX, const int mouseY)
{
    int xDimVisible = g_simDomain.GetXDimVisible();
    float3 rayOrigin, rayDir;
    GetMouseRay(rayOrigin, rayDir, mouseX, mouseY);

    float t = (m_currentZ - rayOrigin.z)/rayDir.z;
    float xf = rayOrigin.x + t*rayDir.x;
    float yf = rayOrigin.y + t*rayDir.y;

    xOut = (xf + 1.f)*0.5f*xDimVisible;
    yOut = (yf + 1.f)*0.5f*xDimVisible;
}

void GraphicsManager::ClickDown(Mouse mouse)
{
    int mod = glutGetModifiers();
    if (m_viewMode == ViewMode::TWO_DIMENSIONAL)
    {
        if (mouse.m_rmb == 1)
        {
            AddObstruction(mouse);
        }
        else if (mouse.m_mmb == 1)
        {
            if (IsInClosestObstruction(mouse))
            {
                RemoveObstruction(mouse);
            }
        }
        else if (mouse.m_lmb == 1)
        {
            if (IsInClosestObstruction(mouse))
            {
                m_currentObstId = FindClosestObstructionId(mouse);
            }
            else
            {
                m_currentObstId = -1;
            }
        }
    }
    else
    {
        if (mouse.m_lmb == 1)
        {
            m_currentObstId = -1;
            int x{ 0 }, y{ 0 }, z{ 0 };
            if (GetSimCoordFrom3DMouseClickOnObstruction(x, y, mouse) == 0)
            {
                m_currentObstId = FindClosestObstructionId(x, y);
            }
        }
        else if (mouse.m_rmb == 1)
        {
            m_currentObstId = -1;
            int x{ 0 }, y{ 0 }, z{ 0 };
            m_currentZ = -0.5f;
            GetSimCoordFrom2DMouseRay(x, y, mouse);
            AddObstruction(x,y);
        }
        else if (mouse.m_mmb == 1 && mod != GLUT_ACTIVE_CTRL)
        {
            m_currentObstId = -1;
            int x{ 0 }, y{ 0 }, z{ 0 };
            if (GetSimCoordFrom3DMouseClickOnObstruction(x, y, mouse) == 0)
            {            
                m_currentObstId = FindClosestObstructionId(x, y);
                RemoveObstruction(x,y);
            }
    
        }
    }
}

void GraphicsManager::Drag(const int xi, const int yi, const float dxf,
    const float dyf, const int button)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        MoveObstruction(xi,yi,dxf,dyf);
    }
    else if (button == GLUT_MIDDLE_BUTTON)
    {
        int mod = glutGetModifiers();
        if (mod == GLUT_ACTIVE_CTRL)
        {
            m_translate_x += dxf;
            m_translate_y += dyf;
        }
        else
        {
            m_rotate_x += dyf*45.f;
            m_rotate_z += dxf*45.f;
        }

    }
}

void GraphicsManager::Wheel(const int button, const int dir, const int x, const int y)
{
    if (dir > 0){
        m_translate_z -= 0.3f;
    }
    else
    {
        m_translate_z += 0.3f;
    }
}

void GraphicsManager::AddObstruction(Mouse mouse)
{
    int xi, yi;
    GetSimCoordFromMouseCoord(xi, yi, mouse);
    Obstruction obst = { m_currentObstShape, xi, yi, m_currentObstSize, 0, 0, 0, Obstruction::NEW };
    int obstId = FindUnusedObstructionId();
    m_obstructions[obstId] = obst;
    Obstruction* obst_d = GetCudaLbm()->GetObst();
    UpdateDeviceObstructions(obst_d, obstId, obst);
}

void GraphicsManager::AddObstruction(const int simX, const int simY)
{
    Obstruction obst = { m_currentObstShape, simX, simY, m_currentObstSize, 0, 0, 0, Obstruction::NEW  };
    int obstId = FindUnusedObstructionId();
    m_obstructions[obstId] = obst;
    Obstruction* obst_d = GetCudaLbm()->GetObst();
    UpdateDeviceObstructions(obst_d, obstId, obst);
}

void GraphicsManager::RemoveObstruction(Mouse mouse)
{
    int obstId = FindClosestObstructionId(mouse);
    if (obstId < 0) return;
    //Obstruction obst = m_obstructions[obstId];// { g_currentShape, -100, -100, 0, 0, Obstruction::NEW };
    m_obstructions[obstId].state = Obstruction::REMOVED;
    Obstruction* obst_d = GetCudaLbm()->GetObst();
    UpdateDeviceObstructions(obst_d, obstId, m_obstructions[obstId]);
}

void GraphicsManager::RemoveObstruction(const int simX, const int simY)
{
    int obstId = FindObstructionPointIsInside(simX,simY,1.f);
    if (obstId >= 0)
    {
        m_obstructions[obstId].state = Obstruction::REMOVED;
        Obstruction* obst_d = GetCudaLbm()->GetObst();
        UpdateDeviceObstructions(obst_d, obstId, m_obstructions[obstId]);
    }
}

void GraphicsManager::MoveObstruction(const int xi, const int yi,
    const float dxf, const float dyf)
{
    int xDimVisible = g_simDomain.GetXDimVisible();
    int yDimVisible = g_simDomain.GetYDimVisible();
    if (m_currentObstId > -1)
    {
        if (m_viewMode == ViewMode::TWO_DIMENSIONAL)
        {
            Obstruction obst = m_obstructions[m_currentObstId];
            float dxi, dyi;
            int windowWidth = m_parent->GetRootPanel()->GetRectIntAbs().m_w;
            int windowHeight = m_parent->GetRootPanel()->GetRectIntAbs().m_h;
            dxi = dxf*static_cast<float>(xDimVisible) / 
                std::min(m_parent->GetRectFloatAbs().m_w, xDimVisible*m_scaleFactor/windowWidth*2.f);
            dyi = dyf*static_cast<float>(yDimVisible) / 
                std::min(m_parent->GetRectFloatAbs().m_h, yDimVisible*m_scaleFactor/windowHeight*2.f);
            obst.x += dxi;
            obst.y += dyi;
            float u = std::max(-0.1f,std::min(0.1f,static_cast<float>(dxi) / (TIMESTEPS_PER_FRAME)));
            float v = std::max(-0.1f,std::min(0.1f,static_cast<float>(dyi) / (TIMESTEPS_PER_FRAME)));
            obst.u = u;
            obst.v = v;
            m_obstructions[m_currentObstId] = obst;
            Obstruction* obst_d = GetCudaLbm()->GetObst();
            UpdateDeviceObstructions(obst_d, m_currentObstId, obst);
        }
        else
        {
            int simX1, simY1, simX2, simY2;
            int dxi, dyi;
            int windowWidth = m_parent->GetRootPanel()->GetRectIntAbs().m_w;
            int windowHeight = m_parent->GetRootPanel()->GetRectIntAbs().m_h;
            dxi = dxf*static_cast<float>(windowWidth) / 2.f;
            dyi = dyf*static_cast<float>(windowHeight) / 2.f;
            GetSimCoordFrom2DMouseRay(simX1, simY1, xi-dxi, yi-dyi);
            GetSimCoordFrom2DMouseRay(simX2, simY2, xi, yi);
            Obstruction obst = m_obstructions[m_currentObstId];
            obst.x += simX2-simX1;
            obst.y += simY2-simY1;
            float u = std::max(-0.1f,std::min(0.1f,static_cast<float>(simX2-simX1) / (TIMESTEPS_PER_FRAME)));
            float v = std::max(-0.1f,std::min(0.1f,static_cast<float>(simY2-simY1) / (TIMESTEPS_PER_FRAME)));
            obst.u = u;
            obst.v = v;
            obst.state = Obstruction::ACTIVE;
            m_obstructions[m_currentObstId] = obst;
            Obstruction* obst_d = GetCudaLbm()->GetObst();
            UpdateDeviceObstructions(obst_d, m_currentObstId, obst);
        }
    }
}

int GraphicsManager::FindUnusedObstructionId()
{
    for (int i = 0; i < MAXOBSTS; i++)
    {
        if (m_obstructions[i].state == Obstruction::REMOVED || 
            m_obstructions[i].state == Obstruction::INACTIVE)
        {
            return i;
        }
    }
    MessageBox(0, "Object could not be added. You are currently using the maximum number of objects.",
        "Error", MB_OK);
    return 0;
}

int GraphicsManager::FindClosestObstructionId(Mouse mouse)
{
    int xi, yi;
    GetSimCoordFromMouseCoord(xi, yi, mouse);
    float dist = 999999999999.f;
    int closestObstId = -1;
    for (int i = 0; i < MAXOBSTS; i++)
    {
        if (m_obstructions[i].state != Obstruction::REMOVED)
        {
            float newDist = GetDistanceBetweenTwoPoints(xi, yi, m_obstructions[i].x, m_obstructions[i].y);
            if (newDist < dist)
            {
                dist = newDist;
                closestObstId = i;
            }
        }
    }
    return closestObstId;
}

int GraphicsManager::FindClosestObstructionId(const int simX, const int simY)
{
    float dist = 999999999999.f;
    int closestObstId = -1;
    for (int i = 0; i < MAXOBSTS; i++)
    {
        if (m_obstructions[i].state != Obstruction::REMOVED)
        {
            float newDist = GetDistanceBetweenTwoPoints(simX, simY, m_obstructions[i].x, m_obstructions[i].y);
            if (newDist < dist)
            {
                dist = newDist;
                closestObstId = i;
            }
        }
    }
    return closestObstId;
}

int GraphicsManager::FindObstructionPointIsInside(const int simX, const int simY,
    const float tolerance)
{
    float dist = 999999999999.f;
    int closestObstId = -1;
    for (int i = 0; i < MAXOBSTS; i++)
    {
        if (m_obstructions[i].state != Obstruction::REMOVED)
        {
            float newDist = GetDistanceBetweenTwoPoints(simX, simY, m_obstructions[i].x,
                m_obstructions[i].y);
            if (newDist < dist && newDist < m_obstructions[i].r1+tolerance)
            {
                dist = newDist;
                closestObstId = i;
            }
        }
    }
    //printf("closest obst: %i", closestObstId);
    return closestObstId;
}

bool GraphicsManager::IsInClosestObstruction(Mouse mouse)
{
    int closestObstId = FindClosestObstructionId(mouse);
    int xi, yi;
    GetSimCoordFromMouseCoord(xi, yi, mouse);
    float dist = GetDistanceBetweenTwoPoints(xi, yi, m_obstructions[closestObstId].x, 
        m_obstructions[closestObstId].y);
    return (dist < m_obstructions[closestObstId].r1);
}

void GraphicsManager::UpdateViewTransformations()
{
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, m_modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, m_projectionMatrix);
}

float GetDistanceBetweenTwoPoints(const float x1, const float y1,
    const float x2, const float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx*dx + dy*dy);
}