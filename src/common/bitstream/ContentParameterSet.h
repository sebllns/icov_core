/** \file     ContentParameterSet.h
    \brief    ContentParameterSet
*/

#ifndef CONTENTPARAMETERSET_H
#define CONTENTPARAMETERSET_H

#include "common.h"

class ContentParameterSet
{
protected:
  ContentType m_content_type;
  int m_content_width;
  int m_content_height;
  float m_content_framerate;
  float m_content_duration;

  bool m_has_metadata;
  //metadata as class

  bool m_is_sov_fixed_size;
  int m_sov_fixed_size;


public:
  ContentParameterSet();
};

#endif // CONTENTPARAMETERSET_H
