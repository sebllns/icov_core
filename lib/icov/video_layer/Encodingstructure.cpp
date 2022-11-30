#include "Encodingstructure.h"

#include <opencv2/imgproc.hpp>

#include "ICOV_Quant.h"
#include "ICOV_Transform2d.h"
#include "IntraPrediction.h"
#include "vtm_opencv_utility.h"
#include "vtm_utility.h"

icov::video::EncodingStructure::EncodingStructure() { deallocate(); }

icov::video::EncodingStructure::EncodingStructure(
    const std::vector<IcuEncoder> &icus, const UInt icusPerRow,
    const UInt icusPerColumn) {
  set(icus, icusPerRow, icusPerColumn);
}

icov::video::EncodingStructure::~EncodingStructure() { deallocate(); }

void icov::video::EncodingStructure::set(const std::vector<IcuEncoder> &icus,
                                         const UInt icusPerRow,
                                         const UInt icusPerColumn) {
  deallocate();

  vec_icus = icus;
  n_icusPerRow = icusPerRow;
  n_icusPerCol = icusPerColumn;
  CHECK(vec_icus.size() != n_icusPerRow * n_icusPerCol,
        "incorrect number of icus per row and per column");

  bool *reqPtr = (bool *)xMalloc(bool, n_icusPerRow *n_icusPerCol);
  _requestedICUs =
      AreaBuf<bool>(reqPtr, n_icusPerRow, n_icusPerRow, n_icusPerCol);
  bool *prePtr = (bool *)xMalloc(bool, n_icusPerRow *n_icusPerCol);
  _decodedICUs =
      AreaBuf<bool>(prePtr, n_icusPerRow, n_icusPerRow, n_icusPerCol);

  UInt n_icus = n_icusPerRow * n_icusPerCol;
  _vec_pixelsRequestedPerICU.resize(n_icus);

  _vec_decodedICUs.reserve(n_icus);

  _vec_tmp_alreadyProcessedICUs.reserve(n_icus);
  bool *alreadyProPtr = (bool *)xMalloc(bool, n_icusPerRow *n_icusPerCol);
  _tmp_alreadyProcessedICUs =
      AreaBuf<bool>(alreadyProPtr, n_icusPerRow, n_icusPerRow, n_icusPerCol);
}

void icov::video::EncodingStructure::setCheckPoints(
    const std::vector<Position> &checkPoints) {
  for (unsigned int i = 0; i < checkPoints.size(); i++)
    setCheckPoint(checkPoints[i]);
}

void icov::video::EncodingStructure::setCheckPoint(const Position &checkPoint) {
  CHECK((checkPoint.x < 0) || (checkPoint.x >= int(n_icusPerRow)) ||
            (checkPoint.y < 0) || (checkPoint.y >= int(n_icusPerCol)),
        "check point is not inside the image");
  for (unsigned int i = 0; i < vec_checkPoints.size(); i++) {
    if (vec_checkPoints[i].x == checkPoint.x &&
        vec_checkPoints[i].y == checkPoint.y) {
      std::cout << "warning: added check point already exists." << std::endl;
      return;
    }
  }
  vec_checkPoints.push_back(checkPoint);
}

bool icov::video::EncodingStructure::isAccessBlock(
    const Position &accessBlockPoint) {
  for (unsigned int ab_ind = 0; ab_ind < vec_checkPoints.size(); ab_ind++)
    if (vec_checkPoints[ab_ind].x == accessBlockPoint.x &&
        vec_checkPoints[ab_ind].y == accessBlockPoint.y)
      return true;

  return false;
}

Position
icov::video::EncodingStructure::get_icuPosition(const UInt icu_index) const {
  div_t divresult;
  divresult = div(icu_index, Int(n_icusPerRow));
  UInt icu_x = divresult.rem;
  UInt icu_y = divresult.quot;
  Position block_position(icu_x, icu_y);

  if (this->get_icuIndex(block_position) != icu_index)
    throw std::logic_error(
        "The output block position is not correct. Something is missing");

  return block_position;
}

