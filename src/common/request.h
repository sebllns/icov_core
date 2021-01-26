#ifndef IREQUEST_H
#define IREQUEST_H

#include "common.h"

class IRequest
{
protected:
  float m_angleY;
  float m_angleX;
public:
  virtual void serialize() const  = 0;
  virtual void deserialize()  = 0;
  virtual void print()  = 0;

  void setAngleX(float value);
  void setAngleY(float value);

  float getAngleX() const;
  float getAngleY() const;

  virtual ~IRequest(){}
};

#endif // IREQUEST_H
