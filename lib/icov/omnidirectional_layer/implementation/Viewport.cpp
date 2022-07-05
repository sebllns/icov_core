#include "Viewport.h"

//Viewport::Viewport(float pcenter_x, float pcenter_y, float pfov_x, float pfov_y):
//    center_x(pcenter_x),
//    center_y(pcenter_y),
//    fov_x(pfov_x),
//    fov_y(pfov_y)
//{

//}

//void Viewport::computeOrdering()
//{
//    std::cout << "****************************************" << std::endl;

//    order = BlockOrder(icus.size());

//    RowMinMax available(rows);
//    int16_t block_row = 0;
//    int16_t block_offset = 0;
//    int16_t minrowidx = 0;
//    int16_t maxrowidx = available.size();

//    getBlockCoords(*icus[access_block_index].get(), block_row, block_offset);

//    int i = 0;
//    while(i < order.size())
//    {
//        std::cout << i << std::endl;

//        if(available[block_row].first == getBlockIndex(block_row, block_offset) && available[block_row].second == getBlockIndex(block_row, block_offset))
//        {
//            order[i++] = getBlockIndex(block_row, block_offset);
//            available[block_row].second = available[block_row].first - 1;
//        }

//        int16_t right = getBlockIndex(block_row, block_offset + 1);
//        int16_t left = getBlockIndex(block_row, block_offset - 1);
//        int16_t up = (block_row > minrowidx) ? getBlockIndex(block_row-1, block_offset) : 0;
//        int16_t down = (block_row < maxrowidx) ? getBlockIndex(block_row+1, block_offset) : 0;

//        if(right > available[block_row].first && right <= available[block_row].second)
//        {
//            int16_t count = available[block_row].second - right + 2;
//            std::iota(order.begin()+i, order.begin()+i+count, right - 1);
//            i += count;
//            std::cout << "right " << count << std::endl;

//            block_offset = available[block_row].second - available[block_row].first;
//            available[block_row].second -= count;
//        }
//        else if(left >= available[block_row].first && left < available[block_row].second)
//        {
//            int16_t count = left - available[block_row].first + 2;
//            icov::helpers::iota(order.begin()+i, order.begin()+i+count, (int)left + 1, -1);
//            available[block_row].first = left + 2;
//            i += count;
//            std::cout << "left " << count << std::endl;

//            block_offset = 0;
//        }
//        else if(block_row > minrowidx && up >= available[block_row-1].first && up <= available[block_row-1].second)
//        {
//            //int offset = currentblock -  available[block_row].first;

//            //up available
//            //up remove
//            //currentblock = up;
//            block_row--;
//            std::cout << "up " << std::endl;
//        }
//        else  if(block_row < maxrowidx && down >= available[block_row+1].first && down <= available[block_row+1].second)
//        {
//            //down available
//            //down remove
//            //currentblock = down;
//            block_row++;
//            std::cout << "down " << std::endl;
//        }
//        else
//        {
//            //jump
//            std::cout << "jump " << std::endl;
//            int16_t searchrow = 0;

//            if(block_row > 0)
//            {
//                searchrow = block_row - 1;
//                while(searchrow > 0 && available[searchrow].second < available[searchrow].first)
//                {
//                    searchrow--;
//                }
//            }

//            if(block_row == 0 || available[searchrow].second < available[searchrow].first)
//            {
//                searchrow = std::min(block_row + 1, (int16_t)available.size() - 1);
//                while(searchrow < (available.size() - 1) && available[searchrow].second < available[searchrow].first)
//                {
//                    searchrow++;
//                }
//            }

//            block_row = searchrow;
//            block_offset = 0;

//        }
//    }

//    //        int rowlength = 1 + available[currentRow].second - available[currentRow].first;
//    //        //update minrowidx & maxrowidx
//    //        if(rowlength == 0 && currentRow )
//}

//void Viewport::computeRows()
//{
//    float u, v;
//    //u = icus.begin()->get()->u;
//    v = v_min;

//    uint32_t rowcount = 1+(v_max - v_min)/icus.begin()->get()->h;

//    std::cout << "rowcount=" << rowcount << std::endl;

//    rows = RowMinMax(rowcount, MinMax(0,0));


//    int rowidx = 0;
//    for(int i = 0; i < (int)icus.size() ; i++)
//    {
//        float iv = icov::helpers::maths::wrap(icus[i]->v, v_min, v_max);
//        if(iv > v)
//        {
//            rowidx++;
//            v = iv;
//            rows[rowidx].first = i;
//        }
//        rows[rowidx].second = i;
//    }
//}

//int16_t Viewport::rowLength(int16_t row)
//{
//    return 1 + rows[row].second - rows[row].first;
//}

//void Viewport::getBlockCoords(const Block &b, int16_t &o_row, int16_t &o_offset)
//{
//    const auto &tl = icus.begin()->get();
//    const auto &br = icus.back();

//    float tl_u = tl->u, tl_v = tl->v, br_u = br->u, br_v = br->v, u = b.u, v = b.v;

//    normalizeUV(tl_u, tl_v);
//    normalizeUV(br_u, br_v);
//    normalizeUV(u, v);

//    o_row = (int16_t)((rows.size()-1) * v);

//    int16_t min = rows[o_row].first;
//    int16_t max = rows[o_row].second;

//    const auto &left = icus[min];
//    const auto &right = icus[max];

//    float l_u = left->u, r_u = right->u;
//    //normalizeUV(l_u, r_u);

//     l_u = icov::helpers::maths::normalize(icov::helpers::maths::wrap(l_u, u_min, u_max), u_min, u_max);
//     r_u = icov::helpers::maths::normalize(icov::helpers::maths::wrap(r_u, u_min, u_max), u_min, u_max);

//    o_offset = (int16_t)((max - min) * icov::helpers::maths::normalize(b.u, l_u, r_u));
//}

//int16_t Viewport::getBlockIndex(int16_t row, int16_t offset)
//{
//    return rows[row].first + offset;
//}

//void Viewport::findNextFreeBlock(int16_t &row, int16_t &offset, bool down)
//{
//    int direction = down ? 1 : -1;

//    row += direction;

//    while(rows[row].first + offset > rows[row].second)
//    {
//        offset = 0;
//        row += direction;
//    }
//}

//void Viewport::normalizeUV(float &u, float &v)
//{
//    u = icov::helpers::maths::normalize(icov::helpers::maths::wrap(u, u_min, u_max), u_min, u_max);
//    v = icov::helpers::maths::normalize(icov::helpers::maths::wrap(v, v_min, v_max), v_min, v_max);
//}
