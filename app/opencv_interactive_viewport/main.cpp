/**************************************************************************
**   Copyright (C): 2021
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/

#include <iostream>
#include <time.h> /* time */
#include <opencv2/opencv.hpp>
#include <chrono>
#include <iomanip> // std::setprecision
#include <fstream>
#include <stdio.h>

#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/BlockExtractor.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "omnidirectional_layer/implementation/BlockOrdering.h"
#include "omnidirectional_layer/implementation/ViewportManagerCenterPadded.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"

#include "omnidirectional_layer/geometry.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "omnidirectional_layer/RequestManager.h"
#include "encoder/FakeEncoder.h"

#include "single360degreecamerahandler.h"

#include <boost/filesystem.hpp>
#include <boost/qvm/all.hpp>
#include <boost/exception/diagnostic_information.hpp>



class BlockProcessor : public IBlockProcessor
{

private:
    cv::Point m_line_point;
    uint16_t m_blockCount;
    uint16_t m_single_si_count;
    uint16_t m_double_si_count;
    std::ofstream m_csv_file;
    std::ifstream m_encoded_image;
    int header[4];

public:
    angle_degree Pov;
    cv::Mat BlockRequested;
    bool b_show_side_infos;
    bool b_show_access_block;
    bool b_show_block_grid;
    bool b_show_block_number;
    bool b_show_path;
    bool b_use_encoded_stream;



    BlockProcessor(const std::string &encoded_image);
    ~BlockProcessor();

    // IBlockProcessor interface
    void processBlock(const Block &b, uint16_t prediction) override;
    void endProcess() override;
    void startProcess() override;
    std::ostream encodingStream();
};

void BlockProcessor::startProcess()
{
    m_blockCount = 0;
    m_single_si_count = 0;
    m_double_si_count = 0;
}

void BlockProcessor::endProcess()
{
    m_csv_file << Pov.x() << "," << Pov.y() << "," << m_blockCount << "," << m_single_si_count << "," << m_double_si_count << std::endl;
    m_csv_file.flush();
}

BlockProcessor::BlockProcessor(const std::string &encoded_image):
    m_encoded_image(encoded_image, std::ios::in | std::ios::binary)
{
    m_csv_file.open("./viewport_report.csv");
    m_csv_file << "pov_x,pov_y,blockCount,single_si_count,double_si_count\n";
    b_show_access_block = true;
    b_show_block_grid = true;
    b_show_block_number = true;
    b_show_side_infos = true;
    b_show_path = true;
    b_use_encoded_stream = true;


    m_encoded_image.read((char*)&header[0], sizeof (header));

    std::cout << header[0] << "x" << header[1] << "/" << header[2] << ", " << header[3]<< std::endl;
}

BlockProcessor::~BlockProcessor()
{
    m_csv_file.close();
}

