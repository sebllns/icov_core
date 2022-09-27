/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "ICOV_IntraPrediction.h"
#include "vtm_utility.h"

void icov::video::IntraPrediction::destroy() {
  for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++) {
    for (UInt buf = 0; buf < NUM_PRED_BUF; buf++) {
      delete[] m_piYuvExt[ch][buf];
      m_piYuvExt[ch][buf] = nullptr;
    }
  }
}

void icov::video::IntraPrediction::xFillReferenceSamples(
    const PelUnitBuf &recoUnitBuf,
    const BoolUnitBuf &flagUnitBufReconstructedPixels, Pel *refBufUnfiltered,
    const CompArea &area, const Int bitDepths) {
  m_flipHoriz =
      false; // in this case only top and left reference samples are assumed
  m_flipVert =
      false; // in this case only top and left reference samples are assumed
  const PelBuf &recoBuf = recoUnitBuf.get(area.compID);
  const BoolBuf &recoBufFlag = flagUnitBufReconstructedPixels.get(area.compID);

  CHECK((recoBuf.width != recoBufFlag.width) ||
            (recoBuf.height != recoBufFlag.height),
        "size do not match");

  const int tuWidth = area.width;
  const int tuHeight = area.height;
  const int predSize = tuWidth + tuHeight;
  const int predStride = predSize + 1;
  CHECK(predStride * predStride > m_iYuvExtSize,
        "Reference sample area not supported");

  const int totalAboveSamples = predSize;
  const int totalLeftSamples = predSize;
  const int totalSamples =
      totalAboveSamples + totalLeftSamples + 1; //  +1: sample on the above left

  // ----- Step 1: analyze neighborhood -----
  const Position posLT = area;

  bool neighborFlags[4 * MAX_CU_SIZE +
                     1]; // +1: for above left and 2*MAX_CU_SIZE: for leftside
                         // or above side maximum size
  memset(neighborFlags, false,
         totalSamples *
             sizeof(bool)); // since since we are filling with 0s even if
                            // sizeof(bool)!=1, then we can use memset. Do not
                            // use memset to assign values to arrays of type
                            // sizeof(T) > 1;

  Pel tmpLineBuf[4 * MAX_CU_SIZE + 1];
  _bitDepth = bitDepths;
  const Pel valueDC = 1 << (bitDepths - 1);

  const CPelBuf roi_recoBuf(recoBuf.buf + rsAddr(area, recoBuf.stride),
                            recoBuf.stride, area);
  const CBoolBuf roi_recoBufFlag(recoBufFlag.buf +
                                     rsAddr(area, recoBufFlag.stride),
                                 recoBufFlag.stride, area);
  const Position aboveLeft = posLT.offset(-1, -1);
  if ((aboveLeft.y < Int(recoBufFlag.height)) &&
      (aboveLeft.y >= 0)) // if the above row is in the image // Fill top-left
                          // border and top and top right with rec. samples
  {
    Int x = std::max(aboveLeft.x, Int(0));
    Int offset_start_x = x - aboveLeft.x; // it should be either 0 or 1
    Int last_x =
        std::min(aboveLeft.x + totalAboveSamples, Int(recoBuf.width) - 1);

    memcpy(neighborFlags + totalLeftSamples + offset_start_x,
           roi_recoBufFlag.buf - roi_recoBufFlag.stride - 1 + offset_start_x,
           (last_x - x + 1) * sizeof(bool));
    memcpy(tmpLineBuf + totalLeftSamples + offset_start_x,
           roi_recoBuf.buf - roi_recoBuf.stride - 1 + offset_start_x,
           (last_x - x + 1) * sizeof(Pel));
  }

  if ((aboveLeft.x < Int(recoBufFlag.width)) &&
      (aboveLeft.x >= 0)) // Fill left and below left border with rec. samples
  {
    Int y = aboveLeft.y + 1; // because aboveLeft.y is already processed in the
                             // process of above row
    Int last_y =
        std::min(aboveLeft.y + totalLeftSamples, Int(recoBuf.height) - 1);

    const Pel *ptrSrc_pel = roi_recoBuf.buf - 1;
    const bool *ptrSrc_bool = roi_recoBufFlag.buf - 1;
    for (int i = totalLeftSamples - 1; y <= last_y; y++, i--) {
      neighborFlags[i] = *ptrSrc_bool;
      tmpLineBuf[i] = *ptrSrc_pel;
      ptrSrc_pel += roi_recoBuf.stride;
      ptrSrc_bool += roi_recoBufFlag.stride;
    }
  }

  // pad if necessary
  int interval_i0 = 0;
  int interval_i1 = interval_i0 + 1;
  bool interval_availableSamples =
      neighborFlags[interval_i0]; // flag which indicates that the current
                                  // interval we are passing is either the
                                  // interval of available samples or non
                                  // available samples
  while (interval_i1 < totalSamples - 1) {

    for (Int i = interval_i0 + 1; i < totalSamples - 1;
         i++) // totalSamples-1: to handle the case when there is no sample
              // available, therefore the data will be copied from
              // tmpLineBuf[totalSamples-1] to all
              // tmpLineBuf[0]:tmpLineBuf[totalSamples-2] with valueDC which is
              // already set in tmpLineBuf[totalSamples-1]
    {
      if (interval_availableSamples != neighborFlags[i])
        break;
      interval_i1 =
          i; // do not put it in for loop like this ( ; interval_i1 <
             // totalSamples-1; interval_i1++;) because for the case when
             // interval_i1>=totalSamples-1 then inside for loop is not executed
             // (as expected), but it will increase interval_i1 one more
    }
    //        std::cout << "interval_i0= " << interval_i0 << ", interval_i1= "
    //        << interval_i1 << std::endl;
    if (!interval_availableSamples &&
        neighborFlags[interval_i1 +
                      1]) // if the current interval is the interval that no
                          // sample is available for that, copy from the
                          // rightmost available data.
      std::fill(tmpLineBuf + interval_i0, tmpLineBuf + interval_i1 + 1,
                tmpLineBuf[interval_i1 +
                           1]); // interval_i1 is included that's why we have +1
                                // because The range filled is [first,last),
                                // which contains all the elements between first
                                // and last, including the element pointed by
                                // first but not the element pointed by last

    if (neighborFlags[interval_i0] !=
        neighborFlags[interval_i1 +
                      1]) // only to handle the case when last interval is not
                          // available, but there were some available samples in
                          // previous intervals
    {
      interval_i0 = interval_i1 + 1;
      interval_availableSamples = neighborFlags[interval_i0];
    }

    interval_i1++;
  }
  if (!interval_availableSamples) // pad the last non-available interval with
                                  // the leftmost available sample
  {
    if (interval_i0 == 0) // handle the case when no sample is available
      std::fill(tmpLineBuf + interval_i0, tmpLineBuf + totalSamples,
                valueDC); // lastIndex=totalSamples-1  is included that's why we
                          // have +1 because The range filled is [first,last),
                          // which contains all the elements between first and
                          // last, including the element pointed by first but
                          // not the element pointed by last
    else
      std::fill(
          tmpLineBuf + interval_i0, tmpLineBuf + totalSamples,
          tmpLineBuf[interval_i0 -
                     1]); // lastIndex=totalSamples-1  is included that's why we
                          // have +1 because The range filled is [first,last),
                          // which contains all the elements between first and
                          // last, including the element pointed by first but
                          // not the element pointed by last
  }

  // Copy processed samples
  Pel *ptrDst = refBufUnfiltered;
  memcpy(ptrDst, tmpLineBuf + totalLeftSamples,
         (totalAboveSamples + 1) *
             sizeof(Pel)); // top left, top and top right samples. +1 in
                           // (totalAboveSamples+1) * sizeof(Pel) is for
                           // aboveLeft sample

  Pel *ptrTmp = tmpLineBuf + totalLeftSamples;
  for (Int i = 1; i <= totalLeftSamples; i++) {
    ptrDst[i * predStride] = ptrTmp[-i];
  } // left side
}

