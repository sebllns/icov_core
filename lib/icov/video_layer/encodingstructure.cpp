/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "encodingstructure.h"

#include <CommonLib/Buffer.h>
#include "myintraprediction.h"

EncodingStructure::EncodingStructure()
{
    deallocate();
}

EncodingStructure::EncodingStructure(const std::vector<ICU_Encoder> &icus, const UInt icusPerRow, const UInt icusPerColumn)
{
    set(icus, icusPerRow, icusPerColumn);
}

EncodingStructure::~EncodingStructure()//#include <opencv2/opencv.hpp>

{
    deallocate();
}

void EncodingStructure::set(const std::vector<ICU_Encoder> &icus, const UInt icusPerRow, const UInt icusPerColumn)
{
    deallocate();

    vec_icus = icus;
    n_icusPerRow = icusPerRow;
    n_icusPerCol = icusPerColumn;
    CHECK(vec_icus.size()!=n_icusPerRow * n_icusPerCol, "incorrect number of icus per row and per column");

    bool* reqPtr = (bool*)xMalloc( bool, n_icusPerRow * n_icusPerCol );
    _requestedICUs = AreaBuf<bool>(reqPtr, n_icusPerRow, n_icusPerRow, n_icusPerCol);
    bool* prePtr = (bool*)xMalloc( bool, n_icusPerRow * n_icusPerCol );
    _decodedICUs = AreaBuf<bool>(prePtr, n_icusPerRow, n_icusPerRow, n_icusPerCol);

    UInt n_icus = n_icusPerRow * n_icusPerCol;
    _vec_pixelsRequestedPerICU.resize(n_icus);

    _vec_decodedICUs.reserve(n_icus);


    _vec_tmp_alreadyProcessedICUs.reserve(n_icus);
    bool* alreadyProPtr = (bool*)xMalloc( bool, n_icusPerRow * n_icusPerCol );
    _tmp_alreadyProcessedICUs = AreaBuf<bool>(alreadyProPtr, n_icusPerRow, n_icusPerRow, n_icusPerCol);


}

void EncodingStructure::setCheckPoints(const std::vector<Position> &checkPoints)
{
    for(unsigned int i=0; i < checkPoints.size(); i++)
        setCheckPoint(checkPoints[i]);
}

void EncodingStructure::setCheckPoint(const Position &checkPoint)
{
    CHECK( (checkPoint.x < 0) || (checkPoint.x >=int(n_icusPerRow)) || (checkPoint.y < 0) || (checkPoint.y >=int(n_icusPerCol)), "check point is not inside the image");
    for(unsigned int i=0; i < vec_checkPoints.size(); i++)
    {
        if( vec_checkPoints[i].x==checkPoint.x && vec_checkPoints[i].y==checkPoint.y)
        {
            std::cout << "warning: added check point already exists." << std::endl;
            return;
        }
    }
    vec_checkPoints.push_back(checkPoint);
}

bool EncodingStructure::isAccessBlock(const Position &accessBlockPoint)
{
    for(unsigned int ab_ind=0; ab_ind < vec_checkPoints.size(); ab_ind++)
        if( vec_checkPoints[ab_ind].x==accessBlockPoint.x && vec_checkPoints[ab_ind].y==accessBlockPoint.y)
            return true;

    return false;
}

Position EncodingStructure::get_icuPosition(const UInt icu_index) const
{
    div_t divresult; divresult = div(icu_index, Int(n_icusPerRow));
    UInt icu_x = divresult.rem;
    UInt icu_y = divresult.quot;
    Position block_position(icu_x, icu_y);

    if(this->get_icuIndex(block_position) != icu_index)
        throw std::logic_error("The output block position is not correct. Something is missing");

    return block_position;
}


