#include "QpParam.h"
#include "CommonLib/ChromaFormat.h"

icov::video::ICOV_QpParam::ICOV_QpParam(const int qpy, const ChannelType chType, const ChromaFormat chFmt, const int channelBitDepth, const int chromaQPOffset, const int dqp)
{
    channelType = chType;
    chromaFomat = chFmt;
    /*
            the quantizer step size is in the range of 0.630 ≤ Δq ≤ 228.1. Thereby, the maximum
            quantizer step size is in the range of the maximum amplitude of the 8 bit input signal.
            For higher bit-depths, a QP offset is specified, increasing the QP parameter range by
            6 per additional bit of signal bit depth towards finer quantization (i.e. towards lower
            QP values).
         */
    const int qpBdOffset = 6 * (channelBitDepth - 8); //seul changement ici
    int baseQp;

    if(isLuma(chType))
    {
        baseQp = qpy + qpBdOffset;
    }
    else
    {
        baseQp = Clip3( -qpBdOffset, (chromaQPMappingTableSize - 1), qpy + chromaQPOffset );

        if(baseQp < 0)
        {
            baseQp = baseQp + qpBdOffset;
        }
        else
        {
            baseQp = getScaledChromaQP(baseQp, chFmt) + qpBdOffset;
        }
    }

    baseQp = Clip3( 0, MAX_QP+qpBdOffset, baseQp + dqp );

    Qp =baseQp;
    per=baseQp/6;
    rem=baseQp%6;
}
