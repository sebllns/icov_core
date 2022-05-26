/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef ENCODINGSTRUCTURE_H
#define ENCODINGSTRUCTURE_H
#include "icu_encoder.h"
#include "video_common.h"
//#include <opencv2/core.hpp>
struct EncodingStructure
{

    EncodingStructure();
    EncodingStructure(const std::vector<ICU_Encoder>& icus, const UInt icusPerRow, const UInt icusPerColumn);
    ~EncodingStructure();
    void set(const std::vector<ICU_Encoder>& icus, const UInt icusPerRow, const UInt icusPerColumn);
    void setCheckPoints(const std::vector<Position>& checkPoints);
    void setCheckPoint(const Position& checkPoint);
    bool isAccessBlock(const Position& accessBlockPoint);

    std::vector<ICU_Encoder> vec_icus;
    std::vector<Position> vec_checkPoints;
    UInt n_icusPerRow, n_icusPerCol;


    inline ICU_Encoder& getICU(const UInt icu_index) {return vec_icus[icu_index];}
    inline const ICU_Encoder& getICU(const UInt icu_index) const {return vec_icus[icu_index];}
    inline ICU_Encoder& getICU(const UInt x_index, const UInt y_index) {return vec_icus[y_index * n_icusPerRow + x_index];}

    inline bool isLeftVerticalBorder(const UInt icu_index) const {return (icu_index < vec_icus.size()) && (icu_index % n_icusPerRow==0);}
    inline bool isRightVerticalBorder(const UInt icu_index) const {return (icu_index < vec_icus.size()) && (icu_index % n_icusPerRow==n_icusPerRow-1);}

    inline UInt get_icuIndex(const Position& pos) const {return UInt(pos.y) * n_icusPerRow + UInt(pos.x);}
    Position get_icuPosition(const UInt icu_index) const;
    /*
     *
     */

    inline void resetDecodedICUs() {_decodedICUs.fill(false); _vec_decodedICUs.clear();}


    void reorderBlockDecodingBasedOnStartingAccessBlock(std::vector<std::vector<Position> >& in_out_requested_icus, const std::vector<Position>& accessBlockInsideAsStartingPosition);

    inline bool isICUAlreadyDecoded(const Position& ICU_position) const {return _decodedICUs.at(ICU_position.x, ICU_position.y);}
    inline const std::vector<Position>& get_vecDecodedICUs() const {return _vec_decodedICUs;}

    inline const std::vector<Position>& get_vecCheckpoints() const {return vec_checkPoints;}
    /** returns an integer which its last 4 least significant bits show which sides are available. For more information refer to enum REFERENCE_SAMPLE_SIDE
     * Also the bit levels [5-8] (zero-based index) is 1 if the corner point is also available:
     * bit level 4 = (not used) to avoid conflict of corner usage in REFERENCE_SAMPLE_SIDE.
     * bit level 5 = top-left corner
     * bit level 6 = top-right corner
     * bit level 7 = bottom-left corner
     * bit level 8 = bottom-right corner
     */
    int getAvailableDecodedSides_regularImage(const Position& icuPos) const;
    int getAvailableDecodedSides_360Image(const Position& icuPos) const;


protected:
    inline void resetAlreadyProcessedICUs() {_tmp_alreadyProcessedICUs.fill(false); _vec_tmp_alreadyProcessedICUs.clear();}
    void updateAlreadyProccessedICU(const Position& ICU_position);
    inline void copyProccessedICUsFromDecodedICUs() {for (unsigned int block_id=0; block_id < _vec_decodedICUs.size(); block_id++) updateAlreadyProccessedICU(_vec_decodedICUs[block_id]);}
    inline bool isICUAlreadyProcessed(const Position& ICU_position) {return _tmp_alreadyProcessedICUs.at(ICU_position.x, ICU_position.y);}
protected:
    std::vector<int> _vec_pixelsRequestedPerICU;
    std::vector<Position> _vec_decodedICUs;
    AreaBuf<bool> _requestedICUs, _decodedICUs;
    std::vector<Position> _vec_tmp_alreadyProcessedICUs;
    AreaBuf<bool> _tmp_alreadyProcessedICUs;
    void deallocate();
};

#endif // ENCODINGSTRUCTURE_H
