#include "request.h"
void IRequest::setAngleX(float value)
{
  m_angleX = value;
}

void IRequest::setAngleY(float value)
{
  m_angleY = value;
}

float IRequest::getAngleX() const
{
  return m_angleX;
}

float IRequest::getAngleY() const
{
  return m_angleY;
}