void icov::video::IntraPrediction::xFillReferenceSamples(
    const PelUnitBuf &recoUnitBuf, const REFERENCE_SAMPLE_SIDE &refSampSide,
    Pel *refBufUnfiltered, const CompArea &area, const Int bitDepths) {
  m_flipHoriz = false; // initialize. it will be changed if it is necessary
                       // later in horizontal loop
  m_flipVert = false; // initialize. it will be changed if it is necessary later
                      // in vertical loop
  const PelBuf &recoBuf = recoUnitBuf.get(area.compID);

  const int tuWidth = area.width;
  const int tuHeight = area.height;
  const int predSize = tuWidth + tuHeight;
  const int predStride = predSize + 1;
  CHECK(predStride * predStride > m_iYuvExtSize,
        "Reference sample area not supported");

  const int totalAboveSamples = predSize;
  const int totalLeftSamples = predSize;
  const int totalSamples =
      totalAboveSamples + totalLeftSamples + 1; //  +1: sample on the above left

  bool neighborFlags[4 * MAX_CU_SIZE +
                     1]; // +1: for above left and 2*MAX_CU_SIZE: for leftside
                         // or above side maximum size
  memset(neighborFlags, false,
         totalSamples *
             sizeof(bool)); // since since we are filling with 0s even if
                            // sizeof(bool)!=1, then we can use memset. Do not
                            // use memset to assign values to arrays of type
                            // sizeof(T) > 1;

  Pel tmpLineBuf[4 * MAX_CU_SIZE + 1];
  _bitDepth = bitDepths;
  const Pel valueDC = 1 << (bitDepths - 1);

  const CPelBuf roi_recoBuf(recoBuf.buf + rsAddr(area, recoBuf.stride),
                            recoBuf.stride, area);

  // ----- Step 1: analyze neighborhood -----
  Int horizontal_y = -1;
  if (refSampSide & REFERENCE_SAMPLE_SIDE::TOP) // top row is used
  {
    m_flipVert = false;
    horizontal_y = area.topLeft().y - 1;
  } else if (refSampSide & REFERENCE_SAMPLE_SIDE::BOTTOM) // bottom row is used
  {
    m_flipVert = true;
    horizontal_y =
        area.topLeft().y +
        area.height; // note: since area.topLeft().y is already inside the area,
                     // therefore "area.topLeft().y + area.height" already point
                     // to the outside of area bottom row
  }

  Int vertical_x = -1;
  if (refSampSide & REFERENCE_SAMPLE_SIDE::LEFT) // left column is used
  {
    m_flipHoriz = false;
    vertical_x = area.topLeft().x - 1;
  } else if (refSampSide & REFERENCE_SAMPLE_SIDE::RIGHT) // right column is used
  {
    m_flipHoriz = true;
    vertical_x =
        area.topLeft().x +
        area.width; // note: since area.topLeft().y is already inside the area,
                    // therefore "area.topLeft().y + area.height" already point
                    // to the outside of area bottom row
  }

  if ((horizontal_y < Int(recoBuf.height)) &&
      (horizontal_y >= 0)) // if the above row is in the image // Fill top-left
                           // border and top and top right with rec. samples
  {
    if (m_flipHoriz) // horizontal line should be flipped
    {
      Int x = area.topLeft().x;
      const Int outside_right_corner_x = x + tuWidth;

      int last_x;
      if (refSampSide & 0b10000) // corner is used
        last_x = std::min(outside_right_corner_x,
                          Int(recoBuf.width) -
                              1); // std::min to handle the case when outside
                                  // corner is not inside image region
      else
        last_x = outside_right_corner_x -
                 1; // it is assumed that area is always in the image ragion and
                    // since in this case corner is not used, therefor last_x is
                    // equal to last column of area region

      const Pel *src_rowPtr =
          roi_recoBuf.buf +
          (horizontal_y - area.topLeft().y) * roi_recoBuf.stride -
          area.topLeft()
              .x; // begining of the original image buffer (not the area buffer)
      for (Int i = totalLeftSamples + outside_right_corner_x - last_x;
           last_x >= x; last_x--, i++) {
        neighborFlags[i] = true;
        tmpLineBuf[i] = src_rowPtr[last_x];
      }

    } else {
      Int left_corner_x = area.topLeft().x - 1;

      Int x;
      if (refSampSide & 0b10000) // corner is used
        x = std::max(left_corner_x, Int(0));
      else
        x = left_corner_x + 1;

      int last_x = area.topLeft().x + tuWidth -
                   1; // x and last_x are both included. no need to check if
                      // last_x is inside image region, because we assumed area
                      // is always inside image region

      memset(
          neighborFlags + totalLeftSamples + x - left_corner_x, true,
          (last_x - x + 1) *
              sizeof(
                  bool)); // memset sets the first num bytes of the block of
                          // memory pointed by ptr to the specified value
                          // (interpreted as an unsigned char), SO SETTING ALL
                          // BIT TO 1 for bool variable doesnt change anything
      memcpy(tmpLineBuf + totalLeftSamples + x - left_corner_x,
             roi_recoBuf.buf - 1 +
                 (horizontal_y - area.topLeft().y) * roi_recoBuf.stride + x -
                 left_corner_x,
             (last_x - x + 1) * sizeof(Pel));
    }
  }

  if ((vertical_x < Int(recoBuf.width)) &&
      (vertical_x >= 0)) // Fill left and below left border with rec. samples
  {

    Int y = area.topLeft().y;
    Int last_y = y + area.height -
                 1; // no need to check if it is inside image region, because we
                    // assume area is always inside image region.

    if (m_flipVert) // vertical line should be flipped
    {

      const Pel *ptrSrc_pel = roi_recoBuf.buf + vertical_x - area.topLeft().x +
                              roi_recoBuf.stride * (last_y - y);
      for (int i = totalLeftSamples - 1; last_y >= y; i--, last_y--) {
        neighborFlags[i] = true;
        tmpLineBuf[i] = *ptrSrc_pel;
        ptrSrc_pel -= roi_recoBuf.stride;
      }

    } else {
      const Pel *ptrSrc_pel = roi_recoBuf.buf + vertical_x - area.topLeft().x;
      for (int i = totalLeftSamples - 1; last_y >= y; i--, y++) {
        neighborFlags[i] = true;
        tmpLineBuf[i] = *ptrSrc_pel;
        ptrSrc_pel += roi_recoBuf.stride;
      }
    }
  }

  // pad if necessary
  int interval_i0 = 0;
  int interval_i1 = interval_i0 + 1;
  bool interval_availableSamples =
      neighborFlags[interval_i0]; // flag which indicates that the current
                                  // interval we are passing is either the
                                  // interval of available samples or non
                                  // available samples
  while (interval_i1 < totalSamples - 1) {

    for (Int i = interval_i0 + 1; i < totalSamples - 1;
         i++) // totalSamples-1: to handle the case when there is no sample
              // available, therefore the data will be copied from
              // tmpLineBuf[totalSamples-1] to all
              // tmpLineBuf[0]:tmpLineBuf[totalSamples-2] with valueDC which is
              // already set in tmpLineBuf[totalSamples-1]
    {
      if (interval_availableSamples != neighborFlags[i])
        break;
      interval_i1 =
          i; // do not put it in for loop like this ( ; interval_i1 <
             // totalSamples-1; interval_i1++;) because for the case when
             // interval_i1>=totalSamples-1 then inside for loop is not executed
             // (as expected), but it will increase interval_i1 one more
    }
    //        std::cout << "interval_i0= " << interval_i0 << ", interval_i1= "
    //        << interval_i1 << std::endl;
    if (!interval_availableSamples &&
        neighborFlags[interval_i1 +
                      1]) // if the current interval is the interval that no
                          // sample is available for that, copy from the
                          // rightmost available data.
      std::fill(tmpLineBuf + interval_i0, tmpLineBuf + interval_i1 + 1,
                tmpLineBuf[interval_i1 +
                           1]); // interval_i1 is included that's why we have +1
                                // because The range filled is [first,last),
                                // which contains all the elements between first
                                // and last, including the element pointed by
                                // first but not the element pointed by last

    if (neighborFlags[interval_i0] !=
        neighborFlags[interval_i1 +
                      1]) // only to handle the case when last interval is not
                          // available, but there were some available samples in
                          // previous intervals
    {
      interval_i0 = interval_i1 + 1;
      interval_availableSamples = neighborFlags[interval_i0];
    }

    interval_i1++;
  }
  if (!interval_availableSamples) // pad the last non-available interval with
                                  // the leftmost available sample
  {
    if (interval_i0 == 0) // handle the case when no sample is available
      std::fill(tmpLineBuf + interval_i0, tmpLineBuf + totalSamples,
                valueDC); // lastIndex=totalSamples-1  is included that's why we
                          // have +1 because The range filled is [first,last),
                          // which contains all the elements between first and
                          // last, including the element pointed by first but
                          // not the element pointed by last
    else
      std::fill(
          tmpLineBuf + interval_i0, tmpLineBuf + totalSamples,
          tmpLineBuf[interval_i0 -
                     1]); // lastIndex=totalSamples-1  is included that's why we
                          // have +1 because The range filled is [first,last),
                          // which contains all the elements between first and
                          // last, including the element pointed by first but
                          // not the element pointed by last
  }

  // Copy processed samples
  Pel *ptrDst = refBufUnfiltered;
  memcpy(ptrDst, tmpLineBuf + totalLeftSamples,
         (totalAboveSamples + 1) *
             sizeof(Pel)); // top left, top and top right samples. +1 in
                           // (totalAboveSamples+1) * sizeof(Pel) is for
                           // aboveLeft sample

  Pel *ptrTmp = tmpLineBuf + totalLeftSamples;
  for (Int i = 1; i <= totalLeftSamples; i++) {
    ptrDst[i * predStride] = ptrTmp[-i];
  } // left side
}

