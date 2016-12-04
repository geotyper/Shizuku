#include "Mouse.h"

void Mouse::SetBasePanel(Panel* basePanel)
{
	m_basePanel = basePanel;
	m_winW = m_basePanel->m_rectInt_abs.m_w;
	m_winH = m_basePanel->m_rectInt_abs.m_h;
	m_x = 0;
	m_y = 0;
}

void Mouse::Update(int x, int y, int button, int state)
{
	m_x = x;
	m_y = y;
	int mouseState = (state == GLUT_DOWN) ? 1 : 0;
	m_lmb = (button == GLUT_LEFT_BUTTON) ? mouseState : m_lmb;
	m_rmb = (button == GLUT_RIGHT_BUTTON) ? mouseState : m_rmb;
	m_mmb = (button == GLUT_MIDDLE_BUTTON) ? mouseState : m_mmb;
}
void Mouse::Update(int x, int y)
{
	m_x = x;
	m_y = y;
}
void Mouse::GetChange(int x, int y)
{
	m_xprev = x;
	m_yprev = y;
}
int Mouse::GetX()
{
	return m_x;
}

int Mouse::GetY()
{
	return m_y;
}

void Mouse::Move(int x, int y)
{
	float dx = intCoordToFloatCoord(x, m_winW) - intCoordToFloatCoord(m_x, m_winW);
	float dy = intCoordToFloatCoord(y, m_winH) - intCoordToFloatCoord(m_y, m_winH);
	Update(x, y);

	if (m_lmb == 1)
	{
		m_currentlySelectedPanel->Drag(dx, dy);
	}

}

void Mouse::Click(int x, int y, int button, int state)
{
	Update(x, y, button, state);
	m_currentlySelectedPanel = GetPanelThatPointIsIn(m_basePanel, intCoordToFloatCoord(x, m_winW), intCoordToFloatCoord(y, m_winH));
	if (m_currentlySelectedPanel != NULL)
	{
		m_currentlySelectedPanel->Click(*this);
	}
}

void Mouse::LeftClickDown(int x, int y)
{

}




float intCoordToFloatCoord(int x, int xDim)
{
	return (static_cast<float> (x) / xDim)*2.f - 1.f;
}

int floatCoordToIntCoord(float x, int xDim)
{
	return static_cast<int> ((x+1.f)/2.f*xDim);
}
