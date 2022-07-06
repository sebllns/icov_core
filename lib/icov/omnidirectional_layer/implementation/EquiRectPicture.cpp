#include "EquiRectPicture.h"
#include <cmath>

//*/phi = y
//theta = x
EquiRectPicture::EquiRectPicture(int pw, int ph):
    IPicture(pw, ph, 0, M_PI/2, 2 * M_PI, M_PI)
{ }

void EquiRectPicture::buildConstantSizeBlocks(int pw, int ph)
{
    int w = m_width;
    int h = m_height;

    block_cols = (int)std::ceil((double)w/pw);
    block_rows = (int)std::ceil((double)h/ph);

    m_total_w = block_cols * pw;
    m_total_h = block_rows * ph;

    float bw = 1.0/block_cols;
    float bh = 1.0/block_rows;

    //m_icus.resize(block_cols * block_rows);
    m_icus.reserve(block_cols * block_rows);

    for(int i = 0; i < block_rows ; i++)
    {
        for(int j = 0; j < block_cols ; j++)
        {
            auto id = getId(i,j);
            Block b(id, this, j*bw, i*bh, bw, bh);
            b.setId(Block::LEFT, getId(i, j-1));
            b.setId(Block::RIGHT, getId(i, j+1));
            b.setId(Block::UP, getId(i-1, j));
            b.setId(Block::DOWN, getId(i+1, j));

            m_icus.push_back(b);
        }
    }
}

size_t EquiRectPicture::getId(int i, int j) const
{
    if(i < 0 || i >= block_rows) return 0;

    while(j < 0 ) j += block_cols;
    j %= block_cols;

    return i*block_cols + j + 1;
}

float EquiRectPicture::u(float angle_x) const
{
    assert(angle_x >= minAngleX() && angle_x <= maxAngleX());
     return (angle_x - minAngleX()) / m_fov_x;
}

float EquiRectPicture::v(float angle_y) const
{
    assert(angle_y >= minAngleY() && angle_y <= maxAngleY());
    return (angle_y - minAngleY())/ m_fov_y;
}


float EquiRectPicture::angleX(float u) const
{
    return u * m_fov_x + minAngleX();
}

float EquiRectPicture::angleY(float v) const
{
     return v * m_fov_y + minAngleY();
}
