#include "InteractiveModeBase.h"

#include "gl_viewer/single360degreecamerahandler.h"
#include "icov_log.h"

#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/qvm/all.hpp>

#include <chrono>
#include <fstream>
#include <iomanip> // std::setprecision
#include <iostream>
#include <memory>
#include <stdio.h>
#include <time.h> /* time */

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
    : m_360viewer(nullptr), fovX_degree(0.), block_size(0), pov_x(0.),
      pov_y(0.), m_special_key(0), m_wait_delay(0) {
  // default
  init360Viewer(1920, 1080, 35);
}

InteractiveModeBase::~InteractiveModeBase()
{
    
}

void InteractiveModeBase::run() {
  m_special_key = m_config.f1;
  cv::namedWindow(const_window_name, cv::WINDOW_NORMAL);

  m_erpImage = cv::Mat(128, 256, CV_8UC3, cv::Scalar(255, 0, 255));
  m_viewportImage = cv::Mat(128, 256, CV_8UC3, cv::Scalar(255, 0, 255));

  showErp();

  char key = '\0';
  reset();
  while (processKey(key)) {
    draw();
    cv::imshow(const_window_name, m_displayImage);
    key = cv::waitKey(m_wait_delay);
    if (key != -1)
      ICOV_DEBUG << "pressed key code = " << (int)key;
  }
}

float InteractiveModeBase::getPov_x_DEG() const { return pov_x; }

float InteractiveModeBase::getPov_y_DEG() const { return pov_y; }

void InteractiveModeBase::setPOV_DEG(float pov_x, float pov_y) {
  move(pov_y - this->pov_y, pov_x - this->pov_x);
}

bool InteractiveModeBase::is_special_key(char key) const
{
    return key == m_config.f1 || key == m_config.f2 || key == m_config.f3 || key == m_config.f4 || key == m_config.f5 ||
           key == m_config.f6 || key == m_config.f7 || key == m_config.f8;
}

void InteractiveModeBase::draw()
{
    this->fill(bgrColor::BLACK());

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

    if (m_showViewport)
        showViewport();
    else
        showErp();
}

void InteractiveModeBase::init360Viewer(int width, int height,
                                        float fov_y_DEG) {
    ICOV_DEBUG << "Init viewer " << width << "x" << height
               << " with vertical FOV of " << fov_y_DEG;
    m_360viewer = std::make_unique<Single360degreeCameraHandler>(
        cv::Size(width, height), fov_y_DEG, false);
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

    else if (key == m_config.view_switch)
        switchView();

    else if (is_special_key(key) && key != m_special_key) {
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

    m_360viewer->pitch(pov_y - previous_pov_y);
    m_360viewer->yaw(previous_pov_x - pov_x);
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
    m_360viewer->resetOrientation();
}

void InteractiveModeBase::clear()
{
    fill(bgrColor::BLACK());
}

void InteractiveModeBase::fill(const cv::Scalar &color)
{
    m_erpImage.setTo(color);
}

const InteractiveModeBase::Config &InteractiveModeBase::config() const
{
    return m_config;
}

void InteractiveModeBase::setWait_delay(int newWait_delay)
{
    m_wait_delay = newWait_delay;
}

const std::unique_ptr<Single360degreeCameraHandler> &
InteractiveModeBase::get360viewer() const {
    return m_360viewer;
}

void InteractiveModeBase::switchView() {
    m_showViewport ? showErp() : showViewport();
}

void InteractiveModeBase::showErp() {
    m_showViewport = false;
    m_displayImage = m_erpImage;
}

void InteractiveModeBase::showViewport() {
    m_360viewer->loadEquirectangularImage(m_erpImage);
    m_360viewer->getImage_BGR(m_viewportImage);

    m_showViewport = true;
    m_displayImage = m_viewportImage;
}
