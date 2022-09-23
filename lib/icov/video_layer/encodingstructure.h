/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef ENCODINGSTRUCTURE_H
#define ENCODINGSTRUCTURE_H
#include <opencv2/core.hpp>

#include "helpers.h"
#include "icu_encoder.h"
#include "mypicture.h"

struct EncodingStructure {
  EncodingStructure();
  EncodingStructure(const std::vector<ICU_Encoder> &icus, const UInt icusPerRow,
                    const UInt icusPerColumn);
  ~EncodingStructure();
  void set(const std::vector<ICU_Encoder> &icus, const UInt icusPerRow,
           const UInt icusPerColumn);
  void setCheckPoints(const std::vector<Position> &checkPoints);
  void setCheckPoint(const Position &checkPoint);
  bool isAccessBlock(const Position &accessBlockPoint);

  std::vector<ICU_Encoder> vec_icus;
  std::vector<Position> vec_checkPoints;
  UInt n_icusPerRow, n_icusPerCol;

  ICU_Encoder &getICU(const UInt icu_index);

  ICU_Encoder &getICU(const UInt x_index, const UInt y_index);

  const ICU_Encoder &getICU(const UInt icu_index) const;

  bool isLeftVerticalBorder(const UInt icu_index) const;
  bool isRightVerticalBorder(const UInt icu_index) const;

  UInt get_icuIndex(const Position &pos) const;
  Position get_icuPosition(const UInt icu_index) const;

  void resetDecodedICUs();

  void updateRequestedICUs(
      const cv::Mat &maskRequestedPixels,
      std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
      std::vector<std::vector<Position>> &requestedICUs_closeTocheckPoints,
      std::vector<Position> &neighborcheckPointInTheComponent,
      double *usefulness_percent = nullptr);

  void updateRequestedICUs(
      const cv::Mat &maskRequestedPixels,
      std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
      std::vector<std::vector<Position>> &requestedICUs_closeTocheckPoints,
      std::vector<Position> &neighborcheckPointInTheComponent,
      std::vector<Position> &closestCheckPointToTheRemainingICUs,
      std::vector<std::vector<Position>>
          &remainingICUS_afterClosestCheckPointToBeDecoded,
      std::vector<std::vector<Position>>
          &remainingICUS_afterAlreadyProcessedICUsToBeDecoded);

  void reorderBlockDecodingBasedOnStartingAccessBlock(
      std::vector<std::vector<Position>> &in_out_requested_icus,
      const std::vector<Position> &accessBlockInsideAsStartingPosition);
  void get_decodedICUsInRequestedICUs(
      const cv::Mat &maskRequestedPixels,
      std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
      std::vector<std::vector<Position>> &decodedICUs_inRequestedICUs);
  void updateDecodedICU(const Position &ICU_position);
  void updateDecodedICUs(
      const std::vector<std::vector<Position>> &connectedComponents);
  bool isICUAlreadyDecoded(const Position &ICU_position) const;
  const std::vector<Position> &get_vecDecodedICUs() const;

  const std::vector<Position> &get_vecCheckpoints() const;
  /** returns an integer which its last 4 least significant bits show which
   * sides are available. For more information refer to enum
   * REFERENCE_SAMPLE_SIDE Also the bit levels [5-8] (zero-based index) is 1 if
   * the corner point is also available: bit level 4 = (not used) to avoid
   * conflict of corner usage in REFERENCE_SAMPLE_SIDE. bit level 5 = top-left
   * corner bit level 6 = top-right corner bit level 7 = bottom-left corner bit
   * level 8 = bottom-right corner
   */
  int getAvailableDecodedSides_regularImage(const Position &icuPos) const;
  int getAvailableDecodedSides_360Image(const Position &icuPos) const;

  void create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
      ChromaFormat chFmt, const UInt sourceWidth_luma,
      const UInt sourceHeight_luma, const UInt sourceMargin_x_luma,
      const UInt icuWidth_luma, const UInt icuHeight_luma,
      const UInt tuWidth_luma, const UInt tuHeight_luma);

  void applyPredictionBasedOnEntropy(
      MyPicture &encpic, MyIntraPrediction &intraPred, const ComponentID compID,
      const int bitDepths[MAX_NUM_CHANNEL_TYPE], const UInt tuWidth_luma,
      const UInt tuHeight_luma, bool encodeFrequencyTableInBitStream,
      const int qp, int maxLog2TrDynamicRange);

protected:
  void resetAlreadyProcessedICUs();
  void updateAlreadyProccessedICU(const Position &ICU_position);
  void copyProccessedICUsFromDecodedICUs();
  bool isICUAlreadyProcessed(const Position &ICU_position);

  cv::Mat _integralImageMask;
  cv::Mat _maskThresholded;
  std::vector<int> _vec_pixelsRequestedPerICU;
  std::vector<Position> _vec_decodedICUs;
  AreaBuf<bool> _requestedICUs, _decodedICUs;
  std::vector<Position> _vec_tmp_alreadyProcessedICUs;
  AreaBuf<bool> _tmp_alreadyProcessedICUs;
  void deallocate();
};

#endif // ENCODINGSTRUCTURE_H
