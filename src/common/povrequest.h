#ifndef POVREQUEST_H
#define POVREQUEST_H

#include "request.h"

class PoVRequest : public IRequest
{

public:
  PoVRequest();
  void serialize() const;
  void deserialize();



  // IRequest interface
public:
  void print();
};

#endif // POVREQUEST_H
