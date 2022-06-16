/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "icu_encoder.h"
#include <math.h>



bool ICU_Encoder::addIntraSI(MyIntraPrediction::REFERENCE_SAMPLE_SIDE refSampSide, bool createTransformBuffer, bool createQuantizedTransformBuffer, bool createInverseQuantizedTransformBuffer)
{
    std::pair<std::map<MyIntraPrediction::REFERENCE_SAMPLE_SIDE, SideInformationUnit >::iterator, bool> ret;
    ret = _mapIntraSIs.insert(std::make_pair(refSampSide, SideInformationUnit(_sourceX, createTransformBuffer, createQuantizedTransformBuffer, createInverseQuantizedTransformBuffer) ));
    if(ret.second == false)
    {
        std::cerr << "ICU_Encoder::addIntraSI Error: reference sample refSampSide already existed" << std::endl;
        return false;
    }

    for(unsigned int i=0; i < _sourceX.TUs.size(); i++)
    {
        SideInformationUnit& si = ret.first->second;
        si.addTU(_sourceX.TUs[i]);
    }

    return true;
}

void ICU_Encoder::creatTUs(const UInt tuWidth_luma, const UInt tuHeight_luma)
{
    // clear previous TUs
    _sourceX.TUs.clear();
    for (auto it=_mapIntraSIs.begin(); it!=_mapIntraSIs.end(); ++it)   // loop over all intra side information
    {
        SideInformationUnit& si = it->second;
        si.TUs.clear();
    }

    div_t divresult;
    UInt nx_tus =  (_sourceX.Y().width  + tuWidth_luma  - 1) / tuWidth_luma;
    UInt ny_tus =  (_sourceX.Y().height + tuHeight_luma - 1) / tuHeight_luma;
    UInt n_tus = nx_tus * ny_tus;
    for(UInt tuIndex = 0; tuIndex < n_tus; tuIndex++)
    {
        divresult = div(Int(tuIndex), Int(nx_tus));
        UInt tuXPosTopLeft = divresult.rem;
        UInt tuYPosTopLeft = divresult.quot;
        Area tuLumaArea = Area(tuXPosTopLeft*tuWidth_luma + _sourceX.Y().x, tuYPosTopLeft*tuHeight_luma + _sourceX.Y().y, tuWidth_luma, tuHeight_luma);// & (_sourceX.Y());
        addTU(UnitArea(_sourceX.chromaFormat, tuLumaArea ));
    }
}

SideInformationUnit *ICU_Encoder::get_sideInformation(const MyIntraPrediction::REFERENCE_SAMPLE_SIDE &rfSamp)
{
    auto it = _mapIntraSIs.find(rfSamp);
    if (it != _mapIntraSIs.end())
        return &(it->second);

    return nullptr;
}

const SideInformationUnit *ICU_Encoder::get_sideInformation(const MyIntraPrediction::REFERENCE_SAMPLE_SIDE &rfSamp) const
{
    auto it = _mapIntraSIs.find(rfSamp);
    if (it != _mapIntraSIs.end())
        return &(it->second);

    return nullptr;
}


//#include "vtm_opencv_utility.h"