static const int dx[4] = {1, -1, 0, 0};
static const int dy[4] = {0, 0, -1, 1};
static void dfs_360(AreaBuf<bool> &requestedIcus,
                    const AreaBuf<bool> &alreadyDecodedICus,
                    std::vector<Position> &currentComponent_requestedICUs,
                    int x, int y, int &remainingICUs) {
  if (!alreadyDecodedICus.at(x, y)) // add only if it is not decoded
    currentComponent_requestedICUs.push_back(Position(x, y));

  remainingICUs--;
  requestedIcus.at(x, y) = false;
  int iWidth = int(
      requestedIcus
          .width); // if we do not convert to int, the traversing equation for
  // 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) %
  // iWidth) converts everything to UInt
  int iHeight = int(requestedIcus.height);
  for (int i = 0; i < 4; i++) {
    int neighbor_x = x + dx[i];
    neighbor_x =
        (iWidth + (neighbor_x % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    int neighbor_y = y + dy[i];
    if (neighbor_y < 0 ||
        neighbor_y >= iHeight) // do not check outside image in y direction
      continue;
    if (requestedIcus.at(neighbor_x, neighbor_y))
      dfs_360(requestedIcus, alreadyDecodedICus, currentComponent_requestedICUs,
              neighbor_x, neighbor_y, remainingICUs);
  }
}

static void dfs_360(AreaBuf<bool> &requestedIcus,
                    const Position &checkPoint_pos,
                    std::vector<Position> &currentComponent_requestedICUs,
                    int x, int y, int &remainingICUs) {
  if (x != checkPoint_pos.x || y != checkPoint_pos.y)
    currentComponent_requestedICUs.push_back(Position(x, y));

  remainingICUs--;
  requestedIcus.at(x, y) = false;
  int iWidth = int(
      requestedIcus
          .width); // if we do not convert to int, the traversing equation for
  // 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) %
  // iWidth) converts everything to UInt
  int iHeight = int(requestedIcus.height);
  for (int i = 0; i < 4; i++) {
    int neighbor_x = x + dx[i];
    neighbor_x =
        (iWidth + (neighbor_x % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    int neighbor_y = y + dy[i];
    if (neighbor_y < 0 ||
        neighbor_y >= iHeight) // do not check outside image in y direction
      continue;
    if (requestedIcus.at(neighbor_x, neighbor_y))
      dfs_360(requestedIcus, checkPoint_pos, currentComponent_requestedICUs,
              neighbor_x, neighbor_y, remainingICUs);
  }
}

static void dfs_360(AreaBuf<bool> &requestedIcus,
                    std::vector<Position> &currentComponent_requestedICUs,
                    int x, int y, int &remainingICUs) {
  //    if( !requestedIcus.at(x,y) )    // add only if it is not decoded
  CHECK(!requestedIcus.at(x, y),
        "Something is wrong. The requestedIcus in this position must be true.");
  currentComponent_requestedICUs.push_back(Position(x, y));
  remainingICUs--;
  requestedIcus.at(x, y) = false;
  int iWidth = int(
      requestedIcus
          .width); // if we do not convert to int, the traversing equation for
  // 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) %
  // iWidth) converts everything to UInt
  int iHeight = int(requestedIcus.height);
  for (int i = 0; i < 4; i++) {
    int neighbor_x = x + dx[i];
    neighbor_x =
        (iWidth + (neighbor_x % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    int neighbor_y = y + dy[i];
    if (neighbor_y < 0 ||
        neighbor_y >= iHeight) // do not check outside image in y direction
      continue;
    if (requestedIcus.at(neighbor_x, neighbor_y))
      dfs_360(requestedIcus, currentComponent_requestedICUs, neighbor_x,
              neighbor_y, remainingICUs);
  }
}

static void fillHolesHorizontally_360(AreaBuf<bool> &requestedIcus,
                                      int rowIndex, int &n_requestedICUs) {
  int iWidth = int(
      requestedIcus
          .width); // if we do not convert to int, the traversing equation for
  // 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) %
  // iWidth) converts everything to UInt

  bool *requested_rowPtr = &requestedIcus.at(0, rowIndex);
  int x_pre, x_next;
  for (int x = 0; x < iWidth; x++) {
    x_pre =
        (iWidth + (int(x - 1) % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    x_next =
        (iWidth + (int(x + 1) % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    if (!requested_rowPtr[x]) // if the icu is false, but the previous icu and
    // the next icu are true, then fill the hole
    {
      if (requested_rowPtr[x_pre] && requested_rowPtr[x_next]) {
        requested_rowPtr[x] = true;
        n_requestedICUs++;
      }
    }
  }
}

void icov::video::EncodingStructure::
    create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
        ChromaFormat chFmt, const UInt sourceWidth_luma,
        const UInt sourceHeight_luma, const UInt sourceMargin_x_luma,
        const UInt icuWidth_luma, const UInt icuHeight_luma,
        const UInt tuWidth_luma, const UInt tuHeight_luma) {
  using namespace VTM_Utility;
  const bool si_createTransformBuffer = true;
  const bool si_createQuantizedTransformBuffer = true;
  const bool si_createInverseQuantizedTransformBuffer = true;

  std::map<int, icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE>
      mapOfSidesCondition;
  mapOfSidesCondition[0] = BlockPrediction::LEFT;
  mapOfSidesCondition[1] = BlockPrediction::RIGHT;
  mapOfSidesCondition[2] = BlockPrediction::TOP;
  mapOfSidesCondition[3] = BlockPrediction::BOTTOM;
  mapOfSidesCondition[4] = BlockPrediction::TOP_LEFT;
  mapOfSidesCondition[5] = BlockPrediction::BOTTOM_LEFT;
  mapOfSidesCondition[6] = BlockPrediction::BOTTOM_RIGHT;
  mapOfSidesCondition[7] = BlockPrediction::TOP_RIGHT;

  unsigned int mapSize = mapOfSidesCondition.size();

  UInt nx_icus = (sourceWidth_luma + icuWidth_luma - 1) / icuWidth_luma;
  UInt ny_icus = (sourceHeight_luma + icuHeight_luma - 1) / icuHeight_luma;
  UInt n_icus = nx_icus * ny_icus;

  std::vector<IcuEncoder> icus_enc;

  icus_enc.reserve(n_icus);
  div_t divresult;

  Area extendedFrameArea_luma(0, 0, sourceWidth_luma + 2 * sourceMargin_x_luma,
                              sourceHeight_luma);
  for (UInt icuIndex = 0; icuIndex < n_icus; icuIndex++) {
    divresult = div(Int(icuIndex), Int(nx_icus));
    UInt icuXPosTopLeft = divresult.rem;
    UInt icuYPosTopLeft = divresult.quot;

    ICOV_LOG(trace) << "create ICU #" << icuIndex << " (" << icuXPosTopLeft
                    << ", " << icuYPosTopLeft << ")";

    Area icuLumaArea =
        Area(icuXPosTopLeft * icuWidth_luma + sourceMargin_x_luma,
             icuYPosTopLeft * icuHeight_luma, icuWidth_luma, icuHeight_luma) &
        extendedFrameArea_luma;

    try {
      CHECK(static_cast<UInt>(icuLumaArea.width) != icuWidth_luma ||
                static_cast<UInt>(icuLumaArea.height) != icuHeight_luma,
            "currently only frame which is divisible by icuWidth is supported");
    } catch (...) {
      std::cout << "...\n";
      return;
    }

    icus_enc.push_back(IcuEncoder(chFmt, icuLumaArea));
    IcuEncoder &icu_enc = icus_enc.back();
    icu_enc.createTUs(tuWidth_luma, tuHeight_luma);

    using SI_SIDE = icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE;

    for (UInt i = 0; i < mapSize; i++) {
      SI_SIDE refSamp = mapOfSidesCondition[i];

      if (
          // TOP
          !(icuYPosTopLeft == 0 && (refSamp & SI_SIDE::TOP)) &&
          // BOTTOM
          !(icuYPosTopLeft == (ny_icus - 1) && (refSamp & SI_SIDE::BOTTOM))

      ) {

        icu_enc.addIntraSI(refSamp, si_createTransformBuffer,
                           si_createQuantizedTransformBuffer,
                           si_createInverseQuantizedTransformBuffer);
      }
    }

    // give all possible SIs for non-border ICUs
    //    if ((icuYPosTopLeft > 0) &&
    //        (icuYPosTopLeft < ny_icus - 1)) // all icus except top and bottom
    //    {
    //      for (UInt i = 0; i < mapSize; i++) {
    //        icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE refSamp =
    //            mapOfSidesCondition[i];
    //        icu_enc.addIntraSI(refSamp, si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //      }
    //    } else {
    //      if (icuYPosTopLeft == 0) // top icus
    //      {
    //        icu_enc.addIntraSI(
    //            icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::LEFT,
    //            si_createTransformBuffer, si_createQuantizedTransformBuffer,
    //            si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(
    //            icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::RIGHT,
    //            si_createTransformBuffer, si_createQuantizedTransformBuffer,
    //            si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(
    //            icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::BOTTOM,
    //            si_createTransformBuffer, si_createQuantizedTransformBuffer,
    //            si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::BOTTOM_LEFT,
    //                           si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::BOTTOM_RIGHT,
    //                           si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //      } else {
    //        icu_enc.addIntraSI(icov::video::IntraPrediction::LEFT,
    //                           si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(
    //            icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::RIGHT,
    //            si_createTransformBuffer, si_createQuantizedTransformBuffer,
    //            si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(icov::video::IntraPrediction::TOP,
    //                           si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(icov::video::IntraPrediction::LEFT_TOP,
    //                           si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //        icu_enc.addIntraSI(icov::video::IntraPrediction::RIGHT_TOP,
    //                           si_createTransformBuffer,
    //                           si_createQuantizedTransformBuffer,
    //                           si_createInverseQuantizedTransformBuffer);
    //      }
    //    }
  }
  set(icus_enc, nx_icus, ny_icus);
}

void icov::video::EncodingStructure::updateRequestedICUs(
    const cv::Mat &maskRequestedPixels,
    std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
    std::vector<std::vector<Position>> &requestedICUs_closeTocheckPoints,
    std::vector<Position> &neighborcheckPointInTheComponent,
    double *usefulness_percent) {
  _requestedICUs.fill(false);

  _maskThresholded.create(maskRequestedPixels.size(), CV_8UC1);
  if (_maskThresholded.isContinuous())
    memset(_maskThresholded.ptr(), 0,
           _maskThresholded.step[0] *
               _maskThresholded
                   .rows); // _maskThresholded.step[0] * _maskThresholded.rows
                           // represents cv::Mat size in byte. It is almost 2
                           // times faster than setTo(0)
  else
    _maskThresholded.setTo(0);

  _maskThresholded.setTo(
      1,
      maskRequestedPixels); // This is done because in maskRequestedPixels
  // nonzero pixels have value 255, so in the next
  // cv::integral function overflow can happen for
  // very large images. That's why I changed nonzero
  // pixels to 1

  cv::integral(_maskThresholded, _integralImageMask, CV_32SC1);

  int x1 = 0, y1 = 0, x2, y2;
  int n_nonzero;
  int n_requestedICUs = 0;
  for (UInt y = 0; y < n_icusPerCol; y++) {
    bool *requested_rowPtr = &_requestedICUs.at(0, y);
    //        bool* previous_rowPtr = &_decodedICUs.at(0,y);
    for (UInt x = 0; x < n_icusPerRow; x++) {
      int i = y * n_icusPerRow + x;

      const InteractiveCodingUnit &icu = vec_icus[i].get_sourceX();
      // the reason that we can not use the x,y and of icu is because for
      // example for 360 images, first icu position starts from point
      // (margin_x,y), but maskRequestioPixels contains the main frame area with
      // no margins
      x2 = x1 + int(icu.Y().width);
      y2 = y1 + int(icu.Y().height);
      // to compute all values in range (x1 <= x < x2 && y1 <= y < y2), i.e. x2,
      // y2 not included
      //

      n_nonzero = _integralImageMask.at<int>(y2, x2) -
                  _integralImageMask.at<int>(y2, x1) -
                  _integralImageMask.at<int>(y1, x2) +
                  _integralImageMask.at<int>(y1, x1);

      _vec_pixelsRequestedPerICU[i] = n_nonzero;

      if (n_nonzero /*&& (!previous_rowPtr[x])*/) // if it is requested but it
      // is not already decoded
      {
        requested_rowPtr[x] = true;
        n_requestedICUs++;
      }

      x1 = x2;
    }
    x1 = 0;
    y1 = y2;
  }

  //    {
  //        cv::namedWindow("requested before checking decoded ICUS",
  //        CV_WINDOW_NORMAL); cv::imshow("requested before checking decoded
  //        ICUS", VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  /*
      // apply closing operation (morphological transformation) on the top row
     and bottom row because on top and bottom there are lots of discontinuities
      fillHolesHorizontally_360(_requestedICUs, 0, n_requestedICUs);
      fillHolesHorizontally_360(_requestedICUs, n_icusPerCol-1,
     n_requestedICUs);
  */

  //    {
  //        cv::namedWindow("requested after closing", CV_WINDOW_NORMAL);
  //        cv::imshow("requested after closing",
  //        VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  int iWidth = int(n_icusPerRow); // if we do not convert to int, the traversing
                                  // equation for 360 video (neighbor_x =
                                  // (iWidth + (neighbor_x%iWidth)) % iWidth)
                                  // converts everything to UInt
  int iHeight = int(n_icusPerCol);
  bool neighborAlreadyDecoded;
  requestedICUs_closeToDecodedICUs.clear();
  int n_remainingRequestedICUs = n_requestedICUs;
  for (UInt y = 0; y < n_icusPerCol;
       y++) // check for requested ICUs where there is at least an already
  // decoded ICU there
  {
    const bool *requested_rowPtr = &_requestedICUs.at(0, y);
    const bool *decoded_rowPtr = &_decodedICUs.at(0, y);
    for (UInt x = 0; x < n_icusPerRow; x++) {
      if (requested_rowPtr[x] &&
          decoded_rowPtr[x]) // I used || (or) instead of && (and) because I
      // saw some situations where a block beside the
      // requested blocks (not inside the requested
      // blocks) has been already decoded but there is
      // no decoded block inside. next line afte dfs_360
      // if there was no requested block in the
      // connected component I pop_back the empty vector
      {
        requestedICUs_closeToDecodedICUs.push_back(std::vector<Position>());
        dfs_360(_requestedICUs, _decodedICUs,
                requestedICUs_closeToDecodedICUs.back(), x, y,
                n_remainingRequestedICUs);
      }

      if (requested_rowPtr[x]) {
        neighborAlreadyDecoded = false;
        if (decoded_rowPtr[x])
          neighborAlreadyDecoded = true;
        else {
          for (int i = 0; i < 4; i++) {
            int neighbor_x = x + dx[i];
            neighbor_x = (iWidth + (neighbor_x % iWidth)) %
                         iWidth; // to generate circular traversing for 360
            // video in x direction
            int neighbor_y = y + dy[i];
            if (neighbor_y < 0 ||
                neighbor_y >=
                    iHeight) // do not check outside image in y direction
              continue;
            if (_decodedICUs.at(neighbor_x, neighbor_y)) {
              neighborAlreadyDecoded = true;
              break;
            }
          }
        }
        if (neighborAlreadyDecoded) {
          requestedICUs_closeToDecodedICUs.push_back(std::vector<Position>());
          dfs_360(_requestedICUs, _decodedICUs,
                  requestedICUs_closeToDecodedICUs.back(), x, y,
                  n_remainingRequestedICUs);
        }
      }
    }
  }

  //    {
  //        cv::namedWindow("requested after checking decoded ICUS",
  //        CV_WINDOW_NORMAL); cv::imshow("requested after checking decoded
  //        ICUS", VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  CHECK(n_remainingRequestedICUs < 0 ||
            n_remainingRequestedICUs > n_requestedICUs,
        "there is a bug. it shouldn't be negative");

  // Note, don't forget that dfs_360 changes the values of already traveresed
  // blocks to false, so the following loop checks only the remaining

  requestedICUs_closeTocheckPoints.clear();
  neighborcheckPointInTheComponent.clear();
  if (n_remainingRequestedICUs > 0) // look for the remaining requested blocks
  // which there is a check point there
  {
    // the number of check points is less than number of pixels. so it is better
    // to traveresed through checkpoints this time
    for (UInt chk = 0; chk < vec_checkPoints.size(); chk++) {
      const Position &chPoint_pos = vec_checkPoints[chk];
      if (_requestedICUs.at(chPoint_pos.x, chPoint_pos.y)) {
        neighborcheckPointInTheComponent.push_back(chPoint_pos);
        requestedICUs_closeTocheckPoints.push_back(std::vector<Position>());
        dfs_360(_requestedICUs, chPoint_pos,
                requestedICUs_closeTocheckPoints.back(), chPoint_pos.x,
                chPoint_pos.y, n_remainingRequestedICUs);
      }
    }
  }

  //    {
  //        cv::namedWindow("requested after checking checkpoints",
  //        CV_WINDOW_NORMAL); cv::imshow("requested after checking
  //        checkpoints", VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  // check the top and the bottom row to see if the remaining blocks is there
  // and neglect them if they contain less than thresholdToNeglectICU pixels.
  // This is because on top and botoom row there is a huge discontinuity
  int thresholdToNeglectICU = 15;

  if (n_remainingRequestedICUs != 0) {
    UInt halfRows =
        (n_icusPerCol % 2 == 0) ? n_icusPerCol / 2 : n_icusPerCol / 2 + 1;
    UInt y;
    UInt icu_index;
    for (UInt i = 0; i < halfRows; i++) {
      y = i;
      bool *requested_rowPtrTop = &_requestedICUs.at(0, y);
      for (UInt x = 0; x < n_icusPerRow; x++) {
        if (requested_rowPtrTop[x]) // if on the top row there is a block which
        // is not assigned to any of
        // requestedICUs_closeToDecodedICUs,
        // requestedICUs_closeTocheckPoints,
        // neighborcheckPointInTheComponent
        {
          icu_index = y * n_icusPerRow + x;
          if (_vec_pixelsRequestedPerICU[icu_index] < thresholdToNeglectICU) {
            n_remainingRequestedICUs--;
            requested_rowPtrTop[x] = false;

            if (n_remainingRequestedICUs == 0)
              break;
          } else
            std::cerr
                << "number of pixels in the ICU which is neglected is to many:"
                << _vec_pixelsRequestedPerICU[icu_index] << std::endl;
        }
      }

      if (n_remainingRequestedICUs == 0)
        break;

      y = n_icusPerCol - 1 - i;
      bool *requested_rowPtrBottom = &_requestedICUs.at(0, y);
      for (UInt x = 0; x < n_icusPerRow; x++) {
        if (requested_rowPtrBottom[x]) // if on the top row there is a block
        // which is not assigned to any of
        // requestedICUs_closeToDecodedICUs,
        // requestedICUs_closeTocheckPoints,
        // neighborcheckPointInTheComponent
        {
          icu_index = y * n_icusPerRow + x;
          if (_vec_pixelsRequestedPerICU[icu_index] < thresholdToNeglectICU) {
            n_remainingRequestedICUs--;
            requested_rowPtrBottom[x] = false;

            if (n_remainingRequestedICUs == 0)
              break;
          } else
            std::cerr
                << "number of pixels in the ICU which is neglected is to many:"
                << _vec_pixelsRequestedPerICU[icu_index] << std::endl;
        }
      }

      if (n_remainingRequestedICUs == 0)
        break;
    }
  }
  CHECK(n_remainingRequestedICUs != 0, "not enough check points");

  if (usefulness_percent) {
    UInt sum_pixels = 0;
    UInt sum_icusArea = 0;
    UInt icu_index;

    // std::vector<std::vector<Position> > &requestedICUs_closeToDecodedICUs,
    // std::vector<std::vector<Position> > &requestedICUs_closeTocheckPoints,
    // std::vector<Position> &neighborcheckPointInTheComponent
    for (unsigned int i = 0; i < requestedICUs_closeToDecodedICUs.size(); i++) {
      for (unsigned int j = 0; j < requestedICUs_closeToDecodedICUs[i].size();
           j++) {
        icu_index = get_icuIndex(requestedICUs_closeToDecodedICUs[i][j]);
        sum_pixels += _vec_pixelsRequestedPerICU[icu_index];
        sum_icusArea += getICU(icu_index).get_sourceX().Y().area();
      }
    }

    for (unsigned int i = 0; i < requestedICUs_closeTocheckPoints.size(); i++) {
      icu_index = get_icuIndex(neighborcheckPointInTheComponent[i]);
      sum_pixels += _vec_pixelsRequestedPerICU[icu_index];
      sum_icusArea += getICU(icu_index).get_sourceX().Y().area();

      for (unsigned int j = 0; j < requestedICUs_closeTocheckPoints[i].size();
           j++) {
        icu_index = get_icuIndex(requestedICUs_closeTocheckPoints[i][j]);
        sum_pixels += _vec_pixelsRequestedPerICU[icu_index];
        sum_icusArea += getICU(icu_index).get_sourceX().Y().area();
      }
    }

    if (sum_icusArea != 0)
      *usefulness_percent = double(sum_pixels) / double(sum_icusArea);
    else
      *usefulness_percent = 0.;
  }
}

void icov::video::EncodingStructure::updateRequestedICUs(
    const cv::Mat &maskRequestedPixels,
    std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
    std::vector<std::vector<Position>> &requestedICUs_closeTocheckPoints,
    std::vector<Position> &neighborcheckPointInTheComponent,
    std::vector<Position> &closestCheckPointToTheRemainingICUs,
    std::vector<std::vector<Position>>
        &remainingICUS_afterClosestCheckPointToBeDecoded,
    std::vector<std::vector<Position>>
        &remainingICUS_afterAlreadyProcessedICUsToBeDecoded) {
  _requestedICUs.fill(false);
  resetAlreadyProcessedICUs();
  copyProccessedICUsFromDecodedICUs();

  _maskThresholded.create(maskRequestedPixels.size(), CV_8UC1);
  if (_maskThresholded.isContinuous())
    memset(_maskThresholded.ptr(), 0,
           _maskThresholded.step[0] *
               _maskThresholded
                   .rows); // _maskThresholded.step[0] * _maskThresholded.rows
                           // represents cv::Mat size in byte. It is almost 2
                           // times faster than setTo(0)
  else
    _maskThresholded.setTo(0);

  _maskThresholded.setTo(
      1,
      maskRequestedPixels); // This is done because in maskRequestedPixels
  // nonzero pixels have value 255, so in the next
  // cv::integral function overflow can happen for
  // very large images. That's why I changed nonzero
  // pixels to 1

  cv::integral(_maskThresholded, _integralImageMask, CV_32SC1);

  int x1 = 0, y1 = 0, x2, y2;
  int n_nonzero;
  int n_requestedICUs = 0;
  for (UInt y = 0; y < n_icusPerCol; y++) {
    bool *requested_rowPtr = &_requestedICUs.at(0, y);
    //        bool* previous_rowPtr = &_decodedICUs.at(0,y);
    for (UInt x = 0; x < n_icusPerRow; x++) {
      int i = y * n_icusPerRow + x;

      const InteractiveCodingUnit &icu = vec_icus[i].get_sourceX();
      // the reason that we can not use the x,y and of icu is because for
      // example for 360 images, first icu position starts from point
      // (margin_x,y), but maskRequestioPixels contains the main frame area with
      // no margins
      x2 = x1 + int(icu.Y().width);
      y2 = y1 + int(icu.Y().height);
      // to compute all values in range (x1 <= x < x2 && y1 <= y < y2), i.e. x2,
      // y2 not included
      //

      n_nonzero = _integralImageMask.at<int>(y2, x2) -
                  _integralImageMask.at<int>(y2, x1) -
                  _integralImageMask.at<int>(y1, x2) +
                  _integralImageMask.at<int>(y1, x1);

      _vec_pixelsRequestedPerICU[i] = n_nonzero;

      if (n_nonzero /*&& (!previous_rowPtr[x])*/) // if it is requested but it
      // is not already decoded
      {
        requested_rowPtr[x] = true;
        n_requestedICUs++;
      }

      x1 = x2;
    }
    x1 = 0;
    y1 = y2;
  }

  //    {
  //        cv::namedWindow("requested before checking decoded ICUS",
  //        CV_WINDOW_NORMAL); cv::imshow("requested before checking decoded
  //        ICUS", VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  /*
      // apply closing operation (morphological transformation) on the top row
     and bottom row because on top and bottom there are lots of discontinuities
      fillHolesHorizontally_360(_requestedICUs, 0, n_requestedICUs);
      fillHolesHorizontally_360(_requestedICUs, n_icusPerCol-1,
     n_requestedICUs);
  */

  //    {
  //        cv::namedWindow("requested after closing", CV_WINDOW_NORMAL);
  //        cv::imshow("requested after closing",
  //        VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  int iWidth = int(n_icusPerRow); // if we do not convert to int, the traversing
                                  // equation for 360 video (neighbor_x =
                                  // (iWidth + (neighbor_x%iWidth)) % iWidth)
                                  // converts everything to UInt
  int iHeight = int(n_icusPerCol);
  bool neighborAlreadyDecoded;
  requestedICUs_closeToDecodedICUs.clear();
  int n_remainingRequestedICUs = n_requestedICUs;
  for (UInt y = 0; y < n_icusPerCol;
       y++) // check for requested ICUs where there is at least an already
  // decoded ICU there
  {
    const bool *requested_rowPtr = &_requestedICUs.at(0, y);
    const bool *decoded_rowPtr = &_decodedICUs.at(0, y);
    for (UInt x = 0; x < n_icusPerRow; x++) {
      if (requested_rowPtr[x] &&
          decoded_rowPtr[x]) // I used || (or) instead of && (and) because I
      // saw some situations where a block beside the
      // requested blocks (not inside the requested
      // blocks) has been already decoded but there is
      // no decoded block inside. next line afte dfs_360
      // if there was no requested block in the
      // connected component I pop_back the empty vector
      {
        requestedICUs_closeToDecodedICUs.push_back(std::vector<Position>());
        dfs_360(_requestedICUs, _decodedICUs,
                requestedICUs_closeToDecodedICUs.back(), x, y,
                n_remainingRequestedICUs);

        const std::vector<Position> &currAddedBlocks =
            requestedICUs_closeToDecodedICUs.back();
        for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
          updateAlreadyProccessedICU(currAddedBlocks[ii]);
      }

      if (requested_rowPtr[x]) {
        neighborAlreadyDecoded = false;
        if (decoded_rowPtr[x])
          neighborAlreadyDecoded = true;
        else {
          for (int i = 0; i < 4; i++) {
            int neighbor_x = x + dx[i];
            neighbor_x = (iWidth + (neighbor_x % iWidth)) %
                         iWidth; // to generate circular traversing for 360
            // video in x direction
            int neighbor_y = y + dy[i];
            if (neighbor_y < 0 ||
                neighbor_y >=
                    iHeight) // do not check outside image in y direction
              continue;
            if (_decodedICUs.at(neighbor_x, neighbor_y)) {
              neighborAlreadyDecoded = true;
              break;
            }
          }
        }
        if (neighborAlreadyDecoded) {
          requestedICUs_closeToDecodedICUs.push_back(std::vector<Position>());
          dfs_360(_requestedICUs, _decodedICUs,
                  requestedICUs_closeToDecodedICUs.back(), x, y,
                  n_remainingRequestedICUs);

          const std::vector<Position> &currAddedBlocks =
              requestedICUs_closeToDecodedICUs.back();
          for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
            updateAlreadyProccessedICU(currAddedBlocks[ii]);
        }
      }
    }
  }

  //    {
  //        cv::namedWindow("requested after checking decoded ICUS",
  //        CV_WINDOW_NORMAL); cv::imshow("requested after checking decoded
  //        ICUS", VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  CHECK(n_remainingRequestedICUs < 0 ||
            n_remainingRequestedICUs > n_requestedICUs,
        "there is a bug. it shouldn't be negative");

  // Note, don't forget that dfs_360 changes the values of already traveresed
  // blocks to false, so the following loop checks only the remaining

  requestedICUs_closeTocheckPoints.clear();
  neighborcheckPointInTheComponent.clear();
  if (n_remainingRequestedICUs > 0) // look for the remaining requested blocks
  // which there is a check point there
  {
    // the number of check points is less than number of pixels. so it is better
    // to traveresed through checkpoints this time
    for (UInt chk = 0; chk < vec_checkPoints.size(); chk++) {
      const Position &chPoint_pos = vec_checkPoints[chk];
      if (_requestedICUs.at(chPoint_pos.x, chPoint_pos.y)) {
        neighborcheckPointInTheComponent.push_back(chPoint_pos);
        requestedICUs_closeTocheckPoints.push_back(std::vector<Position>());
        dfs_360(_requestedICUs, chPoint_pos,
                requestedICUs_closeTocheckPoints.back(), chPoint_pos.x,
                chPoint_pos.y, n_remainingRequestedICUs);

        updateAlreadyProccessedICU(chPoint_pos);
        const std::vector<Position> &currAddedBlocks =
            requestedICUs_closeTocheckPoints.back();
        for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
          updateAlreadyProccessedICU(currAddedBlocks[ii]);
      }
    }
  }

  //    {
  //        cv::namedWindow("requested after checking checkpoints",
  //        CV_WINDOW_NORMAL); cv::imshow("requested after checking
  //        checkpoints", VTM_OPENCV_Utility::toMat<bool>(_requestedICUs)*255);
  //        cv::waitKey(0);
  //    }

  remainingICUS_afterClosestCheckPointToBeDecoded.clear();
  closestCheckPointToTheRemainingICUs.clear();
  remainingICUS_afterAlreadyProcessedICUsToBeDecoded.clear();
  if (n_remainingRequestedICUs >
      0) // look for the remaining requested blocks to see which already
  // proccessed blocks are closest to requested connected blocks and
  // then add the blocks in between to arrive to the requested region of
  // blocks
  {
    int n_remainingRequestedICUs_temp = n_remainingRequestedICUs;
    std::vector<std::vector<Position>> remaining_connected_blocks;
    for (UInt y = 0; y < n_icusPerCol;
         y++) // check for the remaining requested blocks
    {
      const bool *requested_rowPtr = &_requestedICUs.at(0, y);
      for (UInt x = 0; x < n_icusPerRow; x++) {
        if (requested_rowPtr[x]) {
          remaining_connected_blocks.push_back(std::vector<Position>());
          dfs_360(_requestedICUs, remaining_connected_blocks.back(), x, y,
                  n_remainingRequestedICUs);
        }
      }
    }

    // Sounds funny but I have to fill back the _requestedICUs buffer with the
    // remaining_connected_blocks, because dfs_360 set the blocks in
    // remaining_connected_blocks to false
    for (unsigned int cc_id = 0; cc_id < remaining_connected_blocks.size();
         cc_id++)
      for (unsigned int block_id = 0;
           block_id < remaining_connected_blocks[cc_id].size(); block_id++)
        _requestedICUs.at(remaining_connected_blocks[cc_id][block_id]) = true;

    unsigned int n_CC = remaining_connected_blocks.size();
    std::vector<unsigned int> unprocessedCC_ids(n_CC);
    for (unsigned int cc_id = 0; cc_id < remaining_connected_blocks.size();
         cc_id++)
      unprocessedCC_ids[cc_id] = cc_id;

    while (unprocessedCC_ids.size()) {
      bool a_processed_block_in_the_requested_region = false;
      Position proccessedBlockInRequestedRegion(-1, -1);

      for (const unsigned int &patch_id : unprocessedCC_ids) {
        const std::vector<Position> &currentCC =
            remaining_connected_blocks[patch_id];
        for (unsigned int block_id = 0; block_id < currentCC.size();
             block_id++) {
          if (isICUAlreadyProcessed(currentCC[block_id])) {
            a_processed_block_in_the_requested_region = true;
            proccessedBlockInRequestedRegion = currentCC[block_id];
            break;
          }
        }

        if (a_processed_block_in_the_requested_region) {
          CHECK(!_requestedICUs.at(proccessedBlockInRequestedRegion.x,
                                   proccessedBlockInRequestedRegion.y) ||
                    !_tmp_alreadyProcessedICUs.at(
                        proccessedBlockInRequestedRegion.x,
                        proccessedBlockInRequestedRegion.y),
                "Something is wrong. An already processed block must be in the "
                "requested region.");

          remainingICUS_afterAlreadyProcessedICUsToBeDecoded.push_back(
              std::vector<Position>());
          dfs_360(_requestedICUs, _tmp_alreadyProcessedICUs,
                  remainingICUS_afterAlreadyProcessedICUsToBeDecoded.back(),
                  proccessedBlockInRequestedRegion.x,
                  proccessedBlockInRequestedRegion.y,
                  n_remainingRequestedICUs_temp);

          const std::vector<Position> &currAddedBlocks =
              remainingICUS_afterAlreadyProcessedICUsToBeDecoded.back();
          for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
            updateAlreadyProccessedICU(currAddedBlocks[ii]);

          unprocessedCC_ids.erase(remove(unprocessedCC_ids.begin(),
                                         unprocessedCC_ids.end(), patch_id),
                                  unprocessedCC_ids.end());
          break;
        }
      }

      if (!a_processed_block_in_the_requested_region) // No already processed
      // block in the requested
      // region. So start
      // looking for the
      // closest access block
      {
        const unsigned int &patch_id = unprocessedCC_ids[0];
        const std::vector<Position> &currentCC =
            remaining_connected_blocks[patch_id];
        unsigned int chosen_block_id = 0;
        unsigned int chosen_access_block_id = 0;
        unsigned int minimum_distance_squared =
            std::numeric_limits<unsigned int>::max();
        unsigned int distance_squared;
        Position diff;
        for (unsigned int block_id = 0; block_id < currentCC.size();
             block_id++) {
          for (unsigned int access_block_id = 0;
               access_block_id < vec_checkPoints.size(); access_block_id++) {
            diff = currentCC[block_id] - vec_checkPoints[access_block_id];
            distance_squared = static_cast<unsigned int>(diff.x * diff.x) +
                               static_cast<unsigned int>(diff.y * diff.y);
            if (distance_squared < minimum_distance_squared) {
              minimum_distance_squared = distance_squared;
              chosen_block_id = block_id;
              chosen_access_block_id = access_block_id;
            }
          }
        }

        const Position &chosen_block_pos = currentCC[chosen_block_id];
        const Position &chosen_access_block_pos =
            vec_checkPoints[chosen_access_block_id];
        // check if any block in between is already processsed

        Position diff_chosen_blocks =
            chosen_access_block_pos - chosen_block_pos;
        const int dir_x_to_access_block =
            (diff_chosen_blocks.x < 0)
                ? -1
                : 1; // direction x to go from chosen requested block to the
        // chosen access block
        const int dir_y_to_access_block =
            (diff_chosen_blocks.y < 0)
                ? -1
                : 1; // direction y to go from chosen requested block to the
        // chosen access block
        Position abs_diff_chosen_block(abs(diff_chosen_blocks.x),
                                       abs(diff_chosen_blocks.y));

        Position processedBlock(-1, -1);
        bool isAnyProccessedBlockInBetween = false;

        // Traverse in "L-shape" from chosen block to access blocka nd to see if
        // there is any processed block in between.
        for (int abs_diff_x = 1; abs_diff_x <= abs_diff_chosen_block.x;
             abs_diff_x++) {
          int diff_x = dir_x_to_access_block * abs_diff_x;
          Position current_pos(chosen_block_pos.x + diff_x, chosen_block_pos.y);
          if (isICUAlreadyProcessed(current_pos)) {
            isAnyProccessedBlockInBetween = true;
            processedBlock = current_pos;
            break;
          }
        }
        if (!isAnyProccessedBlockInBetween) // if no already processed in the
        // horizontal direction, then check
        // the vertical direction to arrive
        // to access block
        {
          for (int abs_diff_y = 1; abs_diff_y <= abs_diff_chosen_block.y;
               abs_diff_y++) // <=: because = means if the chosen access block
          // is already decoded, there is no need to send it
          // as an access block
          {
            int diff_y = dir_y_to_access_block * abs_diff_y;
            Position current_pos(chosen_access_block_pos.x,
                                 chosen_block_pos.y + diff_y);
            if (isICUAlreadyProcessed(current_pos)) {
              isAnyProccessedBlockInBetween = true;
              processedBlock = current_pos;
              break;
            }
          }
        }

        if (isAnyProccessedBlockInBetween) {
          remainingICUS_afterAlreadyProcessedICUsToBeDecoded.push_back(
              std::vector<Position>());
          std::vector<Position> &blocks_to_be_added =
              remainingICUS_afterAlreadyProcessedICUsToBeDecoded.back();

          Position diff_processed_to_requested_blocks =
              chosen_block_pos - processedBlock;
          const int dir_x_to_requested_block =
              (diff_processed_to_requested_blocks.x >= 0) ? 1 : -1;
          const int dir_y_to_requested_block =
              (diff_processed_to_requested_blocks.y >= 0) ? 1 : -1;
          Position abs_diff_processed_to_requested_blocks(
              abs(diff_processed_to_requested_blocks.x),
              abs(diff_processed_to_requested_blocks.y));
          for (int abs_diff_y = 1;
               abs_diff_y <= abs_diff_processed_to_requested_blocks.y;
               abs_diff_y++) {
            int diff_y = dir_y_to_requested_block * abs_diff_y;
            Position current_pos(processedBlock.x, processedBlock.y + diff_y);
            blocks_to_be_added.push_back(current_pos);
            updateAlreadyProccessedICU(current_pos);
          }
          for (int abs_diff_x = 1;
               abs_diff_x < abs_diff_processed_to_requested_blocks.x;
               abs_diff_x++) // less than "<" for
          // abs_diff_processed_to_requested_blocks.x rather
          // than less than or equal "=<", because the next
          // block is in the requested region
          {
            int diff_x = dir_x_to_requested_block * abs_diff_x;
            Position current_pos(processedBlock.x + diff_x, chosen_block_pos.y);
            blocks_to_be_added.push_back(current_pos);
            updateAlreadyProccessedICU(current_pos);
          }

          dfs_360(_requestedICUs, _tmp_alreadyProcessedICUs, blocks_to_be_added,
                  chosen_block_pos.x, chosen_block_pos.y,
                  n_remainingRequestedICUs_temp);

          const std::vector<Position> &currAddedBlocks = blocks_to_be_added;
          for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
            updateAlreadyProccessedICU(currAddedBlocks[ii]);

        } else {
          bool neighboringOfAccessBlockAlreadyDecoded = false;
          for (int ii = 0; ii < 4; ii++) {
            int neighbor_x = chosen_access_block_pos.x + dx[ii];
            neighbor_x = (iWidth + (neighbor_x % iWidth)) %
                         iWidth; // to generate circular traversing for 360
            // video in x direction
            int neighbor_y = chosen_access_block_pos.y + dy[ii];
            if (neighbor_y < 0 ||
                neighbor_y >=
                    iHeight) // do not check outside image in y direction
              continue;
            if (_tmp_alreadyProcessedICUs.at(neighbor_x, neighbor_y)) {
              neighboringOfAccessBlockAlreadyDecoded = true;
              break;
            }
          }

          const int dir_x_to_requested_block =
              -dir_x_to_access_block; // direction x to go from the chosen
          // access block to the requested block
          const int dir_y_to_requested_block =
              -dir_y_to_access_block; // direction y to go from the chosen
          // access block to the requested block

          if (neighboringOfAccessBlockAlreadyDecoded) {
            remainingICUS_afterAlreadyProcessedICUsToBeDecoded.push_back(
                std::vector<Position>());
            std::vector<Position> &blocks_to_be_added =
                remainingICUS_afterAlreadyProcessedICUsToBeDecoded.back();
            blocks_to_be_added.push_back(chosen_access_block_pos);

            for (int abs_diff_y = 1; abs_diff_y <= abs_diff_chosen_block.y;
                 abs_diff_y++) {
              int diff_y = dir_y_to_requested_block * abs_diff_y;
              Position current_pos(chosen_access_block_pos.x,
                                   chosen_access_block_pos.y + diff_y);
              blocks_to_be_added.push_back(current_pos);
              updateAlreadyProccessedICU(current_pos);
            }
            for (int abs_diff_x = 1; abs_diff_x < abs_diff_chosen_block.x;
                 abs_diff_x++) // less than "<" for
            // abs_diff_processed_to_requested_blocks.x
            // rather than less than or equal "=<", because
            // the next block is in the requested region
            {
              int diff_x = dir_x_to_requested_block * abs_diff_x;
              Position current_pos(chosen_access_block_pos.x + diff_x,
                                   chosen_block_pos.y);
              blocks_to_be_added.push_back(current_pos);
              updateAlreadyProccessedICU(current_pos);
            }

            dfs_360(_requestedICUs, _tmp_alreadyProcessedICUs,
                    blocks_to_be_added, chosen_block_pos.x, chosen_block_pos.y,
                    n_remainingRequestedICUs_temp);

            const std::vector<Position> &currAddedBlocks = blocks_to_be_added;
            for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
              updateAlreadyProccessedICU(currAddedBlocks[ii]);

          } else {
            closestCheckPointToTheRemainingICUs.push_back(
                chosen_access_block_pos);
            remainingICUS_afterClosestCheckPointToBeDecoded.push_back(
                std::vector<Position>());
            std::vector<Position> &blocks_to_be_added =
                remainingICUS_afterClosestCheckPointToBeDecoded.back();

            for (int abs_diff_y = 1; abs_diff_y <= abs_diff_chosen_block.y;
                 abs_diff_y++) {
              int diff_y = dir_y_to_requested_block * abs_diff_y;
              Position current_pos(chosen_access_block_pos.x,
                                   chosen_access_block_pos.y + diff_y);
              blocks_to_be_added.push_back(current_pos);
              updateAlreadyProccessedICU(current_pos);
            }
            for (int abs_diff_x = 1; abs_diff_x < abs_diff_chosen_block.x;
                 abs_diff_x++) // less than "<" for
            // abs_diff_processed_to_requested_blocks.x
            // rather than less than or equal "=<", because
            // the next block is in the requested region
            {
              int diff_x = dir_x_to_requested_block * abs_diff_x;
              Position current_pos(chosen_access_block_pos.x + diff_x,
                                   chosen_block_pos.y);
              blocks_to_be_added.push_back(current_pos);
              updateAlreadyProccessedICU(current_pos);
            }

            dfs_360(_requestedICUs, _tmp_alreadyProcessedICUs,
                    blocks_to_be_added, chosen_block_pos.x, chosen_block_pos.y,
                    n_remainingRequestedICUs_temp);

            updateAlreadyProccessedICU(chosen_access_block_pos);
            const std::vector<Position> &currAddedBlocks = blocks_to_be_added;
            for (unsigned int ii = 0; ii < currAddedBlocks.size(); ii++)
              updateAlreadyProccessedICU(currAddedBlocks[ii]);
          }
        }

        unprocessedCC_ids.erase(remove(unprocessedCC_ids.begin(),
                                       unprocessedCC_ids.end(), patch_id),
                                unprocessedCC_ids.end());
      }
    }
  }

  CHECK(n_remainingRequestedICUs != 0, "not enough check points");
}

void icov::video::EncodingStructure::
    reorderBlockDecodingBasedOnStartingAccessBlock(
        std::vector<std::vector<Position>> &in_out_requested_icus,
        const std::vector<Position> &accessBlockInsideAsStartingPosition) {
  CHECK(in_out_requested_icus.size() !=
            accessBlockInsideAsStartingPosition.size(),
        "Size of the vector must be equal.");
  _requestedICUs.fill(false);

  int n_remainingRequestedICUs = 0;

  for (unsigned int cc_id = 0;
       cc_id < accessBlockInsideAsStartingPosition.size(); cc_id++) {
    if (!isAccessBlock(accessBlockInsideAsStartingPosition[cc_id]))
      throw std::runtime_error("This is not an access block.");

    n_remainingRequestedICUs += int(in_out_requested_icus[cc_id].size());

    for (unsigned int block_ind = 0;
         block_ind < in_out_requested_icus[cc_id].size(); block_ind++)
      _requestedICUs.at(in_out_requested_icus[cc_id][block_ind]) = true;

    if (!_requestedICUs.at(accessBlockInsideAsStartingPosition[cc_id])) {
      n_remainingRequestedICUs++;
      _requestedICUs.at(accessBlockInsideAsStartingPosition[cc_id]) = true;
    }
  }

  in_out_requested_icus.clear();
  for (unsigned int cc_id = 0;
       cc_id < accessBlockInsideAsStartingPosition.size(); cc_id++) {
    const Position &chPoint_pos = accessBlockInsideAsStartingPosition[cc_id];
    if (_requestedICUs.at(chPoint_pos.x, chPoint_pos.y)) {
      in_out_requested_icus.push_back(std::vector<Position>());
      dfs_360(_requestedICUs, chPoint_pos, in_out_requested_icus.back(),
              chPoint_pos.x, chPoint_pos.y, n_remainingRequestedICUs);
      //            Check the above line
    }
  }

  CHECK(n_remainingRequestedICUs != 0,
        "something is missing, it should process all input blocks.");
}

static void
dfs_360(AreaBuf<bool> &requestedIcus, const AreaBuf<bool> &alreadyDecodedICus,
        std::vector<Position> &currentComponent_requestedNotDecodedICUs,
        std::vector<Position> &currentComponent_requestedDecodedICUs, int x,
        int y) {
  if (!alreadyDecodedICus.at(x, y)) // add only if it is not decoded
    currentComponent_requestedNotDecodedICUs.push_back(Position(x, y));
  else
    currentComponent_requestedDecodedICUs.push_back(Position(x, y));

  requestedIcus.at(x, y) = false;
  int iWidth = int(
      requestedIcus
          .width); // if we do not convert to int, the traversing equation for
  // 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) %
  // iWidth) converts everything to UInt
  int iHeight = int(requestedIcus.height);
  for (int i = 0; i < 4; i++) {
    int neighbor_x = x + dx[i];
    neighbor_x =
        (iWidth + (neighbor_x % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    int neighbor_y = y + dy[i];
    if (neighbor_y < 0 ||
        neighbor_y >= iHeight) // do not check outside image in y direction
      continue;
    if (requestedIcus.at(neighbor_x, neighbor_y))
      dfs_360(requestedIcus, alreadyDecodedICus,
              currentComponent_requestedNotDecodedICUs,
              currentComponent_requestedDecodedICUs, neighbor_x, neighbor_y);
  }
}

void icov::video::EncodingStructure::applyPredictionBasedOnEntropy(
    icov::video::BufGroups &encpic, icov::video::IntraPrediction &intraPred,
    const ComponentID compID, const int bitDepths[MAX_NUM_CHANNEL_TYPE],
    const UInt tuWidth_luma, const UInt tuHeight_luma,
    bool encodeFrequencyTableInBitStream, const int qp,
    int maxLog2TrDynamicRange) {

  auto reconstructionUnitBuffer = encpic.getBuf(PIC_RECONSTRUCTION);
  ChromaFormat chromaFormatIDC = getICU(0).get_sourceX().chromaFormat;
  UInt tuWidth_component =
      tuWidth_luma >> getComponentScaleX(compID, chromaFormatIDC);
  UInt tuHeight_component =
      tuHeight_luma >> getComponentScaleY(compID, chromaFormatIDC);
  if (tuWidth_component == 0)
    tuWidth_component = 1;
  if (tuHeight_component == 0)
    tuHeight_component = 1;
  UInt nBands = tuWidth_component * tuHeight_component;

  std::vector<TCoeff> transformSequence_sourceX, transformSequence_sourceY;
  std::vector<int> residual;

  ChannelType chType = toChannelType(compID);
  double entropy, currEntropy;

  Transform2D transform2D;
  Quant quant;
  TCoeff uiAbsSum;
  QpParam cQP(qp, toChannelType(compID), getICU(0).get_sourceX().chromaFormat,
              bitDepths[chType], 0, 0);

  std::vector<UInt> availableDirModes;
  if (chType == CHANNEL_TYPE_LUMA) {
    availableDirModes.reserve(NUM_LUMA_MODE);
    for (UInt dirMode = 0; dirMode < NUM_LUMA_MODE; dirMode++)
      availableDirModes.push_back(dirMode);
  } else
    availableDirModes = {DM_CHROMA_IDX, PLANAR_IDX, DC_IDX, HOR_IDX, VER_IDX};

  for (UInt i = 0; i < vec_icus.size(); i++) {
    //    std::cout << "-- block is " << i << std::endl;
    int progress = (i * 100) / (vec_icus.size() - 1);
    std::cout << "    " << progress << "%                \r" << std::flush;
    IcuEncoder &icu_enc = vec_icus[i];
    std::map<BlockPrediction::predType, SideInformationUnit> &currICUintraMap =
        icu_enc.get_intraSIsMap();

    const CompArea &compArea = icu_enc.get_sourceX().block(compID);

    // Iterate through all elements in std::map
    for (auto it = currICUintraMap.begin(); it != currICUintraMap.end();
         ++it) // loop over all intra side information
    {
      auto &si = it->second;
      const auto &refSampSide = it->first;

      uint picId = PIC_PREDICTION + refSampSide;
      //      std::cout << "---- pred pic is " << refSampSide << std::endl;
      const PelUnitBuf predictionUnitBuf = encpic.get(picId);

      //      uint picId = refSampSide;
      //      std::cout << "---- pred pic is " << picId << std::endl;

      PelBuf roiComp_predictionFrameBuff =
          VTM_Utility::getAreaBuf(predictionUnitBuf, compArea);

      auto &si_TUs = si.TUs;
      unsigned int n_tus = si_TUs.size();

      if (chType == CHANNEL_TYPE_CHROMA)
        availableDirModes[0] =
            si.intraDir[COMPONENT_Y]; // set the direct mode for current SI
                                      // chroma channel

      intraPred.initIntraPatternChType(reconstructionUnitBuffer, refSampSide,
                                       compArea, bitDepths[chType], false);

      entropy = std::numeric_limits<double>::max();

      // SEARCH BEST DIRMODE FOR INTRA PRED
      for (const UInt &dirMode : availableDirModes) // access by const reference
      {

        intraPred.predIntraAng(roiComp_predictionFrameBuff, dirMode);
        for (UInt j = 0; j < n_tus; j++) {
          TransformUnit &curr_tu_si = si_TUs[j];

          transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
                                   predictionUnitBuf, curr_tu_si.block(compID),
                                   bitDepths[chType], maxLog2TrDynamicRange);

          quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
                      curr_tu_si.getTransformCoeffs(compID), uiAbsSum, cQP,
                      SliceType::I_SLICE, bitDepths[chType],
                      maxLog2TrDynamicRange);
          //                    quant.dequant(curr_tu_si.getInverseQuantizedTransformCoeffs(compID),
          //                    curr_tu_si.getQuantizedTransformCoeffs(compID),
          //                    cQP, bitDepths[chType], maxLog2TrDynamicRange);

          //                    cout << "icu " << i << ", comID " << compID <<
          //                    ", refSampl " << refSamplToStr(refSampSide) <<
          //                    ", [x, y, w, h] = [ "<<
          //                    curr_tu_si.block(compID).x << ", " <<
          //                    curr_tu_si.block(compID).y << ", " <<
          //                    curr_tu_si.block(compID).width << ", " <<
          //                    curr_tu_si.block(compID).height << "]" << ",
          //                    dirMode= " << dirMode << endl;
        }
        currEntropy =
            static_cast<double>(si.estimateSIRequiredBitsToCodeIntraMode(
                compID, dirMode, refSampSide));
        residual.clear();

        for (UInt b = 0; b < nBands; b++) {
          icu_enc.getTransformSequences(
              compID, refSampSide, b, tuWidth_component, tuHeight_component,
              transformSequence_sourceX, transformSequence_sourceY);

          //                    if(i==0 && dirMode==67)currEntropy
          //                    {
          //                        cout << "transformSequence_sourceY[j]= ";
          //                        for(unsigned int j=0; j <
          //                        transformSequence_sourceY.size(); j++)
          //                            cout << transformSequence_sourceY[j] <<
          //                            ", ";
          //                        cout << endl;
          //                    }

          //                    if(i==0 && dirMode==67)
          //                    {
          //                        cout << "transformSequence_sourceX[j]= ";
          //                        for(unsigned int j=0; j <
          //                        transformSequence_sourceX.size(); j++)
          //                            cout << transformSequence_sourceX[j] <<
          //                            ", ";
          //                        cout << endl;
          //                    }

          CHECK(transformSequence_sourceX.size() !=
                    transformSequence_sourceY.size(),
                "transform sequence vector size do not match");
          for (unsigned int j = 0; j < transformSequence_sourceX.size(); j++) {
            residual.push_back(transformSequence_sourceX[j] -
                               transformSequence_sourceY[j]);
          }

          currEntropy += VTM_Utility::
              computeTheoreticalRateWithEntropyForCompressPrecomputeFrequencyTable2ByteValuesAdaptiveRangeAbsoluteValues(
                  residual, encodeFrequencyTableInBitStream,
                  maxLog2TrDynamicRange + 1);
        }

        // IF CURRENT DIRMODE IS BETTER
        if (currEntropy < entropy) {
          entropy = currEntropy;
          si.intraDir[compID] = dirMode;
        }
      }
      //            if(i < 3)
      //            {
      //                cout << "icu " << i << ", comID " << compID << ",
      //                refSampl " << refSamplToStr(refSampSide) << ", best
      //                intra mode " << UInt(si.intraDir[compID]) << ", entropy
      //                " << entropy << endl;
      //            }

      //            if(i==0)
      //                cout << "entropy= " << entropy << endl;
      intraPred.predIntraAng(roiComp_predictionFrameBuff, si.intraDir[compID]);
      for (UInt j = 0; j < n_tus; j++) {
        TransformUnit &curr_tu_si = si_TUs[j];
        transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
                                 predictionUnitBuf, curr_tu_si.block(compID),
                                 bitDepths[chType], maxLog2TrDynamicRange);
        quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
                    curr_tu_si.getTransformCoeffs(compID), uiAbsSum, cQP,
                    SliceType::I_SLICE, bitDepths[chType],
                    maxLog2TrDynamicRange);
      }

      //      if (oSiResult != nullptr) {
      //        cv::Mat predMat;
      //        VTM_OPENCV_Utility::toBGRMat(predictionUnitBuf, predMat);
      //        oSiResult->insert_or_assign(refSampSide, predMat);
      //      }
    }
  }
}

void icov::video::EncodingStructure::get_decodedICUsInRequestedICUs(
    const cv::Mat &maskRequestedPixels,
    std::vector<std::vector<Position>> &requestedICUs_closeToDecodedICUs,
    std::vector<std::vector<Position>> &decodedICUs_inRequestedICUs) {
  _requestedICUs.fill(false);

  _maskThresholded.create(maskRequestedPixels.size(), CV_8UC1);
  if (_maskThresholded.isContinuous())
    memset(_maskThresholded.ptr(), 0,
           _maskThresholded.step[0] *
               _maskThresholded
                   .rows); // _maskThresholded.step[0] * _maskThresholded.rows
                           // represents cv::Mat size in byte. It is almost 2
                           // times faster than setTo(0)
  else
    _maskThresholded.setTo(0);

  _maskThresholded.setTo(
      1,
      maskRequestedPixels); // This is done because in maskRequestedPixels
  // nonzero pixels have value 255, so in the next
  // cv::integral function overflow can happen for
  // very large images. That's why I changed nonzero
  // pixels to 1

  cv::integral(_maskThresholded, _integralImageMask, CV_32SC1);

  int x1 = 0, y1 = 0, x2, y2;
  int n_nonzero;
  int n_requestedICUs = 0;
  for (UInt y = 0; y < n_icusPerCol; y++) {
    bool *requested_rowPtr = &_requestedICUs.at(0, y);
    //        bool* previous_rowPtr = &_decodedICUs.at(0,y);
    for (UInt x = 0; x < n_icusPerRow; x++) {
      int i = y * n_icusPerRow + x;

      const InteractiveCodingUnit &icu = vec_icus[i].get_sourceX();
      // the reason that we can not use the x,y and of icu is because for
      // example for 360 images, first icu position starts from point
      // (margin_x,y), but maskRequestioPixels contains the main frame area with
      // no margins
      x2 = x1 + int(icu.Y().width);
      y2 = y1 + int(icu.Y().height);
      // to compute all values in range (x1 <= x < x2 && y1 <= y < y2), i.e. x2,
      // y2 not included
      //

      n_nonzero = _integralImageMask.at<int>(y2, x2) -
                  _integralImageMask.at<int>(y2, x1) -
                  _integralImageMask.at<int>(y1, x2) +
                  _integralImageMask.at<int>(y1, x1);

      if (n_nonzero /*&& (!previous_rowPtr[x])*/) // if it is requested but it
      // is not already decoded
      {
        requested_rowPtr[x] = true;
        n_requestedICUs++;
      }

      x1 = x2;
    }
    x1 = 0;
    y1 = y2;
  }

  requestedICUs_closeToDecodedICUs.clear();
  decodedICUs_inRequestedICUs.clear();

  for (UInt y = 0; y < n_icusPerCol;
       y++) // check for requested ICUs where there is at least an already
  // decoded ICU there
  {
    bool *requested_rowPtr = &_requestedICUs.at(0, y);
    for (UInt x = 0; x < n_icusPerRow; x++) {
      if (requested_rowPtr[x]) // checking toghether, we are sure that the
      // first starting point of
      // requestedICUs_connectedComponents is close an
      // already decoded ICU
      {
        requestedICUs_closeToDecodedICUs.push_back(std::vector<Position>());
        decodedICUs_inRequestedICUs.push_back(std::vector<Position>());
        dfs_360(_requestedICUs, _decodedICUs,
                requestedICUs_closeToDecodedICUs.back(),
                decodedICUs_inRequestedICUs.back(), x, y);
      }
    }
  }
}

void icov::video::EncodingStructure::updateDecodedICU(
    const Position &ICU_position) {
  CHECK(ICU_position.x < 0 || ICU_position.x >= Int(n_icusPerRow) ||
            ICU_position.y < 0 || ICU_position.y >= Int(n_icusPerCol),
        "out of range");
  bool &val = _decodedICUs.at(ICU_position.x, ICU_position.y);
  if (!val) {
    val = true;
    _vec_decodedICUs.push_back(ICU_position);
  }
}

void icov::video::EncodingStructure::updateDecodedICUs(
    const std::vector<std::vector<Position>> &connectedComponents) {
  for (UInt i = 0; i < connectedComponents.size(); i++) {
    for (UInt j = 0; j < connectedComponents[i].size(); j++) {
      updateDecodedICU(connectedComponents[i][j]);
    }
  }
}

int icov::video::EncodingStructure::getAvailableDecodedSides_regularImage(
    const Position &icuPos) const {
  int availableSides = 0;

  int horizontal_y;
  int iWidth = _decodedICUs.width;
  int iHeight = _decodedICUs.height;

  const bool *rowPtr = nullptr;
  // check top side
  horizontal_y = icuPos.y - 1;
  if ((horizontal_y < iHeight) && (horizontal_y >= 0)) {
    rowPtr = &_decodedICUs.at(0, horizontal_y);
    if ((icuPos.x - 1 >= 0) &&
        (rowPtr[icuPos.x - 1] == true)) // check top-left corner
      availableSides |= 0b000100000;
    if ((icuPos.x + 1 < iWidth) &&
        (rowPtr[icuPos.x + 1] == true)) // check top-right corner
      availableSides |= 0b001000000;

    if (rowPtr[icuPos.x] == true)
      availableSides |=
          icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::TOP;
  }

  // check bottom side
  horizontal_y = icuPos.y + 1;
  if ((horizontal_y < iHeight) && (horizontal_y >= 0)) {
    rowPtr = &_decodedICUs.at(0, horizontal_y);
    if ((icuPos.x - 1 >= 0) &&
        (rowPtr[icuPos.x - 1] == true)) // check bottom-left corner
      availableSides |= 0b010000000;
    if ((icuPos.x + 1 < iWidth) &&
        (rowPtr[icuPos.x + 1] == true)) // check bottom-right corner
      availableSides |= 0b100000000;

    if (rowPtr[icuPos.x] == true)
      availableSides |=
          icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::BOTTOM;
  }

  rowPtr = &_decodedICUs.at(0, icuPos.y);

  if ((icuPos.x - 1 >= 0) && (rowPtr[icuPos.x - 1] == true)) // check left side
    availableSides |= icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::LEFT;

  if ((icuPos.x + 1 < int(n_icusPerRow)) &&
      (rowPtr[icuPos.x + 1] == true)) // check right side
    availableSides |=
        icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::RIGHT;

  return availableSides;
}

int icov::video::EncodingStructure::getAvailableDecodedSides_360Image(
    const Position &icuPos) const {
  int availableSides = 0;
  //        neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth;  // to
  //        generate circular traversing for 360 video in x direction

  int horizontal_y;
  int iWidth = _decodedICUs.width;
  int iHeight = _decodedICUs.height;
  int x;

  const bool *rowPtr = nullptr;
  // check top side
  horizontal_y = icuPos.y - 1;
  if ((horizontal_y < iHeight) && (horizontal_y >= 0)) {
    rowPtr = &_decodedICUs.at(0, horizontal_y);

    x = (iWidth + ((icuPos.x - 1) % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    if (rowPtr[x] == true) // check top-left corner
      availableSides |= 0b000100000;

    x = (iWidth + ((icuPos.x + 1) % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    if (rowPtr[x] == true) // check top-right corner
      availableSides |= 0b001000000;

    if (rowPtr[icuPos.x] == true)
      availableSides |=
          icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::TOP;
  }

  // check bottom side
  horizontal_y = icuPos.y + 1;
  if ((horizontal_y < iHeight) && (horizontal_y >= 0)) {
    rowPtr = &_decodedICUs.at(0, horizontal_y);

    x = (iWidth + ((icuPos.x - 1) % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    if (rowPtr[x] == true) // check bottom-left corner
      availableSides |= 0b010000000;

    x = (iWidth + ((icuPos.x + 1) % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    if (rowPtr[x] == true) // check bottom-right corner
      availableSides |= 0b100000000;

    if (rowPtr[icuPos.x] == true)
      availableSides |=
          icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::BOTTOM;
  }

  rowPtr = &_decodedICUs.at(0, icuPos.y);

  x = (iWidth + ((icuPos.x - 1) % iWidth)) %
      iWidth; // to generate circular traversing for 360 video in x direction
  if (rowPtr[x] == true) // check left side
    availableSides |= icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::LEFT;

  x = (iWidth + ((icuPos.x + 1) % iWidth)) %
      iWidth; // to generate circular traversing for 360 video in x direction
  if (rowPtr[x] == true) // check right side
    availableSides |=
        icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE::RIGHT;

  return availableSides;
}

void icov::video::EncodingStructure::updateAlreadyProccessedICU(
    const Position &ICU_position) {
  CHECK(ICU_position.x < 0 || ICU_position.x >= Int(n_icusPerRow) ||
            ICU_position.y < 0 || ICU_position.y >= Int(n_icusPerCol),
        "out of range");
  bool &val = _tmp_alreadyProcessedICUs.at(ICU_position.x, ICU_position.y);
  if (!val) {
    val = true;
    _vec_tmp_alreadyProcessedICUs.push_back(ICU_position);
  }
}

void icov::video::EncodingStructure::deallocate() {
  if (_requestedICUs.buf) {
    std::cout << "deallocating encoding structure" << std::endl;
    xFree(_requestedICUs.buf);
    _requestedICUs = AreaBuf<bool>();
    //        requestedBlocks.buf = nullptr;
    //        requestedBlocks.stride = 0;
  }

  if (_decodedICUs.buf) {
    xFree(_decodedICUs.buf);
    _decodedICUs = AreaBuf<bool>();
    //        previousDecodedBlocks.buf = nullptr;
    //        previousDecodedBlocks.stride = 0;
  }

  //    _tmp_alreadyProcessedICUs
  if (_tmp_alreadyProcessedICUs.buf) {
    xFree(_tmp_alreadyProcessedICUs.buf);
    _tmp_alreadyProcessedICUs = AreaBuf<bool>();
  }

  vec_icus.clear();
  vec_checkPoints.clear();
  _vec_decodedICUs.clear();
  _vec_pixelsRequestedPerICU.clear();
  _vec_tmp_alreadyProcessedICUs.clear();

  n_icusPerRow = 0;
  n_icusPerCol = 0;
}

icov::video::IcuEncoder &
icov::video::EncodingStructure::getICU(const UInt icu_index) {
  ICOV_ASSERT_MSG(icu_index >= 0 && icu_index < vec_icus.size(),
                  "Cannot retrieve ICU " + std::to_string(icu_index) +
                      " from vector of size " +
                      std::to_string(vec_icus.size()));
  return vec_icus[icu_index];
}

icov::video::IcuEncoder &
icov::video::EncodingStructure::getICU(const UInt x_index, const UInt y_index) {
  return getICU(y_index * n_icusPerRow + x_index);
}

const icov::video::IcuEncoder &
icov::video::EncodingStructure::getICU(const UInt icu_index) const {
  ICOV_ASSERT_MSG(icu_index >= 0 && icu_index < vec_icus.size(),
                  "Cannot retrieve ICU " + std::to_string(icu_index) +
                      " from vector of size " +
                      std::to_string(vec_icus.size()));
  return vec_icus[icu_index];
}

bool icov::video::EncodingStructure::isLeftVerticalBorder(
    const UInt icu_index) const {
  return (icu_index < vec_icus.size()) && (icu_index % n_icusPerRow == 0);
}

bool icov::video::EncodingStructure::isRightVerticalBorder(
    const UInt icu_index) const {
  return (icu_index < vec_icus.size()) &&
         (icu_index % n_icusPerRow == n_icusPerRow - 1);
}

UInt icov::video::EncodingStructure::get_icuIndex(const Position &pos) const {
  return UInt(pos.y) * n_icusPerRow + UInt(pos.x);
}

UInt icov::video::EncodingStructure::get_icuIndex(const Block &b) const {}

void icov::video::EncodingStructure::resetDecodedICUs() {
  _decodedICUs.fill(false);
  _vec_decodedICUs.clear();
}

bool icov::video::EncodingStructure::isICUAlreadyDecoded(
    const Position &ICU_position) const {
  return _decodedICUs.at(ICU_position.x, ICU_position.y);
}

const std::vector<Position> &
icov::video::EncodingStructure::get_vecDecodedICUs() const {
  return _vec_decodedICUs;
}

const std::vector<Position> &
icov::video::EncodingStructure::get_vecCheckpoints() const {
  return vec_checkPoints;
}

void icov::video::EncodingStructure::resetAlreadyProcessedICUs() {
  _tmp_alreadyProcessedICUs.fill(false);
  _vec_tmp_alreadyProcessedICUs.clear();
}

void icov::video::EncodingStructure::copyProccessedICUsFromDecodedICUs() {
  for (unsigned int block_id = 0; block_id < _vec_decodedICUs.size();
       block_id++)
    updateAlreadyProccessedICU(_vec_decodedICUs[block_id]);
}

bool icov::video::EncodingStructure::isICUAlreadyProcessed(
    const Position &ICU_position) {
  return _tmp_alreadyProcessedICUs.at(ICU_position.x, ICU_position.y);
}