void icov::video::IntraPrediction::xFillReferenceSamples(
    const PelUnitBuf &recoUnitBuf,
    const BoolUnitBuf &flagUnitBufReconstructedPixels,
    const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE &refSampSide,
    Pel *refBufUnfiltered, const CompArea &area, const Int bitDepths) {
  m_flipHoriz = false; // initialize. it will be changed if it is necessary
                       // later in horizontal loop
  m_flipVert = false; // initialize. it will be changed if it is necessary later
                      // in vertical loop
  const PelBuf &recoBuf = recoUnitBuf.get(area.compID);
  const BoolBuf &recoBufFlag = flagUnitBufReconstructedPixels.get(area.compID);

  CHECK((recoBuf.width != recoBufFlag.width) ||
            (recoBuf.height != recoBufFlag.height),
        "size do not match");

  const int tuWidth = area.width;
  const int tuHeight = area.height;
  const int predSize = tuWidth + tuHeight;
  const int predStride = predSize + 1;
  CHECK(predStride * predStride > m_iYuvExtSize,
        "Reference sample area not supported");

  const int totalAboveSamples = predSize;
  const int totalLeftSamples = predSize;
  const int totalSamples =
      totalAboveSamples + totalLeftSamples + 1; //  +1: sample on the above left

  bool neighborFlags[4 * MAX_CU_SIZE +
                     1]; // +1: for above left and 2*MAX_CU_SIZE: for leftside
                         // or above side maximum size
  memset(neighborFlags, false,
         totalSamples *
             sizeof(bool)); // since since we are filling with 0s even if
                            // sizeof(bool)!=1, then we can use memset. Do not
                            // use memset to assign values to arrays of type
                            // sizeof(T) > 1;

  Pel tmpLineBuf[4 * MAX_CU_SIZE + 1];
  _bitDepth = bitDepths;
  const Pel valueDC = 1 << (bitDepths - 1);

  const CPelBuf roi_recoBuf(recoBuf.buf + rsAddr(area, recoBuf.stride),
                            recoBuf.stride, area);
  const CBoolBuf roi_recoBufFlag(recoBufFlag.buf +
                                     rsAddr(area, recoBufFlag.stride),
                                 recoBufFlag.stride, area);

  // ----- Step 1: analyze neighborhood -----
  Int horizontal_y = -1;
  if (refSampSide & REFERENCE_SAMPLE_SIDE::TOP) // top row is used
  {
    m_flipVert = false;
    horizontal_y = area.topLeft().y - 1;
  } else if (refSampSide & REFERENCE_SAMPLE_SIDE::BOTTOM) // bottom row is used
  {
    m_flipVert = true;
    horizontal_y =
        area.topLeft().y +
        area.height; // note: since area.topLeft().y is already inside the area,
                     // therefore "area.topLeft().y + area.height" already point
                     // to the outside of area bottom row
  }

  Int vertical_x = -1;
  if (refSampSide & REFERENCE_SAMPLE_SIDE::LEFT) // left column is used
  {
    m_flipHoriz = false;
    vertical_x = area.topLeft().x - 1;
  } else if (refSampSide & REFERENCE_SAMPLE_SIDE::RIGHT) // right column is used
  {
    m_flipHoriz = true;
    vertical_x =
        area.topLeft().x +
        area.width; // note: since area.topLeft().y is already inside the area,
                    // therefore "area.topLeft().y + area.height" already point
                    // to the outside of area bottom row
  }

  if ((horizontal_y < Int(recoBuf.height)) &&
      (horizontal_y >= 0)) // if the above row is in the image // Fill top-left
                           // border and top and top right with rec. samples
  {
    if (m_flipHoriz) // horizontal line should be flipped
    {
      Int x = area.topLeft().x;
      const Int outside_right_corner_x = x + tuWidth;

      int last_x;
      if (refSampSide & 0b10000) // corner is used
        last_x = std::min(outside_right_corner_x,
                          Int(recoBuf.width) -
                              1); // std::min to handle the case when outside
                                  // corner is not inside image region
      else
        last_x = outside_right_corner_x -
                 1; // it is assumed that area is always in the image ragion and
                    // since in this case corner is not used, therefor last_x is
                    // equal to last column of area region

      const Pel *src_rowPtr =
          roi_recoBuf.buf +
          (horizontal_y - area.topLeft().y) * roi_recoBuf.stride -
          area.topLeft()
              .x; // begining of the original image buffer (not the area buffer)
      const bool *flag_rowPtr =
          roi_recoBufFlag.buf +
          (horizontal_y - area.topLeft().y) * roi_recoBufFlag.stride -
          area.topLeft()
              .x; // begining of the original image buffer (not the area buffer)
      for (Int i = totalLeftSamples + outside_right_corner_x - last_x;
           last_x >= x; last_x--, i++) {
        neighborFlags[i] = flag_rowPtr[last_x];
        tmpLineBuf[i] = src_rowPtr[last_x];
      }

    } else {
      Int left_corner_x = area.topLeft().x - 1;

      Int x;
      if (refSampSide & 0b10000) // corner is used
        x = std::max(left_corner_x, Int(0));
      else
        x = left_corner_x + 1;

      int last_x = area.topLeft().x + tuWidth -
                   1; // x and last_x are both included. no need to check if
                      // last_x is inside image region, because we assumed area
                      // is always inside image region

      memcpy(neighborFlags + totalLeftSamples + x - left_corner_x,
             roi_recoBufFlag.buf - 1 +
                 (horizontal_y - area.topLeft().y) * roi_recoBufFlag.stride +
                 x - left_corner_x,
             (last_x - x + 1) * sizeof(bool));
      memcpy(tmpLineBuf + totalLeftSamples + x - left_corner_x,
             roi_recoBuf.buf - 1 +
                 (horizontal_y - area.topLeft().y) * roi_recoBuf.stride + x -
                 left_corner_x,
             (last_x - x + 1) * sizeof(Pel));
    }
  }

  if ((vertical_x < Int(recoBuf.width)) &&
      (vertical_x >= 0)) // Fill left and below left border with rec. samples
  {

    Int y = area.topLeft().y;
    Int last_y = y + area.height -
                 1; // no need to check if it is inside image region, because we
                    // assume area is always inside image region.

    if (m_flipVert) // vertical line should be flipped
    {

      const Pel *ptrSrc_pel = roi_recoBuf.buf + vertical_x - area.topLeft().x +
                              roi_recoBuf.stride * (last_y - y);
      const bool *ptrFlag_bool = roi_recoBufFlag.buf + vertical_x -
                                 area.topLeft().x +
                                 roi_recoBufFlag.stride * (last_y - y);
      for (int i = totalLeftSamples - 1; last_y >= y; i--, last_y--) {
        neighborFlags[i] = *ptrFlag_bool;
        tmpLineBuf[i] = *ptrSrc_pel;
        ptrSrc_pel -= roi_recoBuf.stride;
        ptrFlag_bool -= roi_recoBufFlag.stride;
      }

    } else {
      const Pel *ptrSrc_pel = roi_recoBuf.buf + vertical_x - area.topLeft().x;
      const bool *ptrFlag_bool =
          roi_recoBufFlag.buf + vertical_x - area.topLeft().x;
      for (int i = totalLeftSamples - 1; last_y >= y; i--, y++) {
        neighborFlags[i] = *ptrFlag_bool;
        tmpLineBuf[i] = *ptrSrc_pel;
        ptrSrc_pel += roi_recoBuf.stride;
        ptrFlag_bool += roi_recoBufFlag.stride;
      }
    }
  }

  // pad if necessary
  int interval_i0 = 0;
  int interval_i1 = interval_i0 + 1;
  bool interval_availableSamples =
      neighborFlags[interval_i0]; // flag which indicates that the current
                                  // interval we are passing is either the
                                  // interval of available samples or non
                                  // available samples
  while (interval_i1 < totalSamples - 1) {

    for (Int i = interval_i0 + 1; i < totalSamples - 1;
         i++) // totalSamples-1: to handle the case when there is no sample
              // available, therefore the data will be copied from
              // tmpLineBuf[totalSamples-1] to all
              // tmpLineBuf[0]:tmpLineBuf[totalSamples-2] with valueDC which is
              // already set in tmpLineBuf[totalSamples-1]
    {
      if (interval_availableSamples != neighborFlags[i])
        break;
      interval_i1 =
          i; // do not put it in for loop like this ( ; interval_i1 <
             // totalSamples-1; interval_i1++;) because for the case when
             // interval_i1>=totalSamples-1 then inside for loop is not executed
             // (as expected), but it will increase interval_i1 one more
    }
    //        std::cout << "interval_i0= " << interval_i0 << ", interval_i1= "
    //        << interval_i1 << std::endl;
    if (!interval_availableSamples &&
        neighborFlags[interval_i1 +
                      1]) // if the current interval is the interval that no
                          // sample is available for that, copy from the
                          // rightmost available data.
      std::fill(tmpLineBuf + interval_i0, tmpLineBuf + interval_i1 + 1,
                tmpLineBuf[interval_i1 +
                           1]); // interval_i1 is included that's why we have +1
                                // because The range filled is [first,last),
                                // which contains all the elements between first
                                // and last, including the element pointed by
                                // first but not the element pointed by last

    if (neighborFlags[interval_i0] !=
        neighborFlags[interval_i1 +
                      1]) // only to handle the case when last interval is not
                          // available, but there were some available samples in
                          // previous intervals
    {
      interval_i0 = interval_i1 + 1;
      interval_availableSamples = neighborFlags[interval_i0];
    }

    interval_i1++;
  }
  if (!interval_availableSamples) // pad the last non-available interval with
                                  // the leftmost available sample
  {
    if (interval_i0 == 0) // handle the case when no sample is available
      std::fill(tmpLineBuf + interval_i0, tmpLineBuf + totalSamples,
                valueDC); // lastIndex=totalSamples-1  is included that's why we
                          // have +1 because The range filled is [first,last),
                          // which contains all the elements between first and
                          // last, including the element pointed by first but
                          // not the element pointed by last
    else
      std::fill(
          tmpLineBuf + interval_i0, tmpLineBuf + totalSamples,
          tmpLineBuf[interval_i0 -
                     1]); // lastIndex=totalSamples-1  is included that's why we
                          // have +1 because The range filled is [first,last),
                          // which contains all the elements between first and
                          // last, including the element pointed by first but
                          // not the element pointed by last
  }

  // Copy processed samples
  Pel *ptrDst = refBufUnfiltered;
  memcpy(ptrDst, tmpLineBuf + totalLeftSamples,
         (totalAboveSamples + 1) *
             sizeof(Pel)); // top left, top and top right samples. +1 in
                           // (totalAboveSamples+1) * sizeof(Pel) is for
                           // aboveLeft sample

  Pel *ptrTmp = tmpLineBuf + totalLeftSamples;
  for (Int i = 1; i <= totalLeftSamples; i++) {
    ptrDst[i * predStride] = ptrTmp[-i];
  } // left side
}