UInt ICU_Encoder::calcDistributions(const ComponentID compId, const MyIntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight,
                                        std::vector<TCoeff> &transformSequence_sourceX, std::vector<TCoeff> &transformSequence_sourceY,
                                        std::map<TCoeff, std::map<TCoeff, double> > &probs_XY, std::map<TCoeff, std::map<TCoeff, double> > &probs_YX,
                                        std::map<TCoeff, double> &probs_X, std::map<TCoeff, double> &probs_Y)
{

    SideInformationUnit *SI_unit = get_sideInformation(rfSamp);
    if(SI_unit == nullptr)
        return -1.;
    probs_XY.clear();
    probs_YX.clear();
    transformSequence_sourceX.clear();
    transformSequence_sourceY.clear();
    transformSequence_sourceX.reserve( (_sourceX.block(compId).width/tuWidth)* (_sourceX.block(compId).height/tuHeight) );
    transformSequence_sourceY.reserve(transformSequence_sourceX.capacity());
    div_t divresult;

    CHECK(_sourceX.TUs.size() != SI_unit->TUs.size(), "size of transform units do not match");

    std::map<TCoeff, std::map<TCoeff, double> >::iterator it_firstElement;
    std::map<TCoeff, double>::iterator it_secondElement;
//    std::pair<std::map<TCoeff, std::map<TCoeff, double> >::iterator, bool> ret;
    for(unsigned int k=0; k < _sourceX.TUs.size(); k++)
    {
        const CCoeffBuf X = _sourceX.TUs[k].getQuantizedTransformCoeffs(compId);    // quantized Transform coefficients for source x
//        const CCoeffBuf X = _sourceX.TUs[k].getInverseQuantizedTransformCoeffs(compId);    // quantized Transform coefficients for source x
//        std::cout << "X= " << VTM_OPENCV_Utility::toMat<TCoeff>(X) << std::endl;

//        const CCoeffBuf Y = SI_unit->TUs[k].getTransformCoeffs(compId); //Transform coefficients for Side information
//        const CCoeffBuf Y = SI_unit->TUs[k].getInverseQuantizedTransformCoeffs(compId); //Transform coefficients for Side information
        const CCoeffBuf Y = SI_unit->TUs[k].getQuantizedTransformCoeffs(compId); //Transform coefficients for Side information
//        std::cout << "Y= " << VTM_OPENCV_Utility::toMat<TCoeff>(Y) << std::endl;

        UInt widthInGroups  = (X.width + tuWidth -1) / tuWidth;
        UInt heightInGroups = (X.height+ tuHeight -1) / tuHeight;

        divresult = div(Int(bandIndex), Int(tuWidth));

        for(UInt j= 0; j < heightInGroups; j++) // regrouping bands if necessary
        {
            UInt y = divresult.quot * heightInGroups + j;
            UInt offset_X = y * X.stride;
            UInt offset_Y = y * Y.stride;
            for(UInt i= 0; i < widthInGroups; i++)
            {
                UInt x = divresult.rem * widthInGroups + i;
                const TCoeff& transformValue_x = X.buf[offset_X + x];
                const TCoeff& transformValue_y = Y.buf[offset_Y + x];

                transformSequence_sourceX.push_back(transformValue_x);
                transformSequence_sourceY.push_back(transformValue_y);

                it_firstElement = probs_XY.find(transformValue_x);
                if ( it_firstElement != probs_XY.end() )
                {
                    it_secondElement = it_firstElement->second.find(transformValue_y);
                    if(it_secondElement != it_firstElement->second.end())
                        it_secondElement->second += 1.;
                    else
                        probs_XY[transformValue_x][transformValue_y] = 1.;

                }
                else
                    probs_XY[transformValue_x][transformValue_y] = 1.;

//                probs_XY[transformValue_x][transformValue_y]++;


                it_firstElement = probs_YX.find(transformValue_y);
                if ( it_firstElement != probs_YX.end() )
                {
                    it_secondElement = it_firstElement->second.find(transformValue_x);
                    if(it_secondElement != it_firstElement->second.end())
                        it_secondElement->second += 1.;
                    else
                        probs_YX[transformValue_y][transformValue_x] = 1.;

                }
                else
                    probs_YX[transformValue_y][transformValue_x] = 1.;
//                probs_YX[transformValue_y][transformValue_x]++;

            }
        }
    }

//    std::cout << "x=[ " << transformSequence_sourceX[0];
//    for(unsigned int i=1; i < transformSequence_sourceX.size(); i++)
//        std::cout << ", " << transformSequence_sourceX[i];
//    std::cout << "]';" << std::endl;

//    std::cout << "y=[ " << transformSequence_sourceY[0];
//    for(unsigned int i=1; i < transformSequence_sourceY.size(); i++)
//        std::cout << ", " << transformSequence_sourceY[i];
//    std::cout << "]';" << std::endl;

//    std::cout << "(x,y) =[ (" << transformSequence_sourceX[0] << "," << transformSequence_sourceY[0] << ")";
//    for(unsigned int i=1; i < transformSequence_sourceX.size(); i++)
//        std::cout << ", (" << transformSequence_sourceX[i] << "," << transformSequence_sourceY[i] << ")";
//    std::cout << "];" << std::endl;

    double marginalProb = 0.;
    UInt sequenceLength = transformSequence_sourceX.size();
    double normalization = 1./((double)sequenceLength);


    // normalize P_YX and compute P_Y
    for(it_firstElement=probs_YX.begin(); it_firstElement!=probs_YX.end(); ++it_firstElement)
    {
        marginalProb = 0.;
        const TCoeff& yVal = it_firstElement->first;
        for(it_secondElement=it_firstElement->second.begin(); it_secondElement!=it_firstElement->second.end(); ++it_secondElement)
        {
//            const TCoeff& xVal = it_secondElement->first;
            double& pVal = it_secondElement->second;

            pVal *= normalization;
            marginalProb += pVal;
        }
        probs_Y[yVal] = marginalProb;
    }


    double conditionalEntropy_XgivenY = 0.;
    double f;
    for(it_firstElement=probs_XY.begin(); it_firstElement!=probs_XY.end(); ++it_firstElement)
    {
        marginalProb = 0.;
        const TCoeff& xVal = it_firstElement->first;
        for(it_secondElement=it_firstElement->second.begin(); it_secondElement!=it_firstElement->second.end(); ++it_secondElement)
        {
            const TCoeff& yVal = it_secondElement->first;
            double& pVal = it_secondElement->second;

            pVal *= normalization;
            marginalProb += pVal;

            f = pVal * std::log2(probs_Y[yVal]/pVal);
            if(std::isinf(f) && (fabs(pVal) < 1e-13) ) // to solve the problem of "epsilon * log2(number/epsilon)"
                f = 0.;
            if(f != f) // if it is nan make it zero
                f=0.;
            conditionalEntropy_XgivenY += f;
        }
        probs_X[xVal] = marginalProb;
    }

//    if(conditionalEntropy_XgivenY > 0.)
//        std::cout << "conditionalEntropy_XgivenY= " << conditionalEntropy_XgivenY << std::endl;
    UInt n_bits = UInt(std::ceil(conditionalEntropy_XgivenY * (double)sequenceLength));
    SI_unit->bitsToDecode[compId][bandIndex] = n_bits;
    return n_bits;

}

