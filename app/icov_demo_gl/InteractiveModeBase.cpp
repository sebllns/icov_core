#include "InteractiveModeBase.h"
#include "encoder/FakeEncoder.h"
#include "encoder/InputMedia.h"

#include "gl_viewer/single360degreecamerahandler.h"
#include "icov_log.h"
#include "omnidirectional_layer/RequestManager.h"
#include "omnidirectional_layer/icov_geometry.h"
#include "omnidirectional_layer/implementation/BlockOrdering.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "omnidirectional_layer/implementation/ViewportManagerCenterPadded.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "opencv2/core/types.hpp"

#include <boost/archive/xml_iarchive.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/qvm/all.hpp>

#include <chrono>
#include <fstream>
#include <iomanip> // std::setprecision
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <time.h> /* time */

using namespace icov::omnidirectional_layer;

enum KeyName
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    QUIT,
};

InteractiveModeBase::InteractiveModeBase()
{
}

InteractiveModeBase::~InteractiveModeBase()
{
    
}

void InteractiveModeBase::run()
{
    pov_x = 0;
    pov_y = 0;
    m_special_key = m_config.f1;

    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    m_outputImage = cv::Mat(128, 256, CV_8UC3, cv::Scalar(255, 0, 255));

    viewer = std::make_unique<Single360degreeCameraHandler>(cv::Size(1920, 1080), 30, false);

    // viewport parameters
    //    cv::Size outputResolution(1920, 1080);

    //    double fovY_radian = 2 * atan(tan(glm::radians(fovX_degree) * 0.5) *
    //                                  ((double)outputResolution.height / ((double)outputResolution.width)));
    //    double fovY_degree = glm::degrees(fovY_radian);
    //    std::cout << "fov_x= " << fovX_degree << ", fov_y= " << fovY_degree << std::endl;

    //    cv::Mat viewportEquirectangularOriginal, maskRequested, blocksOverlay;

    //    Single360degreeCameraHandler viewer(outputResolution, fovY_degree, true);

    //    cv::Mat equirectangularImage = cv::imread(fileAddr, -1);

    //    int pixel_count = equirectangularImage.rows * equirectangularImage.cols;

    //    if (pixel_count <= 0)
    //    {
    //        std::cerr << "No pixels loaded. Exited." << std::endl;
    //        std::exit(1);
    //    }

    //    std::cout << "equirectangular pixels= " << pixel_count << std::endl;

    //    viewer.loadEquirectangularImage(equirectangularImage);

    char key = '\0';

    //    cv::namedWindow("viewport", cv::WINDOW_NORMAL);
    //    cv::namedWindow("requested blocks", cv::WINDOW_NORMAL);
    //    //  cv::namedWindow("mask", cv::WINDOW_NORMAL);

    //    viewer.resetOrientation();

    //    // float pov_x = 180, pov_y = 90;
    //    float pov_x = 0, pov_y = 0;

    //    // Define the viewport static geometry
    //    viewport_geometry vpg;
    //    vpg.focale = 1 / (2.0f * tan(glm::radians(fovX_degree) * 0.5)); // focale
    //    vpg.center = point2d(0.5f, 0.5f);                               // center normalised coords
    //    vpg.fov = angle_radian(fovX_degree * M_PI / 180.0,
    //                           fovY_radian); // field of view in radians
    //    vpg.resolution = point2d(outputResolution.width,
    //                             outputResolution.height); // resolution in pixels
    //    point2d padding(0, 0);

    //    // viewport geometry in cartesian coords
    //    point2d viewport_size(abs(2 * tan(fovX_degree * M_PI_2 / 180.0)), abs(2 * tan(fovY_degree * M_PI_2 / 180.0)));

    //    vpg.roi_size = viewport_size;

    //    RequestManager req_mgr;
    //    auto picture = std::make_shared<EquiRectPicture>(equirectangularImage.cols, equirectangularImage.rows);
    //    picture->buildConstantSizeBlocks(block_size, block_size);
    //    req_mgr.setPicture(picture);
    //    req_mgr.setAccessBlocks(std::make_shared<IcovAccessBlockManager>());
    //    req_mgr.setOrdering(std::make_shared<BlockOrdering>(picture->icus().size()));
    //    req_mgr.setViewport(std::make_shared<ViewportManagerCenterPadded>(1.2f));
    //    req_mgr.setBlockPrediction(std::make_shared<BlockPrediction>());
    //    req_mgr.viewport()->viewportGeometry() = vpg;

    //    boost::filesystem::path encoded_file_path("./encoded.icov");

    //    std::ofstream encoded_file(encoded_file_path.string(), std::ios::out | std::ios::binary);
    //    icov::encoder::FakeEncoder encoder(encoded_file);
    //    icov::encoder::InputImage input(fileAddr);
    //    BlockPrediction predMgr;

    //    encoder.encode(input, *picture, predMgr);
    //    encoded_file.close();

    //    req_mgr.start();

    while (processKey(key))
    {
        //        // user point of view, defining the viewport angle
        //        vpg.pov = angle_radian(pov_x * M_PI / 180.0, pov_y * M_PI / 180.0);

        //        // create images

        //        viewer.loadEquirectangularImage(equirectangularImage);
        //        viewer.getImage_BGR(viewportEquirectangularOriginal, maskRequested);
        //        float lowlight = 0.2;
        //        float hightlight = 0.5;
        //        blockRequested = lowlight * equirectangularImage;
        //        cv::add(blockRequested, (hightlight - lowlight) * equirectangularImage, blockRequested,
        //        maskRequested);

        //        blockprocessor->Pov = angle_degree(pov_x, pov_y);
        //        req_mgr.update(pov_x * M_PI / 180.0, pov_y * M_PI / 180.0);

        //        // add overlay
        //        viewer.loadEquirectangularImage(blockRequested);
        //        viewer.getImage_BGR(blocksOverlay, maskRequested);

        //        // SHOW IMAGES
        //        cv::imshow("requested blocks", blockRequested);
        //        cv::imshow("viewport", blocksOverlay);
        //        //    cv::imshow("mask", maskRequested);

        cv::imshow(window_name, m_outputImage);

        key = cv::waitKey(0);

        ICOV_DEBUG << "pressed key code = " << (int)key;
    }
}