void icov::video::IntraPrediction::xFilterReferenceSamples(
    const Pel *refBufUnfiltered, Pel *refBufFiltered, const CompArea &area) {
  const int tuWidth = area.width;
  const int tuHeight = area.height;
  const int predSize = tuWidth + tuHeight;
  const int predStride = predSize + 1;

  // Regular reference sample filter
  const Pel *piSrcPtr =
      refBufUnfiltered + (predStride * predSize);            // bottom left
  Pel *piDestPtr = refBufFiltered + (predStride * predSize); // bottom left

  // bottom left (not filtered)
  *piDestPtr = *piSrcPtr;
  piDestPtr -= predStride;
  piSrcPtr -= predStride;
  // left column (bottom to top)
  for (UInt i = 1; i < predSize;
       i++, piDestPtr -= predStride, piSrcPtr -= predStride) {
    *piDestPtr =
        (piSrcPtr[predStride] + 2 * piSrcPtr[0] + piSrcPtr[-predStride] + 2) >>
        2;
  }

  // top-left
  *piDestPtr = (piSrcPtr[predStride] + 2 * piSrcPtr[0] + piSrcPtr[1] + 2) >> 2;
  piDestPtr++;
  piSrcPtr++;
  // top row (left-to-right)
  for (UInt i = 1; i < predSize; i++, piDestPtr++, piSrcPtr++) {
    *piDestPtr = (piSrcPtr[1] + 2 * piSrcPtr[0] + piSrcPtr[-1] + 2) >> 2;
  }
  // top right (not filtered)
  *piDestPtr = *piSrcPtr;
}