void ICU_Encoder::getTransformSequences(const ComponentID compId, const MyIntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight, std::vector<TCoeff> &transformSequence_sourceX, std::vector<TCoeff> &transformSequence_sourceY) const
{
    const SideInformationUnit *SI_unit = get_sideInformation(rfSamp);
    transformSequence_sourceX.clear();
    transformSequence_sourceY.clear();

    if(SI_unit == nullptr)
        return;

    transformSequence_sourceX.reserve( (_sourceX.block(compId).width/tuWidth)* (_sourceX.block(compId).height/tuHeight) );
    transformSequence_sourceY.reserve(transformSequence_sourceX.capacity());
    div_t divresult;

    CHECK(_sourceX.TUs.size() != SI_unit->TUs.size(), "size of transform units do not match");

    for(unsigned int k=0; k < _sourceX.TUs.size(); k++)
    {
        const CCoeffBuf X = _sourceX.TUs[k].getQuantizedTransformCoeffs(compId);    // quantized Transform coefficients for source x
//        const CCoeffBuf X = _sourceX.TUs[k].getInverseQuantizedTransformCoeffs(compId);    // quantized Transform coefficients for source x
//        std::cout << "X= " << VTM_OPENCV_Utility::toMat<TCoeff>(X) << std::endl;

//        const CCoeffBuf Y = SI_unit->TUs[k].getTransformCoeffs(compId); //Transform coefficients for Side information
//        const CCoeffBuf Y = SI_unit->TUs[k].getInverseQuantizedTransformCoeffs(compId); //Transform coefficients for Side information
        const CCoeffBuf Y = SI_unit->TUs[k].getQuantizedTransformCoeffs(compId); //Transform coefficients for Side information
//        std::cout << "Y= " << VTM_OPENCV_Utility::toMat<TCoeff>(Y) << std::endl;

        UInt widthInGroups  = (X.width + tuWidth -1) / tuWidth;
        UInt heightInGroups = (X.height+ tuHeight -1) / tuHeight;

        divresult = div(Int(bandIndex), Int(tuWidth));

        for(UInt j= 0; j < heightInGroups; j++) // regrouping bands if necessary
        {
            UInt y = divresult.quot * heightInGroups + j;
            UInt offset_X = y * X.stride;
            UInt offset_Y = y * Y.stride;
            for(UInt i= 0; i < widthInGroups; i++)
            {
                UInt x = divresult.rem * widthInGroups + i;
                const TCoeff& transformValue_x = X.buf[offset_X + x];
                const TCoeff& transformValue_y = Y.buf[offset_Y + x];

                transformSequence_sourceX.push_back(transformValue_x);
                transformSequence_sourceY.push_back(transformValue_y);

            }
        }
    }


}