static const int dx[4] = {1,-1, 0, 0};
static const int dy[4] = {0, 0, -1, 1};
static void dfs_360(AreaBuf<bool> &requestedIcus, const AreaBuf<bool>& alreadyDecodedICus, std::vector<Position>  &currentComponent_requestedICUs, int x,int y, int& remainingICUs)
{
    if( !alreadyDecodedICus.at(x,y) )    // add only if it is not decoded
        currentComponent_requestedICUs.push_back(Position(x,y));

    remainingICUs--;
    requestedIcus.at(x,y) = false;
    int iWidth = int(requestedIcus.width);    // if we do not convert to int, the traversing equation for 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth) converts everything to UInt
    int iHeight = int(requestedIcus.height);
    for(int i=0; i<4;i++)
    {
        int neighbor_x = x+dx[i];
        neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        int neighbor_y = y+dy[i];
        if (neighbor_y < 0 || neighbor_y >= iHeight) // do not check outside image in y direction
            continue;
        if(requestedIcus.at(neighbor_x,neighbor_y))
            dfs_360(requestedIcus, alreadyDecodedICus, currentComponent_requestedICUs, neighbor_x,neighbor_y, remainingICUs);
    }
}

static void dfs_360(AreaBuf<bool> &requestedIcus, const Position& checkPoint_pos, std::vector<Position> &currentComponent_requestedICUs, int x,int y, int& remainingICUs)
{
    if(x!=checkPoint_pos.x || y!=checkPoint_pos.y)
        currentComponent_requestedICUs.push_back(Position(x,y));

    remainingICUs--;
    requestedIcus.at(x,y) = false;
    int iWidth = int(requestedIcus.width);    // if we do not convert to int, the traversing equation for 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth) converts everything to UInt
    int iHeight = int(requestedIcus.height);
    for(int i=0; i<4;i++)
    {
        int neighbor_x = x+dx[i];
        neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        int neighbor_y = y+dy[i];
        if (neighbor_y < 0 || neighbor_y >= iHeight) // do not check outside image in y direction
            continue;
        if(requestedIcus.at(neighbor_x,neighbor_y))
            dfs_360(requestedIcus, checkPoint_pos, currentComponent_requestedICUs, neighbor_x,neighbor_y, remainingICUs);
    }
}

static void dfs_360(AreaBuf<bool> &requestedIcus, std::vector<Position>  &currentComponent_requestedICUs, int x,int y, int& remainingICUs)
{

//    if( !requestedIcus.at(x,y) )    // add only if it is not decoded
    CHECK(!requestedIcus.at(x,y), "Something is wrong. The requestedIcus in this position must be true.");
    currentComponent_requestedICUs.push_back(Position(x,y));
    remainingICUs--;
    requestedIcus.at(x,y) = false;
    int iWidth = int(requestedIcus.width);    // if we do not convert to int, the traversing equation for 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth) converts everything to UInt
    int iHeight = int(requestedIcus.height);
    for(int i=0; i<4;i++)
    {
        int neighbor_x = x+dx[i];
        neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        int neighbor_y = y+dy[i];
        if (neighbor_y < 0 || neighbor_y >= iHeight) // do not check outside image in y direction
            continue;
        if(requestedIcus.at(neighbor_x,neighbor_y))
            dfs_360(requestedIcus, currentComponent_requestedICUs, neighbor_x,neighbor_y, remainingICUs);
    }
}

