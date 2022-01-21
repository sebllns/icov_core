#include "DecodingBlock.h"



bool DecodingBlock::availableForDecoding()
{
    return !decoded && !unused;
}

bool DecodingBlock::isDecoded()
{
    return decoded && !unused;
}

PredictionMask DecodingBlock::getPredictionMask()
{
    if(unused || isAccessBlock) return 0;

    PredictionMask pred = 0;

    for(int i = 0; i < 4 ; i++)
    {
        if(neighbors[i] && neighbors[i]->isDecoded()) pred += (1 << i);
    }

    return pred;
}

DecodingBlock::DecodingBlock()
{
    unused = false;
    decoded = false;

    for(int i = 0; i < 4 ; i++)
    {
        neighbors[i] = nullptr;
    }
}

DecodingBlock::DecodingBlock(float px, float py, float pw, float ph):Block(px, py, pw, ph)
{
    unused = false;
    decoded = false;
    isAccessBlock = false;
}
