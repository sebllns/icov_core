#include "povrequest.h"

PoVRequest::PoVRequest()
{

}

void PoVRequest::serialize() const
{
  cout<<"PoVRequest::serialize()" <<endl;
  cout<<m_angleX <<", " <<m_angleY <<endl;
}

void PoVRequest::deserialize()
{
  cout<<"PoVRequest::deserialize()" <<endl;
  cout<<m_angleX <<", " <<m_angleY <<endl;
}


void PoVRequest::print()
{
  cout << m_angleX << ", " << m_angleY << endl;
}