//draw the requested blocks
void BlockProcessor::processBlock(const Block &b, uint16_t prediction)
{
    const IPicture &p = *(b.parent());
    int bw = (int)(b.w() * p.width());
    int bh = (int)(b.h() * p.height());
    int bx = (int)(p.u(p.angleX(b.u())) * p.width());
    int by = (int)(p.v(p.angleY(b.v())) * p.height());
    auto rect_block = cv::Rect(bx, by, bw, bh);


    if(b_use_encoded_stream)
    {
        cv::Mat blockRegion(bh, bw, BlockRequested.type());

        int si_offset = 0;


        switch (static_cast<BlockPrediction::predType>(prediction))
        {
        case BlockPrediction::NONE:
            si_offset += 0;
            break;
        case BlockPrediction::RIGHT:
            si_offset += 0;
            break;
        case BlockPrediction::LEFT:
            si_offset += 1;
            break;
        case BlockPrediction::TOP:
            si_offset += 2;
            break;
        case BlockPrediction::BOTTOM:
            si_offset += 3;
            break;
        case BlockPrediction::TOP_RIGHT:
            si_offset += 4;
            break;
        case BlockPrediction::TOP_LEFT:
            si_offset += 5;
            break;
        case BlockPrediction::BOTTOM_RIGHT:
            si_offset += 6;
            break;
        case BlockPrediction::BOTTOM_LEFT:
            si_offset += 7;
            break;

        }

        m_encoded_image.seekg(sizeof (header) + ((b.getId(Block::THIS) - 1) * 8 + si_offset) * bw * bh * blockRegion.elemSize());
        m_encoded_image.read((char *)blockRegion.data,  bw * bh * blockRegion.elemSize());

        blockRegion.copyTo(BlockRequested(rect_block));
    }

    if(b_show_block_grid) cv::rectangle(BlockRequested, rect_block,  cv::Scalar(0, 0, 255), 10);

    //access block
    if(m_blockCount == 0 && b_show_access_block) BlockRequested(rect_block).setTo(cv::Scalar(0, 255, 0));

    if(b_show_block_number) cv::putText(BlockRequested, std::to_string(m_blockCount), cv::Point(bx, by + 0.5 * bh), cv::FONT_HERSHEY_PLAIN, 5.0, cv::Scalar(0, 0, 255), 3);

    cv::Point linePoint2(p.i(b.center_u()), p.j(b.center_v()));

    if(m_blockCount > 0 && b_show_path) cv::arrowedLine(BlockRequested, m_line_point, linePoint2, cv::Scalar(255, 255, 255), 4, 8, 0, 0.05);

    m_line_point = linePoint2;

    if(b_show_side_infos)
    {
        int predThickness = 10;
        cv::Scalar line_color;

        switch (static_cast<BlockPrediction::predType>(prediction))
        {
        case BlockPrediction::NONE:
            ICOV_ASSERT_MSG(m_blockCount==0, "Only an access block can have 0 side info");
            break;
            //magenta for horizontal prediction only
        case BlockPrediction::RIGHT:
        case BlockPrediction::LEFT:
        {
            m_single_si_count++;
            line_color = cv::Scalar(255,0,255);
        }
            break;

            //yellow for vertical prediction i.e. worst prediction
        case BlockPrediction::TOP:
        case BlockPrediction::BOTTOM:
        {
            m_single_si_count++;
            line_color = cv::Scalar(0,255,255);
        }
            break;


            //blue for best prediction
        case BlockPrediction::TOP_RIGHT:
        case BlockPrediction::TOP_LEFT:
        case BlockPrediction::BOTTOM_RIGHT:
        case BlockPrediction::BOTTOM_LEFT:
        {
            m_double_si_count++;
            line_color = cv::Scalar(150,0,0);
        }
            break;


        }

        //LEFT
        if(prediction & BlockPrediction::LEFT)
        {
            BlockRequested(cv::Rect(bx, by, predThickness, bh)).setTo(line_color);
        }

        //RIGHT
        if(prediction & BlockPrediction::RIGHT)
        {
            BlockRequested(cv::Rect(bx + bw - predThickness, by, predThickness, bh)).setTo(line_color);
        }

        //TOP
        if(prediction & BlockPrediction::TOP)
        {
            BlockRequested(cv::Rect(bx, by, bw, predThickness)).setTo(line_color);
        }

        //BOTTOM
        if(prediction & BlockPrediction::BOTTOM)
        {
            BlockRequested(cv::Rect(bx, by + bh - predThickness, bw, predThickness)).setTo(line_color);
        }
    }

    m_blockCount++;
}

