#include "GraphicsManager.h"
#include "Mouse.h"
#include "kernel.h"


extern Obstruction* g_obst_d;
extern int g_xDim, g_yDim;
extern Obstruction::Shape g_currentShape;
extern float g_currentSize;

GraphicsManager::GraphicsManager()
{

}

GraphicsManager::GraphicsManager(Panel* panel)
{
	m_parent = panel;
}

void GraphicsManager::GetSimCoordFromMouseCoord(int &xOut, int &yOut, Mouse mouse)
{
	float xf = intCoordToFloatCoord(mouse.m_x, mouse.m_winW);
	float yf = intCoordToFloatCoord(mouse.m_y, mouse.m_winH);
	RectFloat coordsInRelFloat = RectFloat(xf, yf, 1.f, 1.f) / m_parent->m_rectFloat_abs;
	float graphicsToSimDomainScalingFactor = static_cast<float>(g_xDim) / m_parent->m_rectInt_abs.m_w;
	xOut = floatCoordToIntCoord(coordsInRelFloat.m_x, m_parent->m_rectInt_abs.m_w)*graphicsToSimDomainScalingFactor;
	yOut = floatCoordToIntCoord(coordsInRelFloat.m_y, m_parent->m_rectInt_abs.m_h)*graphicsToSimDomainScalingFactor;
}

void GraphicsManager::GetSimCoordFromFloatCoord(int &xOut, int &yOut, float xf, float yf)
{
	RectFloat coordsInRelFloat = RectFloat(xf, yf, 1.f, 1.f) / m_parent->m_rectFloat_abs;
	float graphicsToSimDomainScalingFactor = static_cast<float>(g_xDim) / m_parent->m_rectInt_abs.m_w;
	xOut = floatCoordToIntCoord(coordsInRelFloat.m_x, m_parent->m_rectInt_abs.m_w)*graphicsToSimDomainScalingFactor;
	yOut = floatCoordToIntCoord(coordsInRelFloat.m_y, m_parent->m_rectInt_abs.m_h)*graphicsToSimDomainScalingFactor;
}

void GraphicsManager::Click(Mouse mouse)
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
	}
}

void GraphicsManager::Drag(float dx, float dy)
{
	MoveObstruction(dx, dy);
}

void GraphicsManager::AddObstruction(Mouse mouse)
{
	int xi, yi;
	GetSimCoordFromMouseCoord(xi, yi, mouse);
	Obstruction obst = { g_currentShape, xi, yi, g_currentSize, 0 };
	int obstId = FindUnusedObstructionId();
	m_obstructions[obstId] = obst;
	UpdateDeviceObstructions(g_obst_d, obstId, obst);
}


void GraphicsManager::RemoveObstruction(Mouse mouse)
{
	int obstId = FindClosestObstructionId(mouse);
	if (obstId < 0) return;
	Obstruction obst = { g_currentShape, -100, -100, 0, 0 };
	m_obstructions[obstId] = obst;
	UpdateDeviceObstructions(g_obst_d, obstId, obst);
}


void GraphicsManager::MoveObstruction(float dx, float dy)
{
	if (m_currentObstId > -1)
	{
		Obstruction obst = m_obstructions[m_currentObstId];
//		obst.x += dx*0.5f*m_parent->GetRootPanel()->m_rectInt_abs.m_w;
//		obst.y += dy*0.5f*m_parent->GetRootPanel()->m_rectInt_abs.m_h;
		int dxi, dyi;
		dxi = dx / m_parent->m_rectFloat_abs.m_w*g_xDim;
		dyi = dy / m_parent->m_rectFloat_abs.m_h*g_yDim;
		obst.x += dxi;
		obst.y += dyi;
		m_obstructions[m_currentObstId] = obst;
		UpdateDeviceObstructions(g_obst_d, m_currentObstId, obst);
	}
}

int GraphicsManager::FindUnusedObstructionId()
{
	for (int i = 0; i < MAXOBSTS; i++)
	{
		if (m_obstructions[i].y < 0)
		{
			return i;
		}
	}
	MessageBox(0, "Object could not be added. You are currently using the maximum number of objects.", "Error", MB_OK);
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
		if (m_obstructions[i].y >= 0)
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

bool GraphicsManager::IsInClosestObstruction(Mouse mouse)
{
	int closestObstId = FindClosestObstructionId(mouse);
	int xi, yi;
	GetSimCoordFromMouseCoord(xi, yi, mouse);
	return (GetDistanceBetweenTwoPoints(xi,yi,m_obstructions[closestObstId].x, m_obstructions[closestObstId].y) < m_obstructions[closestObstId].r1);
}

float GetDistanceBetweenTwoPoints(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return sqrt(dx*dx + dy*dy);
}