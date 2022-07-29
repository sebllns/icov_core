/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef VTM_UTILITY_H
#define VTM_UTILITY_H
#include "CommonLib/Unit.h"
#include "encodingstructure.h"

namespace VTM_Utility
{
    Area& operator &= ( Area& a, const Area& b );
    Area  operator & (const Area& a, const Area& b);

    template <class DataType>
    void flipHoriz(const AreaBuf<const DataType>& srcBuffer, AreaBuf<DataType>& dstBuffer)
    {
        CHECK( (srcBuffer.width != dstBuffer.width) || (srcBuffer.height != dstBuffer.height), "sizes do not match");
        const DataType* src = srcBuffer.buf;
        DataType* dst = dstBuffer.buf;

        DataType t0,t1;
        int n = (srcBuffer.width+1) /2;    // Iterate only half the buffer to get a full flip
        int x,y,reverseIndex;
        int rowLastIndex = srcBuffer.width - 1;
        for(unsigned y=0; y < srcBuffer.height; y++, src += srcBuffer.stride, dst += dstBuffer.stride)
        {
            for(x=0; x < n; x++)
            {
                reverseIndex = rowLastIndex - x;

                t0 = src[x];
                t1 = src[reverseIndex];

                dst[reverseIndex] = t0;
                dst[x] = t1;
            }
        }
    }

    template <class DataType>
    void flipVert(const AreaBuf<const DataType>& srcBuffer, AreaBuf<DataType>& dstBuffer)
    {
        CHECK( (srcBuffer.width != dstBuffer.width) || (srcBuffer.height != dstBuffer.height), "sizes do not match");
        const DataType* src0 = srcBuffer.buf;
        const DataType* src1 = srcBuffer.buf + (srcBuffer.height-1) * srcBuffer.stride;
        DataType* dst0 = dstBuffer.buf;
        DataType* dst1 = dstBuffer.buf + (dstBuffer.height-1) * dstBuffer.stride;

        int n = (srcBuffer.height+1) /2;    // Iterate only half the buffer to get a full flip
        DataType t0,t1;

        for(int y=0; y < n; y++, src0 += srcBuffer.stride, src1 -= srcBuffer.stride,
                                 dst0 += dstBuffer.stride, dst1 -= dstBuffer.stride)
        {
            for(unsigned int x=0; x < srcBuffer.width; x++)
            {
                t0 = src0[x];
                t1 = src1[x];

                dst0[x] = t1;
                dst1[x] = t0;
            }
        }

    }

    template <class T>
    inline AreaBuf<T> getAreaBuf(const UnitBuf<T>& unitBuffer, const CompArea &blk )
    {
      const AreaBuf<T>& r = unitBuffer.get(blk.compID);
//      CHECKD( rsAddr( blk.bottomRight(), r.stride ) >= ( ( r.height - 1 ) * r.stride + r.width ), "Trying to access a buf outside of bound!" );
      return AreaBuf<T>( r.buf + rsAddr( blk, r.stride ), r.stride, blk );
    }


    template <class T>
    inline void setNotMaskTo(AreaBuf<T>& buff, const AreaBuf<bool>& buff_mask, const T& value) // everything outside mask is set to value
    {
        CHECK((buff.width!=buff_mask.width) || (buff.height!=buff_mask.height), "size must be equal");

        for(unsigned int y=0; y < buff.height; y++)
        {
            T* buff_rowPtr = &buff.at(0, y);
            const bool* mask_rowPtr = &buff_mask.at(0, y);
            for(unsigned int x=0; x < buff.width; x++)
            {
                if(!mask_rowPtr[x])
                    buff_rowPtr[x] = value;
            }

        }
    }

    Void positionRelativeTo (UnitArea& inputOutput, const Position& origin);


    /*
     * Finds connected component for 360 image (traversing in x direction is continous). inputOutput is modified by this function
     * The function returns blocks of connected componenet in snake scanning order. So the order of blocks in each connected components is already optimal for interactive coding.
    */
    void findConnectedComponents_360(AreaBuf<bool>& inputOutput, std::vector<std::vector< Position> >& connectedComponents);

    double calculatePSNR(const CPelUnitBuf& org, const CPelUnitBuf& cur, const Int internalBitDepth[MAX_NUM_CHANNEL_TYPE], double *mse_ptr=nullptr, uint *maxval_ptr = nullptr);

    void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const int bitDepths[MAX_NUM_CHANNEL_TYPE], int maxLog2TrDynamicRange);

    void quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage, EncodingStructure &encStruct, const ComponentID compID, const int bitDepths[MAX_NUM_CHANNEL_TYPE], int maxLog2TrDynamicRange);


}

#endif // VTM_UTILITY_H