void run()
{
    icov::helpers::rand::init();
    std::string fileAddr = "./P29_8888x4444.jpg";

    // viewport parameters
    cv::Size outputResolution(1920, 1080);
    double fovX_degree = 110.f;

    double fovY_radian = 2 * atan(tan(glm::radians(fovX_degree) * 0.5) * ((double)outputResolution.height / ((double)outputResolution.width)));
    double fovY_degree = glm::degrees(fovY_radian);
    std::cout << "fov_x= " << fovX_degree << ", fov_y= " << fovY_degree << std::endl;

    cv::Mat viewportEquirectangularOriginal, maskRequested, blocksOverlay;

    Single360degreeCameraHandler viewer(outputResolution, fovY_degree, true);

    cv::Mat equirectangularImage = cv::imread(fileAddr, -1);

    int pixel_count = equirectangularImage.rows * equirectangularImage.cols;

    if (pixel_count <= 0)
    {
        std::cerr << "No pixels loaded. Exited." << std::endl;
        std::exit(1);
    }

    std::cout << "equirectangular pixels= " << pixel_count << std::endl;

    viewer.loadEquirectangularImage(equirectangularImage);

    char key = '\0';

    float offset_pitch = 10.;
    float offset_yaw = 12.;

    cv::namedWindow("viewport", cv::WINDOW_NORMAL);
    cv::namedWindow("requested blocks", cv::WINDOW_NORMAL);

    viewer.resetOrientation();

    int block_size = 202;

    //float pov_x = 180, pov_y = 90;
    float pov_x = 0, pov_y = 0;

    //Define the viewport static geometry
    viewport_geometry vpg;
    vpg.focale = 1 / (2.0f * tan(glm::radians(fovX_degree) * 0.5)); // focale
    vpg.center = point2d(0.5f, 0.5f); //center normalised coords
    vpg.fov = angle_radian(fovX_degree* M_PI/180.0, fovY_radian); //field of view in radians
    vpg.resolution = point2d(outputResolution.width, outputResolution.height); //resolution in pixels
    point2d padding(0, 0);



    //viewport geometry in cartesian coords
    point2d viewport_size(abs(2*tan(fovX_degree*M_PI_2/180.0)), abs(2*tan(fovY_degree*M_PI_2/180.0)));

    vpg.roi_size = viewport_size;


    RequestManager req_mgr;
    auto picture = std::make_shared<EquiRectPicture>(equirectangularImage.cols, equirectangularImage.rows);
    picture->buildConstantSizeBlocks(block_size, block_size);
    req_mgr.setPicture(picture);
    req_mgr.setAccessBlocks(std::make_shared<IcovAccessBlockManager>());
    req_mgr.setOrdering(std::make_shared<BlockOrdering>(picture->icus().size()));
    req_mgr.setViewport(std::make_shared<ViewportManagerCenterPadded>(1.2f));
    req_mgr.setBlockPrediction(std::make_shared<BlockPrediction>());

    req_mgr.viewport()->viewportGeometry() = vpg;
    req_mgr.start();

    boost::filesystem::path encoded_file_path("./encoded.icov");
    FakeEncoder encoder;
    encoder.openFile(fileAddr);


    std::ofstream encoded_file(encoded_file_path.string(), std::ios::out | std::ios::binary);
    encoder.encode(encoded_file, *picture, *req_mgr.accessBlocks());
    encoded_file.close();

    auto blockprocessor = std::make_shared<BlockProcessor>(encoded_file_path.string());
    req_mgr.setBlockProcessor(blockprocessor);

    cv::Mat &blockRequested = blockprocessor->BlockRequested;
    blockRequested.create(equirectangularImage.size(), equirectangularImage.type());

    while (key != 27)
    {
        blockRequested.setTo(0);
        //        equiRequested.setTo(0);

        //user point of view, defining the viewport angle
        vpg.pov = angle_radian(pov_x*M_PI/180.0, pov_y*M_PI/180.0);


        //create images

        viewer.loadEquirectangularImage(equirectangularImage);
        viewer.getImage_BGR(viewportEquirectangularOriginal, maskRequested);
        float lowlight = 0.2;
        float hightlight = 0.5;
        blockRequested = lowlight * equirectangularImage;
        cv::add(blockRequested, (hightlight - lowlight) * equirectangularImage, blockRequested, maskRequested);

        blockprocessor->Pov = angle_degree(pov_x, pov_y);
        req_mgr.update(pov_x*M_PI/180.0, pov_y*M_PI/180.0);

        if(blockprocessor->b_show_access_block)
        {
            for(const auto &b: req_mgr.accessBlocks()->blocks())
            {
                int bw = (int)(b->w() * picture->width());
                int bh = (int)(b->h() * picture->height());
                int bx = (int)(picture->u(picture->angleX(b->u())) * picture->width());
                int by = (int)(picture->v(picture->angleY(b->v())) * picture->height());
                auto rect_block = cv::Rect(bx, by, bw, bh);
                cv::rectangle(blockRequested, rect_block,  cv::Scalar(0, 255, 0), 4);
            }
        }


        //add overlay
        viewer.loadEquirectangularImage(blockRequested);
        viewer.getImage_BGR(blocksOverlay, maskRequested);

        //SHOW IMAGES
        cv::imshow("requested blocks", blockRequested);
        cv::imshow("viewport", blocksOverlay);


        //viewport mesh method

        //        point3d pt_viewport(1, -0.5*viewport_size.x(), -0.5*viewport_size.y());

        //        bg::model::d3::point_xyz<coord_precision, bg::cs::spherical<bg::radian> > angle;


        //        point2d delta(viewport_size.x()/100, viewport_size.y()/100);

        //        while (pt_viewport.z() < viewport_size.y()/2)
        //        {
        //            pt_viewport.y( -0.5*viewport_size.x());
        //            while (pt_viewport.y() < viewport_size.x()/2)
        //            {
        //                bg::transform(rotate_YPR(pt_viewport, vpg.pov.x(), vpg.pov.y(), 0), angle);



        //                int jj = (int)(p.u(angle.x()) * p.width());
        //                int ii = (int)(p.v(angle.y()) * p.height());

        //                cv::circle(blockRequested, cv::Point(jj, ii), 4, cv::Scalar(255,255,0), 6);

        //                pt_viewport.y(pt_viewport.y() + delta.x());
        //            }
        //            pt_viewport.z(pt_viewport.z() + delta.y());
        //        }


        key = cv::waitKey(0);

        std::cout << "pressed key code = " << (int)key << std::endl;

        float previous_pov_x = pov_x;
        float previous_pov_y = pov_y;

        switch(key)
        {
        //UP
        case 'z':
        case 'R':
            pov_y += offset_pitch;
            break;

            //DOWN
        case 's':
        case 'T':
            pov_y -= offset_pitch;
            break;

            //LEFT
        case 'q':
        case 'Q':
            pov_x -= offset_yaw;
            break;

            //RIGHT
        case 'd':
        case 'S':
            pov_x += offset_yaw;
            break;

        case -66:
            blockprocessor->b_show_path = !blockprocessor->b_show_path;
            break;

        case -65:
            blockprocessor->b_show_block_number = !blockprocessor->b_show_block_number;
            break;

        case -64:
            blockprocessor->b_show_access_block = !blockprocessor->b_show_access_block;
            break;

        case -63:
            blockprocessor->b_show_side_infos = !blockprocessor->b_show_side_infos;
            break;

        case -62:
            blockprocessor->b_show_block_grid = !blockprocessor->b_show_block_grid;
            break;

            //RESET
        case 'r':
        case 36:
            pov_x = 0;
            pov_y = 0;
            previous_pov_x = 0;
            previous_pov_y = 0;
            viewer.resetOrientation();
            break;
        }

        if(pov_x > 180.0f) pov_x -= 360.0f;
        if(pov_x < -180.0f) pov_x += 360.0f;

        if(pov_y > 90.0f) pov_y = 90.0f;
        if(pov_y < -90.0f) pov_y = -90.0f;

        viewer.pitch(pov_y - previous_pov_y);
        viewer.yaw(previous_pov_x - pov_x);
    }
}

int main(int argc, char *argv[])
{
    try
    {
        icov::DebugTools::debug_stream.rdbuf(std::cout.rdbuf());
        run();
    }
    catch(std::exception& e)
    {
        std::cerr << "\nError: program execution has stopped prematurely" << std::endl;
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "\nFatal error" << std::endl;
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return 2;
    }

    return 0;
}
