/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef ICU_ENCODER_H
#define ICU_ENCODER_H
#include "myunits.h"
#include "video_common.h"

class ICU_Encoder
{
protected:
    InteractiveCodingUnit _sourceX;
    std::map<MyIntraPrediction::REFERENCE_SAMPLE_SIDE, SideInformationUnit> _mapIntraSIs;
public:
    ICU_Encoder(const UnitArea& unit, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=true, bool createInverseQuantizedTransformBuffer= true) : _sourceX(unit, createTransformBuffer, createQuantizedTransformBuffer, createInverseQuantizedTransformBuffer) {}
    ICU_Encoder(const ChromaFormat chromaFormat, const Area &lumaArea, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=true, bool createInverseQuantizedTransformBuffer= true) : _sourceX(chromaFormat, lumaArea, createTransformBuffer, createQuantizedTransformBuffer, createInverseQuantizedTransformBuffer) {}

    bool addIntraSI(MyIntraPrediction::REFERENCE_SAMPLE_SIDE refSampSide, bool createTransformBuffer = true, bool createQuantizedTransformBuffer=false, bool createInverseQuantizedTransformBuffer= false);

    inline InteractiveCodingUnit& get_sourceX() {return _sourceX;}
    inline const InteractiveCodingUnit& get_sourceX() const {return _sourceX;}
    inline std::vector<MyTransformUnit>& sourceX_TUs() {return _sourceX.TUs;}
    inline const std::vector<MyTransformUnit>& sourceX_TUs() const {return _sourceX.TUs;}

    void creatTUs(const UInt tuWidth_luma, const UInt tuHeight_luma);

    SideInformationUnit* get_sideInformation(const MyIntraPrediction::REFERENCE_SAMPLE_SIDE& rfSamp);
    const SideInformationUnit* get_sideInformation (const MyIntraPrediction::REFERENCE_SAMPLE_SIDE& rfSamp) const;

//    void calcConditionalEntropy(const ComponentID compId, const REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
//                               std::vector<Position>& sequenceTransfromValues, std::vector<Position>& uniqueTransformValues, std::vector<double>& probs_XY);


    UInt calcDistributions(const ComponentID compId, const MyIntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
                                   std::vector<TCoeff>& transformSequence_sourceX, std::vector<TCoeff>& transformSequence_sourceY,
                                   std::map<TCoeff, std::map<TCoeff, double> >& probs_XY, std::map<TCoeff, std::map<TCoeff, double> >& probs_YX,    // probs_YX is same is probs_XY but not the first element is y
                                   std::map<TCoeff, double> & probs_X, std::map<TCoeff, double> & probs_Y);   // function returns conditional entropy of XgivenY times length of the sequence: sequenceLength * H(X|Y)

    void getTransformSequences(const ComponentID compId, const MyIntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
                                       std::vector<TCoeff>& transformSequence_sourceX, std::vector<TCoeff>& transformSequence_sourceY) const;

    UInt calcDistributionsBasedOnResidual(const ComponentID compId, const MyIntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
                                          std::vector<TCoeff>& transformSequence_sourceX, std::vector<TCoeff>& transformSequence_sourceY, std::vector<int>& residual,
                                          std::map<int, double>& probs_residualXminusY, bool compressBasedOnAbsValues, int maxLog2DynamicRange);


    void calcDistributions(const ComponentID compId, std::vector<UInt>& numberOfDecodingBits, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
                                   std::vector<std::vector<TCoeff> >& transformSequence_sourceX, std::vector<std::vector<TCoeff> >& allSIs_transformSequence_sourceY,
                                   std::vector<std::map<TCoeff, std::map<TCoeff, double> > >& allSIs_probs_XY, std::vector<std::map<TCoeff, std::map<TCoeff, double> > >& allSIs_probs_YX,    // probs_YX is same is probs_XY but not the first element is y
                                   std::vector<std::map<TCoeff, double> >& allSIs_probs_X, std::vector<std::map<TCoeff, double> > & allSIs_probs_Y);   // function returns conditional entropy of XgivenY times sequence length

    void calcDistributionsBasedOnResidual(const ComponentID compId, std::vector<UInt>& numberOfDecodingBits, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
                                          std::vector<std::vector<TCoeff> >& transformSequence_sourceX, std::vector<std::vector<TCoeff> >& allSIs_transformSequence_sourceY,
                                          std::vector<std::vector<int> >& vec_residual, std::vector<std::map<int, double> >& vec_probs_residualXminusY, bool compressBasedOnAbsValues, int maxLog2DynamicRange);

    std::map<MyIntraPrediction::REFERENCE_SAMPLE_SIDE, SideInformationUnit>& get_intraSIsMap() {return _mapIntraSIs;}
    const std::map<MyIntraPrediction::REFERENCE_SAMPLE_SIDE, SideInformationUnit>& get_intraSIsMap() const {return _mapIntraSIs;}
    inline UInt numberOfSIs() const {return _mapIntraSIs.size();}   // TODO: don't forget to change it when interprediction is also added

protected:
    bool addTU(const UnitArea& unit);

};

#endif // ICU_ENCODER_H
