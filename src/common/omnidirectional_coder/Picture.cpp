#include "Picture.h"

//phi = y
//theta = x
Picture::Picture(int pw, int ph):
    w(pw),
    h(ph)
{
    fov_x = 2 * M_PI;
    fov_y = M_PI;

    center_x = fov_x/2;
    center_y = fov_y/2;
}

void Picture::buildConstantSizeBlocks(int pw, int ph)
{
    block_cols = (int)std::ceil((double)w/pw);
    block_rows = (int)std::ceil((double)h/ph);

    total_w = block_cols * pw;
    total_h = block_rows * ph;

    float bw = 1.0/block_cols;
    float bh = 1.0/block_rows;

    icus.resize(block_cols * block_rows);

    for(int i = 0; i < block_rows ; i++)
    {
        for(int j = 0; j < block_cols ; j++)
        {
            icus[i*block_cols + j] = std::make_shared<Block>(j*bw, i*bh, bw, bh);
        }
    }
}

shared_ptr<Viewport> Picture::getViewport(float pcenter_x, float pcenter_y, float pfov_x, float pfov_y)
{
    auto viewport = std::make_shared<Viewport>(pcenter_x, pcenter_y, pfov_x, pfov_y);

    float u_min = u(pcenter_x - 0.5 * pfov_x);
    float u_max = u(pcenter_x + 0.5 * pfov_x);

    while(u_min < 0)
    {
        u_min += 1.0;
        u_max += 1.0;
    }

    float v_min = v(pcenter_y - 0.5 * pfov_y);
    float v_max = v(pcenter_y + 0.5 * pfov_y);

    while(v_min < 0)
    {
        v_min += 1.0;
        v_max += 1.0;
    }

    int i_min, i_max, j_min, j_max = 0;

    getBlockCoords(u_min, v_min, i_min, j_min);
    getBlockCoords(u_max, v_max, i_max, j_max);

    viewport->u_min = getBlock(i_min, j_min)->u;
    viewport->u_max = getBlock(i_max, j_max)->u;
    viewport->v_min = getBlock(i_min, j_min)->v;
    viewport->v_max = getBlock(i_max, j_max)->v;

    std::cout << "umin, umax = " << u_min << ", " << u_max << std::endl;
    std::cout << "vmin, vmax = " << v_min << ", " << v_max << std::endl;


    while(viewport->u_max < viewport->u_min)
    {
        viewport->u_max += 1.0;
    }

    while(viewport->v_max < viewport->v_min)
    {
        viewport->v_max += 1.0;
    }

    std::cout << "vp umin, umax = " << viewport->u_min << ", " << viewport->u_max << std::endl;
    std::cout << "vp vmin, vmax = " << viewport->v_min << ", " << viewport->v_max << std::endl;


    std::vector<shared_ptr<DecodingBlock>> vp((i_max-i_min+1)*(j_max-j_min+1));

    int k = 0;

    int vp_rows = 1 + i_max - i_min;
    int vp_cols = 1 + j_max - j_min;

    for(int i = i_min ; i <= i_max ; i++)
    {
        for(int j = j_min ; j <= j_max ; j++)
        {
            auto b = getBlock(i, j);

            vp[k] = std::make_shared<DecodingBlock>(b->u, b->v, b->w, b->h);
            vp[k]->isAccessBlock = b->isAccessBlock;

            if(vp[k]->isAccessBlock)
            {
                viewport->access_block_index = k;
            }

            if(k > 0 && vp[k-1]->v == vp[k]->v)
            {
                vp[k]->neighbors[0] =  vp[k-1];
                vp[k-1]->neighbors[1] = vp[k];
            }

            if(i > i_min)
            {
                vp[k]->neighbors[2] = vp[k-vp_cols];
                vp[k-vp_cols]->neighbors[3] = vp[k];
            }

            k++;
        }
    }

    viewport->icus = vp;
    viewport->computeRows();
    viewport->computeOrdering();

    return viewport;
}