void icov::video::IntraPrediction::xPredIntraPlanar(const CPelBuf &pSrc,
                                                    PelBuf &pDst) {
  const UInt width = pDst.width;
  const UInt height = pDst.height;
  const UInt log2W = g_aucLog2[width];
  const UInt log2H = g_aucLog2[height];

  Int leftColumn[MAX_CU_SIZE + 1], topRow[MAX_CU_SIZE + 1],
      bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
  const UInt offset = width * height;

  // Get left and above reference column and row
  for (Int k = 0; k < width + 1; k++) {
    topRow[k] = pSrc.at(k + 1, 0);
  }

  for (Int k = 0; k < height + 1; k++) {
    leftColumn[k] = pSrc.at(0, k + 1);
  }

  // Prepare intermediate variables used in interpolation
  Int bottomLeft = leftColumn[height];
  Int topRight = topRow[width];

  for (Int k = 0; k < width; k++) {
    bottomRow[k] = bottomLeft - topRow[k];
    topRow[k] = topRow[k] << log2H;
  }

  for (Int k = 0; k < height; k++) {
    rightColumn[k] = topRight - leftColumn[k];
    leftColumn[k] = leftColumn[k] << log2W;
  }

  const UInt finalShift = 1 + log2W + log2H;
  const UInt stride = pDst.stride;
  Pel *pred = pDst.buf;
  for (Int y = 0; y < height; y++, pred += stride) {
    Int horPred = leftColumn[y];

    for (Int x = 0; x < width; x++) {
      horPred += rightColumn[y];
      topRow[x] += bottomRow[x];

      Int vertPred = topRow[x];
      pred[x] =
          ((horPred << log2H) + (vertPred << log2W) + offset) >> finalShift;
    }
  }
}

