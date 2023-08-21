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

    m_special_key = m_config.f1;

    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    m_outputImage = cv::Mat(128, 256, CV_8UC3, cv::Scalar(255, 0, 255));

    viewer = std::make_unique<Single360degreeCameraHandler>(cv::Size(1920, 1080), 30, false);

    char key = '\0';
    reset();
    while (processKey(key))
    {
        cv::imshow(window_name, m_outputImage);
        key = cv::waitKey(1000);
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