shared_ptr<Block> Picture::getBlock(int i, int j)
{
    while(i < 0) i += block_rows;
    while(j < 0) j += block_cols;

    i %= block_rows;
    j %= block_cols;

    return icus[i*block_cols + j];
}

void Picture::getBlockCoords(float u, float v, int &i, int &j)
{
    i = (int)(v * block_rows);
    j = (int)(u * block_cols);
}

float Picture::u(float theta)
{
    return theta / fov_x;
}

float Picture::v(float phi)
{
    return phi / fov_y;
}

bool Picture::contains(float u_min, float u_max, float v_min, float v_max, float u, float v)
{
     return (u >= u_min && u <= u_max && v >= v_min && v <= v_max)
             ||((u+1) >= u_min && (u+1) <= u_max && v >= v_min && v <= v_max)
             ||((u+1) >= u_min && (u+1) <= u_max && (v+1) >= v_min && (v+1) <= v_max)
             ||((u) >= u_min && (u) <= u_max && (v+1) >= v_min && (v+1) <= v_max);
}

bool Picture::contains(float u_min, float u_max, float v_min, float v_max, const Block& b)
{
//    std::cout <<  u_min << ", " <<     u_max << ", " << v_min << ", " << v_max << ", " << u << ", " << v;
    while(u_min < 0)
    {
        u_min += 1.0;
        u_max += 1.0;
    }

    while(v_min < 0)
    {
        v_min += 1.0;
        v_max += 1.0;
    }

    while(u_min > u_max)
    {
        u_max += 1.0;
    }

    while(v_min > v_max)
    {
        v_max += 1.0;
    }

    return     contains(u_min, u_max, v_min, v_max, b.u-0.5*b.w, b.v-0.5*b.h)
            || contains(u_min, u_max, v_min, v_max, b.u+0.5*b.w, b.v-0.5*b.h)
            || contains(u_min, u_max, v_min, v_max, b.u+0.5*b.w, b.v+0.5*b.h)
            || contains(u_min, u_max, v_min, v_max, b.u-0.5*b.w, b.v+0.5*b.h);
}

float Picture::phiMin()
{
    return center_y - 0.5 * fov_y;
}

float Picture::thetaMin()
{
    return center_x - 0.5 * fov_x;
}

float Picture::phiMax()
{
    return center_y + 0.5 * fov_y;

}

float Picture::thetaMax()
{
    return center_x + 0.5 * fov_x;
}

void Picture::initAccessBlocks(float pfov_x, float pfov_y)
{
    float step_x = pfov_x;
    float step_y = pfov_y;

    float min_x = 0;
    float min_y = 0;

    float max_x = pfov_x;
    float max_y = pfov_y;

    for(min_x = 0; min_x < fov_x ; min_x += step_x)
    {
        for(min_y = 0; min_y < fov_y ; min_y += step_y)
        {
            std::cout << min_x << ", "<< max_y << std::endl;
            max_x = min_x + pfov_x;
            max_y = min_y + pfov_y;

            std::cout << "****************************************\n" << std::endl;
            float pcenter_u = u(0.5 * (min_x + max_x));
            float pcenter_v = v(0.5 * (min_y + max_y));

            float u_min = u(min_x);
            float u_max = u(max_x);

            float v_min = v(min_y);
            float v_max = v(max_y);

            bool found = false;
            for(auto b: access_blocks)
            {
                if(contains(u_min, u_max, v_min, v_max, *(icus[b].get())))
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                int i = 0, j = 0;
                getBlockCoords(pcenter_u, pcenter_v, i, j);

                while(i < 0) i += block_rows;
                while(j < 0) j += block_cols;

                i %= block_rows;
                j %= block_cols;

                uint16_t acces_block_idx = i*block_cols + j;
                access_blocks.push_back(acces_block_idx);
                icus[acces_block_idx]->isAccessBlock = true;
            }
        }
    }
}