Pel icov::video::IntraPrediction::xGetPredValDc(const CPelBuf &pSrc,
                                                const Size &dstSize) {
  CHECK(dstSize.width == 0 || dstSize.height == 0, "Empty area provided");

  Int iInd, iSum = 0;
  Pel pDcVal;
  const int width = dstSize.width;
  const int height = dstSize.height;

  for (iInd = 0; iInd < width; iInd++) {
    iSum += pSrc.at(1 + iInd, 0);
  }
  for (iInd = 0; iInd < height; iInd++) {
    iSum += pSrc.at(0, 1 + iInd);
  }

  pDcVal = (iSum + ((width + height) >> 1)) / (width + height);
  return pDcVal;
}

void icov::video::IntraPrediction::xPredIntraAng(const CPelBuf &pSrc,
                                                 PelBuf &pDst,
                                                 const UInt dirMode) {
  Int width = Int(pDst.width);
  Int height = Int(pDst.height);

  CHECK(!(dirMode > DC_IDX && dirMode < NUM_LUMA_MODE), "Invalid intra dir");

  const Bool bIsModeVer = (dirMode >= DIA_IDX);
  const Int intraPredAngleMode =
      (bIsModeVer) ? (Int)dirMode - VER_IDX : -((Int)dirMode - HOR_IDX);
  const Int absAngMode = abs(intraPredAngleMode);
  const Int signAng = intraPredAngleMode < 0 ? -1 : 1;

  // Set bitshifts and scale the angle parameter to block size

  static const Int angTable[17] = {0,  1,  2,  3,  5,  7,  9,  11, 13,
                                   15, 17, 19, 21, 23, 26, 29, 32};
  static const Int invAngTable[17] = {
      0,   8192, 4096, 2731, 1638, 1170, 910, 745, 630,
      546, 482,  431,  390,  356,  315,  282, 256}; // (256 * 32) / Angle

  Int invAngle = invAngTable[absAngMode];
  Int absAng = angTable[absAngMode];
  Int intraPredAngle = signAng * absAng;

  Pel *refMain;
  Pel *refSide;

  Pel refAbove[2 * MAX_CU_SIZE + 1];
  Pel refLeft[2 * MAX_CU_SIZE + 1];

  // Initialize the Main and Left reference array.
  if (intraPredAngle < 0) {
    for (Int x = 0; x < width + 1; x++) {
      refAbove[x + height - 1] = pSrc.at(x, 0);
    }
    for (Int y = 0; y < height + 1; y++) {
      refLeft[y + width - 1] = pSrc.at(0, y);
    }
    refMain = (bIsModeVer ? refAbove + height : refLeft + width) - 1;
    refSide = (bIsModeVer ? refLeft + width : refAbove + height) - 1;

    // Extend the Main reference to the left.
    Int invAngleSum = 128; // rounding for (shift by 8)
    const Int refMainOffsetPreScale = bIsModeVer ? height : width;
    for (Int k = -1; k > (refMainOffsetPreScale * intraPredAngle) >> 5; k--) {
      invAngleSum += invAngle;
      refMain[k] = refSide[invAngleSum >> 8];
    }
  } else {
    for (Int x = 0; x < width + height + 1; x++) {
      refAbove[x] = pSrc.at(x, 0);
      refLeft[x] = pSrc.at(0, x);
    }
    refMain = bIsModeVer ? refAbove : refLeft;
    refSide = bIsModeVer ? refLeft : refAbove;
  }

  // swap width/height if we are doing a horizontal mode:
  Pel tempArray[MAX_CU_SIZE * MAX_CU_SIZE];
  const Int dstStride = bIsModeVer ? pDst.stride : MAX_CU_SIZE;
  Pel *pDstBuf = bIsModeVer ? pDst.buf : tempArray;
  if (!bIsModeVer) {
    std::swap(width, height);
  }

  if (intraPredAngle == 0) // pure vertical or pure horizontal
  {
    for (Int y = 0; y < height; y++) {
      for (Int x = 0; x < width; x++) {
        pDstBuf[y * dstStride + x] = refMain[x + 1];
      }
    }
  } else {
    Pel *pDsty = pDstBuf;

    for (Int y = 0, deltaPos = intraPredAngle; y < height;
         y++, deltaPos += intraPredAngle, pDsty += dstStride) {
      const Int deltaInt = deltaPos >> 5;
      const Int deltaFract = deltaPos & (32 - 1);

      if (absAng < 32) {
        {
          // Do linear filtering
          const Pel *pRM = refMain + deltaInt + 1;
          Int lastRefMainPel = *pRM++;
          for (Int x = 0; x < width; pRM++, x++) {
            Int thisRefMainPel = *pRM;
            pDsty[x + 0] = (Pel)(((32 - deltaFract) * lastRefMainPel +
                                  deltaFract * thisRefMainPel + 16) >>
                                 5);
            lastRefMainPel = thisRefMainPel;
          }
        }
      } else {
        // Just copy the integer samples
        for (Int x = 0; x < width; x++) {
          pDsty[x] = refMain[x + deltaInt + 1];
        }
      }
    }
  }

  // Flip the block if this is the horizontal mode
  if (!bIsModeVer) {
    for (Int y = 0; y < height; y++) {
      for (Int x = 0; x < width; x++) {
        pDst.at(y, x) = pDstBuf[x];
      }
      pDstBuf += dstStride;
    }
  }
}

