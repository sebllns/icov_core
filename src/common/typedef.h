/** \file     typeDef.h
    \brief    Define macros, basic types, new types and enumerations
*/

#ifndef TYPEDEF_H
#define TYPEDEF_H


#ifndef COMMON_H
#error Include common.h not typedef.h
#endif


#include <vector>
#include <utility>
#include <sstream>
#include <cstddef>
#include <cstring>
#include <assert.h>
#include <cassert>

// ====================================================================================================================
// Enumeration
// ====================================================================================================================

enum ContentType
{
  CONTENT_TYPE_VIDEO        = 0,
  CONTENT_TYPE_MESH        = 1,
  NUM_CONTENT_TYPE,
};



/// chroma formats (according to how the monochrome or the color planes are intended to be coded)
enum ChromaFormat
{
  CHROMA_400        = 0,
  CHROMA_420        = 1,
  CHROMA_422        = 2,
  CHROMA_444        = 3,
  NUM_CHROMA_FORMAT = 4
};



enum ChannelType
{
  CHANNEL_TYPE_LUMA    = 0,
  CHANNEL_TYPE_CHROMA  = 1,
  MAX_NUM_CHANNEL_TYPE = 2
};

#define CH_L CHANNEL_TYPE_LUMA
#define CH_C CHANNEL_TYPE_CHROMA

enum ComponentID
{
  COMPONENT_Y         = 0,
  COMPONENT_Cb        = 1,
  COMPONENT_Cr        = 2,
  MAX_NUM_COMPONENT   = 3,
  JOINT_CbCr          = MAX_NUM_COMPONENT,
  MAX_NUM_TBLOCKS     = MAX_NUM_COMPONENT
};

#define MAP_CHROMA(c) (ComponentID(c))


enum ModeType
{
  MODE_TYPE_ALL = 0, 
  MODE_TYPE_INTER = 1, 
  MODE_TYPE_INTRA = 2, 
};


/// supported prediction type
enum PredMode
{
  MODE_INTER                 = 0,     ///< inter-prediction mode
  MODE_INTRA                 = 1,     ///< intra-prediction mode
  NUMBER_OF_PREDICTION_MODES = 3,
};

/// motion vector predictor direction used in AMVP
enum MvpDir
{
  MD_LEFT = 0,          ///< MVP of left block
  MD_ABOVE,             ///< MVP of above block
  MD_ABOVE_RIGHT,       ///< MVP of above right block
  MD_BELOW_LEFT,        ///< MVP of below left block
  MD_ABOVE_LEFT         ///< MVP of above left block
};


enum TransformDirection
{
  TRANSFORM_FORWARD              = 0,
  TRANSFORM_INVERSE              = 1,
  TRANSFORM_NUMBER_OF_DIRECTIONS = 2
};



enum NalUnitType
{
  NAL_UNIT_CODED_VPU = 0,   // 0
  NAL_UNIT_SOV,             // 1
  NAL_UNIT_CPS,             // 2
  NAL_UNIT_EOC,             // 3

  NAL_UNIT_UNSPECIFIED_001,
  NAL_UNIT_UNSPECIFIED_002,
  NAL_UNIT_UNSPECIFIED_003,
  NAL_UNIT_UNSPECIFIED_004,
  NAL_UNIT_INVALID
};

#endif