//#include "myutility.h"

UInt ICU_Encoder::calcDistributionsBasedOnResidual(const ComponentID compId, const MyIntraPrediction::REFERENCE_SAMPLE_SIDE rfSamp, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight, std::vector<TCoeff> &transformSequence_sourceX, std::vector<TCoeff> &transformSequence_sourceY, std::vector<int> &residual, std::map<int, double> &probs_residualXminusY, bool compressBasedOnAbsValues, int maxLog2DynamicRange)
{

    getTransformSequences(compId, rfSamp, bandIndex, tuWidth, tuHeight, transformSequence_sourceX, transformSequence_sourceY);
    residual.reserve(transformSequence_sourceX.size());
    residual.clear();
    CHECK(transformSequence_sourceX.size() != transformSequence_sourceY.size(), "transform sequence vector size do not match");
    for(unsigned int i=0; i < transformSequence_sourceX.size(); i++)
        residual.push_back(transformSequence_sourceX[i] - transformSequence_sourceY[i]);

    probs_residualXminusY.clear();

    if(compressBasedOnAbsValues)
    {
        const int entropyCodingMaximumAbs =  (1 << maxLog2DynamicRange);
        uint32_t n_range = entropyCodingMaximumAbs + 1;

        uint32_t lastAbsPosition =0;


        for(unsigned int i=0; i < residual.size(); i++)
        {
            int b = abs(residual[i]);
            if (b == EOF)
                break;
            if (b < 0 || b >= n_range)
                throw std::logic_error("Assertion error");

            uint32_t position = static_cast<uint32_t>(b);
            probs_residualXminusY[position] += 1.;

            if(position > lastAbsPosition)
                lastAbsPosition = position;

        }
        n_range = lastAbsPosition + 1;    // new range
        probs_residualXminusY[n_range] += 1.;
    }
    else
    {
        for(unsigned int i=0; i < residual.size(); i++)
            probs_residualXminusY[residual[i]] += 1.;
        // TODO: I don't have any idea to have eof symbol like compressBasedOnAbsValues
    }

    UInt sequenceLength = residual.size();

    if(compressBasedOnAbsValues)
        sequenceLength++;   // for eof which is used a few lines above for probs_residualXminusY[n_range] += 1.;

    double entropy = 0.;
    double f;

    for (auto & x : probs_residualXminusY)
    {
        x.second = x.second / double(sequenceLength);

        f = x.second * std::log2(x.second);
//        if(std::isinf(f) && (fabs(x.second) < 1e-13) ) // to solve the problem of "epsilon * log2(number/epsilon)"
//            f = 0.;
        if(f != f) // if it is nan make it zero
            f=0.;
        entropy -= f;

    }

    SideInformationUnit *SI_unit = get_sideInformation(rfSamp);
    UInt n_bits = UInt(std::ceil(entropy * (double)sequenceLength));
//    UInt n_bits = UInt(entropy * (double)sequenceLength);
    if(compressBasedOnAbsValues)    // if absolut values are used, don't forget to encode the signs
        n_bits += residual.size();
    SI_unit->bitsToDecode[compId][bandIndex] = n_bits;


//    if(compressBasedOnAbsValues)
//    {
//        UInt test =UInt(std::ceil(MyUtility::computeTheoreticalRateWithEntropyForCompressPrecomputeFrequencyTable2ByteValuesAdaptiveRangeAbsoluteValues(residual, false, maxLog2DynamicRange)));
//        if( test !=n_bits)
//        {
//            if(test >= n_bits)
//            {
//                std::cout << "ICU_Encoder::theoretical = " << n_bits << std::endl;
//                std::cout << "MyUtility::theoretical = " << test << std::endl;
//                std::cout << "MyUtility::theoretical has higher rate. difference = " << test - n_bits << std::endl;
//            }
//            else
//            {
//                std::cout << "ICU_Encoder::theoretical = " << n_bits << std::endl;
//                std::cout << "MyUtility::theoretical = " << test << std::endl;
//                std::cout << "ICU_Encoder::theoretical has higher rate. difference = " << n_bits - test << std::endl;
//            }
//            throw std::logic_error("error in rate computation");
//        }
//    }

    return n_bits;

}