icov::video::IntraPrediction::IntraPrediction()
    : m_currChromaFormat(NUM_CHROMA_FORMAT) {
  for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++) {
    for (UInt buf = 0; buf < NUM_PRED_BUF; buf++) {
      m_piYuvExt[ch][buf] = nullptr;
    }
  }
}

icov::video::IntraPrediction::~IntraPrediction() { destroy(); }

void icov::video::IntraPrediction::init(ChromaFormat chromaFormatIDC) {
  // if it has been initialised before, but the chroma format has changed,
  // release the memory and start again.
  if (m_piYuvExt[COMPONENT_Y][PRED_BUF_UNFILTERED] != nullptr &&
      m_currChromaFormat != chromaFormatIDC) {
    destroy();
  }

  m_currChromaFormat = chromaFormatIDC;

  if (m_piYuvExt[COMPONENT_Y][PRED_BUF_UNFILTERED] ==
      nullptr) // check if first is null (in which case, nothing initialised
               // yet)
  {
    m_iYuvExtSize = (MAX_CU_SIZE * 2 + 1) * (MAX_CU_SIZE * 2 + 1);

    for (UInt ch = 0; ch < MAX_NUM_COMPONENT; ch++) {
      for (UInt buf = 0; buf < NUM_PRED_BUF; buf++) {
        m_piYuvExt[ch][buf] = new Pel[m_iYuvExtSize];
      }
    }
  }

  std::cout << "pred init\n";
}

void icov::video::IntraPrediction::initIntraPatternChType(
    const PelUnitBuf &recoUnitBuf,
    const BoolUnitBuf &flagUnitBufReconstructedPixels, const CompArea &area,
    const Int bitDepths, const Bool bFilterRefSamples) {
  Pel *refBufUnfiltered = m_piYuvExt[area.compID][PRED_BUF_UNFILTERED];
  Pel *refBufFiltered = m_piYuvExt[area.compID][PRED_BUF_FILTERED];
  m_lastArea = area;
  m_filterRefSamples = bFilterRefSamples;
  // ----- Step 1: unfiltered reference samples -----
  xFillReferenceSamples(recoUnitBuf, flagUnitBufReconstructedPixels,
                        refBufUnfiltered, area, bitDepths);
  // ----- Step 2: filtered reference samples -----
  if (bFilterRefSamples) {
    xFilterReferenceSamples(refBufUnfiltered, refBufFiltered, area);
  }
}

void icov::video::IntraPrediction::initIntraPatternChType(
    const PelUnitBuf &recoUnitBuf, const REFERENCE_SAMPLE_SIDE &refSampSide,
    const CompArea &area, const Int bitDepths, const Bool bFilterRefSamples) {
  Pel *refBufUnfiltered = m_piYuvExt[area.compID][PRED_BUF_UNFILTERED];
  Pel *refBufFiltered = m_piYuvExt[area.compID][PRED_BUF_FILTERED];
  m_lastArea = area;
  m_filterRefSamples = bFilterRefSamples;
  // ----- Step 1: unfiltered reference samples -----
  xFillReferenceSamples(recoUnitBuf, refSampSide, refBufUnfiltered, area,
                        bitDepths);
  // ----- Step 2: filtered reference samples -----
  if (bFilterRefSamples) {
    xFilterReferenceSamples(refBufUnfiltered, refBufFiltered, area);
  }
}

void icov::video::IntraPrediction::initIntraPatternChType(
    const PelUnitBuf &recoUnitBuf,
    const BoolUnitBuf &flagUnitBufReconstructedPixels,
    const icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE &refSampSide,
    const CompArea &area, const Int bitDepths, const Bool bFilterRefSamples) {
  Pel *refBufUnfiltered = m_piYuvExt[area.compID][PRED_BUF_UNFILTERED];
  Pel *refBufFiltered = m_piYuvExt[area.compID][PRED_BUF_FILTERED];
  m_lastArea = area;
  m_filterRefSamples = bFilterRefSamples;
  // ----- Step 1: unfiltered reference samples -----
  xFillReferenceSamples(recoUnitBuf, flagUnitBufReconstructedPixels,
                        refSampSide, refBufUnfiltered, area, bitDepths);
  // ----- Step 2: filtered reference samples -----
  if (bFilterRefSamples) {
    xFilterReferenceSamples(refBufUnfiltered, refBufFiltered, area);
  }
}