float InteractiveModeBase::getPov_x() const
{
    return pov_x;
}

float InteractiveModeBase::getPov_y() const
{
    return pov_y;
}

bool InteractiveModeBase::is_special_key(char key) const
{
    return key == m_config.f1 || key == m_config.f2 || key == m_config.f3 || key == m_config.f4 || key == m_config.f5 ||
           key == m_config.f6 || key == m_config.f7 || key == m_config.f8;
}

void InteractiveModeBase::draw()
{
    if (this->special_key() == config().f1)
        updateF1();

    else if (this->special_key() == config().f2)
        updateF2();

    else if (this->special_key() == config().f3)
        updateF3();

    else if (this->special_key() == config().f4)
        updateF4();

    else if (this->special_key() == config().f5)
        updateF5();

    else if (this->special_key() == config().f6)
        updateF6();

    else if (this->special_key() == config().f7)
        updateF7();

    else if (this->special_key() == config().f8)
        updateF8();
}

bool InteractiveModeBase::processKey(char key)
{
    if (key == m_config.quit)
        return false;

    if (key == m_config.reset)
        reset();

    else if (key == m_config.left)
        moveLeft();

    else if (key == m_config.right)
        moveRight();

    else if (key == m_config.up)
        moveUp();

    else if (key == m_config.down)
        moveDown();

    if (is_special_key(key) && key != m_special_key)
    {
        int previous = m_special_key;
        m_special_key = key;
        onSpecialKey(previous, key);
    }

    return true;
}

void InteractiveModeBase::onSpecialKey(int previous, int current)
{
    m_special_key = current;

    if (current == m_config.f1)
        onF1();

    else if (current == m_config.f2)
        onF2();

    else if (current == m_config.f3)
        onF3();

    else if (current == m_config.f4)
        onF4();

    else if (current == m_config.f5)
        onF5();

    else if (current == m_config.f6)
        onF6();

    else if (current == m_config.f7)
        onF7();

    else if (current == m_config.f8)
        onF8();
}

void InteractiveModeBase::onF1()
{
}

void InteractiveModeBase::onF2()
{
}

void InteractiveModeBase::onF3()
{
}

void InteractiveModeBase::onF4()
{
}

void InteractiveModeBase::onF5()
{
}

void InteractiveModeBase::onF6()
{
}

void InteractiveModeBase::onF7()
{
}

void InteractiveModeBase::onF8()
{
}

void InteractiveModeBase::updateF1()
{
    
}

void InteractiveModeBase::updateF2()
{
}

void InteractiveModeBase::updateF3()
{
}

void InteractiveModeBase::updateF4()
{
}

void InteractiveModeBase::updateF5()
{
}

void InteractiveModeBase::updateF6()
{
}

void InteractiveModeBase::updateF7()
{
}

void InteractiveModeBase::updateF8()
{
}

void InteractiveModeBase::move(float offset_pitch, float offset_yaw)
{
    float previous_pov_x = pov_x;
    float previous_pov_y = pov_y;

    pov_x += offset_yaw;
    pov_y += offset_pitch;

    if (pov_x > 180.0f)
        pov_x -= 360.0f;
    if (pov_x < -180.0f)
        pov_x += 360.0f;

    if (pov_y > 90.0f)
        pov_y = 90.0f;
    if (pov_y < -90.0f)
        pov_y = -90.0f;

    viewer->pitch(pov_y - previous_pov_y);
    viewer->yaw(previous_pov_x - pov_x);
}

void InteractiveModeBase::moveLeft()
{
    move(0, -m_config.offset_yaw);
}

void InteractiveModeBase::moveRight()
{
    move(0, m_config.offset_yaw);
}

void InteractiveModeBase::moveUp()
{
    move(m_config.offset_pitch, 0);
}

void InteractiveModeBase::moveDown()
{
    move(-m_config.offset_pitch, 0);
}

void InteractiveModeBase::reset()
{
    pov_x = m_config.start_yaw;
    pov_y = m_config.start_pitch;
    viewer->resetOrientation();
}

void InteractiveModeBase::clear()
{
    fill(bgrColor::BLACK());
}

void InteractiveModeBase::fill(const cv::Scalar &color)
{
    m_outputImage.setTo(color);
}

const InteractiveModeBase::Config &InteractiveModeBase::config() const
{
    return m_config;
}
