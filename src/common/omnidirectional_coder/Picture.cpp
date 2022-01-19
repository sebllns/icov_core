#include "Picture.h"

Picture::Picture(int pw, int ph):
    w(pw),
    h(ph)
{
    center_x = 0;
    center_y = 0;
    fov_x = 2 * M_PI;
    fov_y = M_PI;
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
            icus[i*block_cols + j] = std::make_shared<Block>(j*bw, 1 - i*bh, bw, bh);
        }
    }
}

shared_ptr<Viewport> Picture::getViewport(float pcenter_x, float pcenter_y, float pfov_x, float pfov_y)
{
    auto viewport = std::make_shared<Viewport>(pcenter_x, pcenter_y, pfov_x, pfov_y);

    float u_min = u(pcenter_x - 0.5 * pfov_x);
    float u_max = u(pcenter_x + 0.5 * pfov_x);

    float v_min = v(pcenter_y - 0.5 * pfov_y);
    float v_max = v(pcenter_y + 0.5 * pfov_y);

    int i_min, i_max, j_min, j_max = 0;

    getBlockCoords(u_min, v_min, i_min, j_max);
    getBlockCoords(u_max, v_max, i_max, j_min);

    BlockVector vp((i_max-i_min+1)*(j_max-j_min+1));

    int k = 0;
    for(int i = i_min ; i <= i_max ; i++)
    {
        for(int j = j_min ; j <= j_max ; j++)
        {
            vp[k++] = getBlock(i, j);
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
    i = (int)(u * block_cols);
    j = (int)((1-v) * block_rows);
}

float Picture::u(float theta)
{
    return theta / fov_x;
}

float Picture::v(float phi)
{
    return phi / fov_y;
}