int icov::video::IntraPrediction::getAvailableSides(
    const BoolUnitBuf &flagUnitBufReconstructedPixels, const CompArea &area) {

  int availableSides = 0;
  const BoolBuf &recoBufFlag = flagUnitBufReconstructedPixels.get(area.compID);
  const CBoolBuf roi_recoBufFlag(recoBufFlag.buf +
                                     rsAddr(area, recoBufFlag.stride),
                                 recoBufFlag.stride, area);

  bool flagAllPelAvailable;
  int horizontal_y, vertical_x;
  const bool *srcPtr;

  // check top side
  horizontal_y = area.topLeft().y - 1;
  if ((horizontal_y < Int(recoBufFlag.height)) &&
      (horizontal_y >= 0)) // if the above row is in the image // Fill top-left
                           // border and top and top right with rec. samples
  {
    flagAllPelAvailable = true;
    srcPtr = roi_recoBufFlag.buf - roi_recoBufFlag.stride;

    if (area.topLeft().x - 1 >= 0 &&
        (*(srcPtr - 1) == true)) // check top-left corner
      availableSides |= 0b000100000;

    if (((area.topLeft().x + area.width) < recoBufFlag.width) &&
        (*(srcPtr + area.width) == true)) // check top-right corner
      availableSides |= 0b001000000;

    for (UInt i = 0; i < area.width; i++, srcPtr++) {
      if (!(*srcPtr)) {
        flagAllPelAvailable = false;
        break;
      }
    }
    if (flagAllPelAvailable)
      availableSides |= REFERENCE_SAMPLE_SIDE::TOP;
  }

  // check bottom side
  horizontal_y =
      area.topLeft().y +
      area.height; // note: since area.topLeft().y is already inside the area,
                   // therefore "area.topLeft().y + area.height" already point
                   // to the outside of area bottom row
  if ((horizontal_y < Int(recoBufFlag.height)) &&
      (horizontal_y >= 0)) // if the above row is in the image // Fill top-left
                           // border and top and top right with rec. samples
  {
    flagAllPelAvailable = true;
    srcPtr = roi_recoBufFlag.buf + area.height * roi_recoBufFlag.stride;

    if (area.topLeft().x - 1 >= 0 &&
        (*(srcPtr - 1) ==
         true)) // check bottom-left corner. important Note we can use topLeft.x
                // because bottomLeft.x has also the same x. the advantage is
                // that topLeft.x returns a reference so no copy happens
      availableSides |= 0b010000000;

    if (((area.topLeft().x + area.width) < recoBufFlag.width) &&
        (*(srcPtr + area.width) == true)) // check bottom-right corner
      availableSides |= 0b100000000;

    for (UInt i = 0; i < area.width; i++, srcPtr++) {
      if (!(*srcPtr)) {
        flagAllPelAvailable = false;
        break;
      }
    }
    if (flagAllPelAvailable)
      availableSides |= REFERENCE_SAMPLE_SIDE::BOTTOM;
  }

  // check left side
  vertical_x = area.topLeft().x - 1;
  if ((vertical_x < Int(recoBufFlag.width)) &&
      (vertical_x >= 0)) // Fill left and below left border with rec. samples
  {
    flagAllPelAvailable = true;
    srcPtr = roi_recoBufFlag.buf - 1;

    for (UInt j = 0; j < area.height; j++, srcPtr += roi_recoBufFlag.stride) {
      if (!(*srcPtr)) {
        flagAllPelAvailable = false;
        break;
      }
    }

    if (flagAllPelAvailable)
      availableSides |= REFERENCE_SAMPLE_SIDE::LEFT;
  }

  // check right side
  vertical_x = area.topLeft().x +
               area.width; // note: since area.topLeft().y is already inside the
                           // area, therefore "area.topLeft().y + area.height"
                           // already point to the outside of area bottom row
  if ((vertical_x < Int(recoBufFlag.width)) &&
      (vertical_x >= 0)) // Fill left and below left border with rec. samples
  {
    flagAllPelAvailable = true;
    srcPtr = roi_recoBufFlag.buf + area.width;

    for (UInt j = 0; j < area.height; j++, srcPtr += roi_recoBufFlag.stride) {
      if (!(*srcPtr)) {
        flagAllPelAvailable = false;
        break;
      }
    }

    if (flagAllPelAvailable)
      availableSides |= REFERENCE_SAMPLE_SIDE::RIGHT;
  }

  return availableSides;
}

void icov::video::IntraPrediction::predIntraAng(PelBuf &piPred,
                                                const UInt dirMode) {
  CHECK((piPred.width != m_lastArea.width) ||
            (piPred.height != m_lastArea.height),
        "size does not match with initialized data");

  const Int iWidth = piPred.width;
  const Int iHeight = piPred.height;
  const Int srcStride = (iWidth + iHeight + 1);
  Pel *ptrSrc = getPredictorPtr(m_lastArea.compID, m_filterRefSamples);

  if (NUM_LUMA_MODE == 68 &&
      dirMode == (NUM_LUMA_MODE -
                  1)) // I added this later because I added another mode
                      // (dirMode NUM_LUMA_MODE-1) to send midan intensity value
    piPred.fill((1 << (_bitDepth - 1)));
  else if (dirMode == PLANAR_IDX)
    xPredIntraPlanar(CPelBuf(ptrSrc, srcStride, srcStride), piPred);
  else if (dirMode == DC_IDX)
    xPredIntraDc(CPelBuf(ptrSrc, srcStride, srcStride), piPred);
  else
    xPredIntraAng(CPelBuf(ptrSrc, srcStride, srcStride), piPred,
                  dirMode); // no need to check if dirMode is out of range 0 <=
                            // dirMode < NUM_LUMA_MODE, because it is already
                            // checked in xPredIntraAng

  //    switch(dirMode)
  //    {
  //    case(NUM_LUMA_MODE-1):
  //        piPred.fill((1 << (_bitDepth - 1)));
  //        break;
  //    case( PLANAR_IDX ):
  //        xPredIntraPlanar( CPelBuf( ptrSrc, srcStride, srcStride ), piPred);
  //        break;
  //    case( DC_IDX ):     // including DCPredFiltering
  //        xPredIntraDc( CPelBuf( ptrSrc, srcStride, srcStride ), piPred);
  //        break;
  //    default:
  //        xPredIntraAng( CPelBuf( ptrSrc, srcStride, srcStride ), piPred,
  //        dirMode); break;
  //    }

  if (m_flipVert)
    VTM_Utility::flipVert<Pel>(piPred, piPred);

  if (m_flipHoriz)
    VTM_Utility::flipHoriz<Pel>(piPred, piPred);
}