static void fillHolesHorizontally_360(AreaBuf<bool> &requestedIcus, int rowIndex, int &n_requestedICUs)
{


    int iWidth  = int(requestedIcus.width);    // if we do not convert to int, the traversing equation for 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth) converts everything to UInt

    bool* requested_rowPtr = &requestedIcus.at(0,rowIndex);
    int x_pre, x_next;
    for(int x=0; x < iWidth; x++)
    {
        x_pre  = (iWidth + (int(x-1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        x_next = (iWidth + (int(x+1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        if(!requested_rowPtr[x])    // if the icu is false, but the previous icu and the next icu are true, then fill the hole
        {
            if(requested_rowPtr[x_pre] && requested_rowPtr[x_next])
            {
                requested_rowPtr[x] = true;
                n_requestedICUs++;
            }
        }

    }
}

void EncodingStructure::reorderBlockDecodingBasedOnStartingAccessBlock(std::vector<std::vector<Position> > &in_out_requested_icus, const std::vector<Position> &accessBlockInsideAsStartingPosition)
{
    CHECK(in_out_requested_icus.size() != accessBlockInsideAsStartingPosition.size(), "Size of the vector must be equal.");
    _requestedICUs.fill(false);

    int n_remainingRequestedICUs = 0;

    for(unsigned int cc_id=0; cc_id < accessBlockInsideAsStartingPosition.size(); cc_id++)
    {
        if(!isAccessBlock(accessBlockInsideAsStartingPosition[cc_id]))
            throw std::runtime_error("This is not an access block.");

        n_remainingRequestedICUs += int(in_out_requested_icus[cc_id].size());

        for(unsigned int block_ind=0; block_ind < in_out_requested_icus[cc_id].size(); block_ind++)
            _requestedICUs.at(in_out_requested_icus[cc_id][block_ind]) = true;

        if (!_requestedICUs.at(accessBlockInsideAsStartingPosition[cc_id]))
        {
            n_remainingRequestedICUs++;
            _requestedICUs.at(accessBlockInsideAsStartingPosition[cc_id]) = true;
        }
    }


    in_out_requested_icus.clear();
    for(unsigned int cc_id=0; cc_id < accessBlockInsideAsStartingPosition.size(); cc_id++)
    {

        const Position& chPoint_pos = accessBlockInsideAsStartingPosition[cc_id];
        if(_requestedICUs.at(chPoint_pos.x, chPoint_pos.y))
        {

            in_out_requested_icus.push_back(std::vector<Position>());
            dfs_360(_requestedICUs, chPoint_pos, in_out_requested_icus.back(), chPoint_pos.x, chPoint_pos.y, n_remainingRequestedICUs);
//            Check the above line


        }

    }

    CHECK(n_remainingRequestedICUs !=0, "something is missing, it should process all input blocks.");

}

static void dfs_360(AreaBuf<bool> &requestedIcus, const AreaBuf<bool>& alreadyDecodedICus, std::vector<Position>  &currentComponent_requestedNotDecodedICUs, std::vector<Position>  &currentComponent_requestedDecodedICUs, int x,int y)
{
    if(! alreadyDecodedICus.at(x,y))    // add only if it is not decoded
        currentComponent_requestedNotDecodedICUs.push_back(Position(x,y));
    else
        currentComponent_requestedDecodedICUs.push_back(Position(x,y));


    requestedIcus.at(x,y) = false;
    int iWidth = int(requestedIcus.width);    // if we do not convert to int, the traversing equation for 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth) converts everything to UInt
    int iHeight = int(requestedIcus.height);
    for(int i=0; i<4;i++)
    {
        int neighbor_x = x+dx[i];
        neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        int neighbor_y = y+dy[i];
        if (neighbor_y < 0 || neighbor_y >= iHeight) // do not check outside image in y direction
            continue;
        if(requestedIcus.at(neighbor_x,neighbor_y))
            dfs_360(requestedIcus, alreadyDecodedICus, currentComponent_requestedNotDecodedICUs, currentComponent_requestedDecodedICUs, neighbor_x, neighbor_y);
    }
}

int EncodingStructure::getAvailableDecodedSides_regularImage(const Position &icuPos) const
{
    int availableSides = 0;

    int horizontal_y;
    int iWidth = _decodedICUs.width;
    int iHeight = _decodedICUs.height;

    const bool* rowPtr=nullptr;
    // check top side
    horizontal_y = icuPos.y-1;
    if( (horizontal_y < iHeight) && (horizontal_y >=0) )
    {
        rowPtr = &_decodedICUs.at(0,horizontal_y);
        if( (icuPos.x-1 >=0) && (rowPtr[icuPos.x-1]==true))  // check top-left corner
            availableSides |= 0b000100000;
        if( (icuPos.x+1 < iWidth) && (rowPtr[icuPos.x+1]==true))  // check top-right corner
            availableSides |= 0b001000000;

        if(rowPtr[icuPos.x]==true)
            availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::TOP;
    }

    // check bottom side
    horizontal_y = icuPos.y+1;
    if( (horizontal_y < iHeight) && (horizontal_y >=0) )
    {
        rowPtr = &_decodedICUs.at(0,horizontal_y);
        if( (icuPos.x-1 >=0) && (rowPtr[icuPos.x-1]==true))  // check bottom-left corner
            availableSides |= 0b010000000;
        if( (icuPos.x+1 < iWidth) && (rowPtr[icuPos.x+1]==true))  // check bottom-right corner
            availableSides |= 0b100000000;

        if(rowPtr[icuPos.x]==true)
            availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::DOWN;
    }

    rowPtr = &_decodedICUs.at(0,icuPos.y);

    if( (icuPos.x-1 >=0) && (rowPtr[icuPos.x-1]==true)) // check left side
        availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::LEFT;

    if( (icuPos.x+1 < int(n_icusPerRow)) && (rowPtr[icuPos.x+1]==true))  // check right side
        availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::RIGHT;

    return availableSides;

}

int EncodingStructure::getAvailableDecodedSides_360Image(const Position &icuPos) const
{
    int availableSides = 0;
//        neighbor_x = (iWidth + (neighbor_x%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction

    int horizontal_y;
    int iWidth = _decodedICUs.width;
    int iHeight = _decodedICUs.height;
    int x;

    const bool* rowPtr=nullptr;
    // check top side
    horizontal_y = icuPos.y-1;
    if( (horizontal_y < iHeight) && (horizontal_y >=0) )
    {
        rowPtr = &_decodedICUs.at(0,horizontal_y);

        x = (iWidth + ((icuPos.x-1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        if( rowPtr[x]==true)  // check top-left corner
            availableSides |= 0b000100000;

        x = (iWidth + ((icuPos.x+1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        if( rowPtr[x]==true )  // check top-right corner
            availableSides |= 0b001000000;

        if(rowPtr[icuPos.x]==true)
            availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::TOP;
    }

    // check bottom side
    horizontal_y = icuPos.y+1;
    if( (horizontal_y < iHeight) && (horizontal_y >=0) )
    {
        rowPtr = &_decodedICUs.at(0,horizontal_y);

        x = (iWidth + ((icuPos.x-1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        if( rowPtr[x]==true)  // check bottom-left corner
            availableSides |= 0b010000000;

        x = (iWidth + ((icuPos.x+1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
        if( rowPtr[x]==true)  // check bottom-right corner
            availableSides |= 0b100000000;

        if(rowPtr[icuPos.x]==true)
            availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::DOWN;
    }

    rowPtr = &_decodedICUs.at(0,icuPos.y);

    x = (iWidth + ((icuPos.x-1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
    if( rowPtr[x]==true) // check left side
        availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::LEFT;

    x = (iWidth + ((icuPos.x+1)%iWidth)) % iWidth;  // to generate circular traversing for 360 video in x direction
    if( rowPtr[x]==true)  // check right side
        availableSides |= MyIntraPrediction::REFERENCE_SAMPLE_SIDE::RIGHT;

    return availableSides;
}

void EncodingStructure::updateAlreadyProccessedICU(const Position &ICU_position)
{
    CHECK(ICU_position.x < 0 || ICU_position.x >= Int(n_icusPerRow) || ICU_position.y < 0 || ICU_position.y >= Int(n_icusPerCol), "out of range");
    bool &val = _tmp_alreadyProcessedICUs.at(ICU_position.x, ICU_position.y);
    if(!val)
    {
        val = true;
        _vec_tmp_alreadyProcessedICUs.push_back(ICU_position);
    }
}


void EncodingStructure::deallocate()
{
    if(_requestedICUs.buf)
    {
        std::cout << "deallocating encoding structure" << std::endl;
        xFree( _requestedICUs.buf);
        _requestedICUs = AreaBuf<bool>();
//        requestedBlocks.buf = nullptr;
//        requestedBlocks.stride = 0;
    }

    if(_decodedICUs.buf)
    {
        xFree( _decodedICUs.buf);
        _decodedICUs = AreaBuf<bool>();
//        previousDecodedBlocks.buf = nullptr;
//        previousDecodedBlocks.stride = 0;
    }

//    _tmp_alreadyProcessedICUs
    if(_tmp_alreadyProcessedICUs.buf)
    {
        xFree( _tmp_alreadyProcessedICUs.buf);
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