void ICU_Encoder::calcDistributions(const ComponentID compId, std::vector<UInt> &numberOfDecodingBits, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight, std::vector<std::vector<TCoeff> > &transformSequence_sourceX, std::vector<std::vector<TCoeff> > &allSIs_transformSequence_sourceY, std::vector<std::map<TCoeff, std::map<TCoeff, double> > > &allSIs_probs_XY, std::vector<std::map<TCoeff, std::map<TCoeff, double> > > &allSIs_probs_YX, std::vector<std::map<TCoeff, double> > &allSIs_probs_X, std::vector<std::map<TCoeff, double> > &allSIs_probs_Y)
{
    UInt nSIs = numberOfSIs();
    numberOfDecodingBits.clear();
    transformSequence_sourceX.clear();
    allSIs_transformSequence_sourceY.clear();
    allSIs_probs_XY.clear();
    allSIs_probs_YX.clear();
    allSIs_probs_X.clear();
    allSIs_probs_Y.clear();

    numberOfDecodingBits.resize(nSIs, 0);
    transformSequence_sourceX.resize(nSIs); // this must be similar for all element
    allSIs_transformSequence_sourceY.resize(nSIs);
    allSIs_probs_XY.resize(nSIs);
    allSIs_probs_YX.resize(nSIs);
    allSIs_probs_X.resize(nSIs);
    allSIs_probs_Y.resize(nSIs);

    UInt i=0;
    for (auto const& intraSIType : _mapIntraSIs)
    {
        numberOfDecodingBits[i] = calcDistributions(compId, intraSIType.first, bandIndex, tuWidth, tuHeight,
                                       transformSequence_sourceX[i], allSIs_transformSequence_sourceY[i],
                                       allSIs_probs_XY[i], allSIs_probs_YX[i],
                                       allSIs_probs_X[i], allSIs_probs_Y[i]);   // function returns conditional entropy of XgivenY
        i++;
    }
}

void ICU_Encoder::calcDistributionsBasedOnResidual(const ComponentID compId, std::vector<UInt> &numberOfDecodingBits, const UInt bandIndex, const UInt tuWidth, const UInt tuHeight, std::vector<std::vector<TCoeff> > &transformSequence_sourceX, std::vector<std::vector<TCoeff> > &allSIs_transformSequence_sourceY, std::vector<std::vector<int> >& vec_residual, std::vector<std::map<int, double> > &vec_probs_residualXminusY, bool compressBasedOnAbsValues, int maxLog2DynamicRange)
{
    UInt nSIs = numberOfSIs();
    numberOfDecodingBits.clear();
    transformSequence_sourceX.clear();
    allSIs_transformSequence_sourceY.clear();
    vec_residual.clear();
    vec_probs_residualXminusY.clear();

    numberOfDecodingBits.resize(nSIs, 0);
    transformSequence_sourceX.resize(nSIs); // this must be similar for all element
    allSIs_transformSequence_sourceY.resize(nSIs);
    vec_residual.resize(nSIs);
    vec_probs_residualXminusY.resize(nSIs);

    UInt i=0;
    for (auto const& intraSIType : _mapIntraSIs)
    {
        numberOfDecodingBits[i] = calcDistributionsBasedOnResidual(compId, intraSIType.first, bandIndex, tuWidth, tuHeight,
                                       transformSequence_sourceX[i], allSIs_transformSequence_sourceY[i],
                                       vec_residual[i], vec_probs_residualXminusY[i],
                                       compressBasedOnAbsValues, maxLog2DynamicRange);   // function returns conditional entropy of XgivenY
        i++;
    }
}



bool ICU_Encoder::addTU(const UnitArea &unit)
{
    MyTransformUnit* tu_ptr;
    tu_ptr=_sourceX.addTU(unit);
    if(!tu_ptr)
    {
        std::cerr << "ICU_Encoder::addTU Error adding transform unit" << std::endl;
        return false;
    }


    // Iterate through all elements in std::map
    for (auto it=_mapIntraSIs.begin(); it!=_mapIntraSIs.end(); ++it)   // loop over all intra side information
    {
        SideInformationUnit& si = it->second;
        tu_ptr=si.addTU(unit);
        if(!tu_ptr)
        {
            std::cerr << "ICU_Encoder::addTU Error adding transform unit for side information" << std::endl;
            return false;
        }

    }
    return true;
}
