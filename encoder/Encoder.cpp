#include "Encoder.h"

#include <boost/filesystem.hpp>
#include "omnidirectional_layer/BlockPrediction.h"
#include "rgb888_ycrcb.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "video_layer/mypicture.h"
#include "video_layer/quantizeFrame.h"

namespace fs = boost::filesystem;

Encoder::Encoder()
{
  std::cout << "new encoder" << std::endl;
}

Encoder::~Encoder()
{
  std::cout << "destroy encoder" << std::endl;
}

void Encoder::openFile(const std::string &filename)
{
    std::cout << "open file to encode " << filename << std::endl;
    fs::path filepath = fs::complete(fs::path(filename));
    ICOV_ASSERT_MSG(fs::is_regular_file(filepath), "File not found: " + filepath.string());
    m_img_src = cv::imread(filename, -1);
}

void Encoder::encode(std::ostream &output, const IPicture &picture, const IAccessBlockManager &access_blocks)
{
    picture.getBlock(0,0)->w();

     ICOV_ASSERT_MSG(m_img_src.total() > 0, "Cannot encode empty image");
     int values[] = {m_img_src.rows, m_img_src.cols, m_img_src.type(), m_img_src.channels()};
     output.write(reinterpret_cast<const char*>(&values), sizeof(values));

     //TODO manage resize

     ChromaFormat chromaFormatIDC = CHROMA_420;
     int sourceWidth = m_img_src.cols;
     int sourceHeight = m_img_src.rows;
     const int margin_x_360luma = 2; // in fact we need 1 margin for each color component, but since UnitArea handles division by 2 for chroma 420, it is easier to add 2 pixel margins per left and right side and this will be divided by 2 for chroma.

     UnitArea extendedFrameArea(chromaFormatIDC, Area(0, 0, sourceWidth + 2*margin_x_360luma, sourceHeight));
     UnitArea mainFrameArea(chromaFormatIDC, Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

     PelStorage picTemp; picTemp.create(extendedFrameArea);

     MyPicture encoderPicture;
     encoderPicture.create(PIC_ORIGINAL, extendedFrameArea);
     encoderPicture.create(PIC_RECONSTRUCTION, extendedFrameArea);
     encoderPicture.create(PIC_PREDICTION, extendedFrameArea);

     PelUnitBuf mainFrameOrig = encoderPicture.getOrigBuf(mainFrameArea);
     cvMatToYUV(m_img_src, mainFrameOrig);
     EncodingStructure enc_struc;

     const std::vector<int> qp_set = {22, 27, 32, 37, 42};

     Int internalBitDepth[MAX_NUM_CHANNEL_TYPE];
     internalBitDepth[CHANNEL_TYPE_LUMA] = 8;   // Bit-depth the codec operates at (input/output files will be converted). (default: MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data will be converted
     internalBitDepth[CHANNEL_TYPE_CHROMA] = 8;   // Bit-depth the codec operates at (input/output files will be converted). (default: MSBExtendedBitDepth). If different to MSBExtendedBitDepth, source data will be converted
     const int maxLog2TrDynamicRange = 15;

     for(int qp: qp_set)
     {
         quantizeFrame(qp, encoderPicture.getOrigBuf(), encoderPicture.getRecoBuf(), enc_struc, internalBitDepth, maxLog2TrDynamicRange);  // give the whole extended image ( .getOrigBuf() and .getRecoBuf() ) to the function because vector icues_enc is based on extended frames


     }

     //encode the blocks
     for(const auto& b: picture.icus())
     {
         encodeBlock(output, b, access_blocks.isAccessBlock(b.getId(Block::THIS)));
     }
}

void Encoder::encodeBlock(std::ostream &output, const Block &b, bool access_block)
{
    std::cout << "-- block " << b.center_u() << ", " << b.center_v() << std::endl;

    //if(access_block) encodeBlockWithPrediction(output, b, BlockPrediction::NONE);

    std::vector<BlockPrediction::predType> pred_values = {
        BlockPrediction::RIGHT,
        BlockPrediction::LEFT,
        BlockPrediction::TOP,
        BlockPrediction::BOTTOM,
        BlockPrediction::TOP_RIGHT,
        BlockPrediction::TOP_LEFT,
        BlockPrediction::BOTTOM_RIGHT,
        BlockPrediction::BOTTOM_LEFT,
    };

    for(auto pred: pred_values)
    {
        encodeBlockWithPrediction(output, b, static_cast<uint16_t>(pred));
    }
}

void Encoder::encodeBlockWithPrediction(std::ostream &output, const Block &b, uint16_t prediction)
{
    std::cout << "---- prediction " << prediction << std::endl;

    const IPicture &p = *(b.parent());
    int bw = (int)(b.w() * p.width());
    int bh = (int)(b.h() * p.height());
    int bx = (int)(p.u(p.angleX(b.u())) * p.width());
    int by = (int)(p.v(p.angleY(b.v())) * p.height());
    auto rect_block = cv::Rect(bx, by, bw, bh);


}
