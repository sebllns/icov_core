#include "Block.h"

Block::Block(float px, float py, float pw, float ph):
    u(px),
    v(py),
    w(pw),
    h(ph)
{
    isAccessBlock = false;
}
