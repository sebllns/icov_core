/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/

#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

#include "single360degreecamerahandler.h"
#include "openglhandler.h"
#include <iostream>

const float zNear = 0.1f;
const float zFar = 10.0f;

Single360degreeCameraHandler::Single360degreeCameraHandler(
    const cv::Size &resolution, float fovY, bool computeMask)
    : _computeMask(computeMask) {
  init();
  _cameraController.setupPinholeCamera(
      CAMERA_CONTROLLER::VIEWPORT(0, 0 + resolution.width, 0,
                                  0 + resolution.height),
      zNear, zFar, fovY);
  _cameraController.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

  resetOrientation();
  setResolution(resolution);
  setFovY(fovY);
}

Single360degreeCameraHandler::~Single360degreeCameraHandler() { release(); }

void Single360degreeCameraHandler::resetOrientation() {
  // default orientation: camera looks into Y world coordinate (Z up)
  //    _cameraController.setOrientation(glm::vec3(1.0f, 0.0f, 0.0f),
  //    glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
  _cameraController.setOrientation(glm::vec3(0.0f, 1.0f, 0.0f),
                                   glm::vec3(0.0f, 0.0f, 1.0f),
                                   glm::vec3(1.0f, 0.0f, 0.0f));
}

void Single360degreeCameraHandler::setResolution(const cv::Size &resolution) {
  _cameraController.changeViewport(CAMERA_CONTROLLER::VIEWPORT(
      0, 0 + resolution.width, 0, 0 + resolution.height));

  releaseFBO();
  /*
   * You can create multiple framebuffers as you like and bind to them when you
   * please. (Give or take). You'll need a valid OpenGL context to use this,
   * which usually approximates to creating a window on most platforms, but you
   * don't ever have to draw anything into that window. more information on
   * http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
   */

  // Create Frame Buffer for offline rendering
  // Here we render the composition of the image and the projection of the mesh
  // on top of it in a texture (using FBO - Frame Buffer Object)
  glGenFramebuffers(1, &_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

  glViewport(0, 0, resolution.width, resolution.height);
  // Generate a render texture (which will contain the image and mesh in
  // wireframe overlay)
  glGenTextures(1, &_fboTextureID);
  //    glEnable(GL_TEXTURE_2D);
  //    glDisable(GL_COLOR_MATERIAL);
  glBindTexture(GL_TEXTURE_2D, _fboTextureID);
  // Give an empty image to OpenGL ( the last "0" as pointer )
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, resolution.width, resolution.height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Important note: here since we are inside a sphere, we don't need depth
  // buffer for depth testing.

  // Set "_fboTextureID" as our color attachment #0
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _fboTextureID, 0);

  // Set the list of draw buffers.
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);

  // Always check that our framebuffer is ok
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "invalid FrameBuffer" << std::endl;
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Single360degreeCameraHandler::release() {
  releaseFBO();

  if (_VAO)
    glDeleteVertexArrays(1, &_VAO);
  if (_VBO)
    glDeleteBuffers(1, &_VBO);

  if (_equirectangularImgTextureID)
    glDeleteTextures(1, &_equirectangularImgTextureID);

  if (_maskTextureID)
    glDeleteTextures(1, &_maskTextureID);

  _VAO = 0;
  _VBO = 0;
  _equirectangularImgTextureID = 0;
  _maskTextureID = 0;
}

void Single360degreeCameraHandler::releaseFBO() {
  if (_fbo)
    glDeleteFramebuffers(1, &_fbo);

  if (_fboTextureID)
    glDeleteTextures(1, &_fboTextureID);

  _fbo = 0;
  _fboTextureID = 0;
}

void Single360degreeCameraHandler::init() {
  OpenGLHandler::getInstance(); // it is called only to be sure that opengl
                                // context is created
  _equirectangularImgSize = cv::Size(0, 0);
  _equirectangularImgChannels = 0;
  _VBO = 0;
  _VAO = 0;
  _fbo = 0;
  _fboTextureID = 0;
  _equirectangularImgTextureID = 0;
  _maskTextureID = 0;

  glGenTextures(1, &_equirectangularImgTextureID);
  glEnable(GL_TEXTURE_2D);

  if (_computeMask)
    glGenTextures(1, &_maskTextureID);

  glGenVertexArrays(1, &_VAO);
  glGenBuffers(1, &_VBO);
  glBindVertexArray(_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, _VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Single360degreeCameraHandler::loadEquirectangularImage(
    cv::InputArray image) {
  if (!image.empty()) {
    if (image.cols() % 4 != 0)
      throw std::invalid_argument(
          "In opengl texture width should be divisable by 4");

    cv::Mat image_mat = image.getMat();
    int nChannels = image_mat.channels();
    CV_Assert(image_mat.isContinuous());

    GLint internalFormat = -1;
    GLenum sourceFormat = 0;
    if (nChannels == 1) {
      internalFormat = GL_RED;
      sourceFormat = GL_RED;
    } else if (nChannels == 3) {
      internalFormat = GL_RGB;
      sourceFormat = GL_BGR;
    } else if (nChannels == 4) {
      internalFormat = GL_RGBA;
      sourceFormat = GL_BGRA;
    }

    if ((image_mat.size() != _equirectangularImgSize) ||
        (nChannels != _equirectangularImgChannels)) {
      if (_equirectangularImgTextureID) // delete the previous one
        glDeleteTextures(1, &_equirectangularImgTextureID);
      _equirectangularImgTextureID = 0;
      glGenTextures(1, &_equirectangularImgTextureID);

      glBindTexture(GL_TEXTURE_2D, _equirectangularImgTextureID);
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image_mat.cols,
                   image_mat.rows, 0, sourceFormat, GL_UNSIGNED_BYTE,
                   image_mat.ptr());
      //        glGenerateMipmap(GL_TEXTURE_2D);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR); // GL_LINEAR_MIPMAP_LINEAR or GL_LINEAR
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      //            //        const float color[] = {0.0f, 0.0f, 0.0f, 0.0f};
      //            //        glTexParameterfv(GL_TEXTURE_2D,
      //            GL_TEXTURE_BORDER_COLOR, color);

      glBindTexture(GL_TEXTURE_2D, 0);
      _equirectangularImgSize = image_mat.size();
      _equirectangularImgChannels = nChannels;

      if (_computeMask) {
        if (_maskTextureID) // delete the previous one
          glDeleteTextures(1, &_maskTextureID);

        _maskTextureID = 0;
        glGenTextures(1, &_maskTextureID);
        glBindTexture(GL_TEXTURE_2D, _maskTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
        //    maskEquirectangular.cols, maskEquirectangular.rows, 0, GL_RGBA,
        //    GL_FLOAT, NULL);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, _equirectangularImgSize.width,
                     _equirectangularImgSize.height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, NULL);
      }
    } else {
      glBindTexture(GL_TEXTURE_2D, _equirectangularImgTextureID);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_mat.cols, image_mat.rows,
                      sourceFormat, GL_UNSIGNED_BYTE, image_mat.ptr());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
  }
}

void Single360degreeCameraHandler::getImage_BGR(cv::OutputArray image,
                                                cv::OutputArray mask) const {
  if (!_computeMask) // if mask is not set, then only return the view
  {
    std::cout << "Warning: computing mask is not set in the constructor."
              << std::endl;
    getImage_BGR(image);
    return;
  }
  const OpenGLHandler &oglHandler = OpenGLHandler::getInstance();
  const SHADER_PROGRAM &shader = oglHandler.get360VideoShaderProgram();
  // render to FBO
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /// \todo TODO: allocate mask only once
  mask.create(_equirectangularImgSize, CV_8UC1);
  cv::Mat maskEquirectangular = mask.getMat();

  memset(maskEquirectangular.ptr(), 0,
         maskEquirectangular.step[0] *
             maskEquirectangular
                 .rows); // maskEquirectangular.step[0] *
                         // maskEquirectangular.rows represents cv::Mat size in
                         // byte. It is almost 2 times faster than setTo(0)
  glBindTexture(GL_TEXTURE_2D, _maskTextureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, maskEquirectangular.cols,
                  maskEquirectangular.rows, GL_RED, GL_UNSIGNED_BYTE,
                  (void *)maskEquirectangular.ptr()); // clear texture

  const glm::mat4 &projection = _cameraController.getProjection();
  const glm::mat4 &view = _cameraController.getViewMatrix();

  shader.use();
  shader.setUniformValue("projection", projection);
  shader.setUniformValue("view", view);

  glActiveTexture(GL_TEXTURE0);
  shader.setUniformValue("tex", 0);
  // and finally bind the texture
  glBindTexture(GL_TEXTURE_2D, _equirectangularImgTextureID);

  glBindImageTexture(3, _maskTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
  shader.setUniformValue("computeMask", true);

  const CAMERA_CONTROLLER::VIEWPORT &vp = _cameraController.getViewport();

  float xZnear_max, xZnear_min, yZnear_max, yZnear_min;
  float zNear = _cameraController.getZNear();
  if (vp.width() >= vp.height()) {
    float aspect = vp.width() / vp.height();
    yZnear_max = zNear * tan(_cameraController.getFovY() *
                             (float)3.141592653589793 / (float)360.0);
    yZnear_min = -yZnear_max;
    xZnear_min = yZnear_min * aspect;
    xZnear_max = yZnear_max * aspect;
  } else {
    float aspect = vp.height() / vp.width();
    xZnear_max = zNear * tan(_cameraController.getFovY() *
                             (float)3.141592653589793 / (float)360.0);
    xZnear_min = -xZnear_max;
    yZnear_min = xZnear_min * aspect;
    yZnear_max = xZnear_max * aspect;
  }

  float scale = 1.001f;
  const glm::vec3 QuadWorld[4] = {
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_max, yZnear_min, -zNear)),
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_max, yZnear_max, -zNear)),
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_min, yZnear_max, -zNear)),
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_min, yZnear_min, -zNear))};

  glBindBuffer(GL_ARRAY_BUFFER, _VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QuadWorld), QuadWorld, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(
      _VAO); // seeing as we only have a single VAO there's no need to bind it
             // every time, but we'll do so to keep things a bit more organized
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindVertexArray(0);

  image.create(cv::Size(vp.width(), vp.height()),
               CV_MAKETYPE(CV_8U, _equirectangularImgChannels));

  cv::Mat img = image.getMat();
  //    img.setTo(0);

  GLenum format = 0;
  if (_equirectangularImgChannels == 1)
    format = GL_RED;
  else if (_equirectangularImgChannels == 3)
    format = GL_BGR;
  else if (_equirectangularImgChannels == 4)
    format = GL_BGRA;

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  // use fast 4-byte alignment (default anyway) if possible. This is because
  // cv::Mat not neccessarily storing image rows contiguously. There might be a
  // small padding value at the end of each row to make rows 4-byte aligned (or
  // 8?). So you need to mess with the pixel storage modes
  glPixelStorei(
      GL_PACK_ALIGNMENT,
      (img.step & 3) ? 1 : 4); // smart way to check divisible by 4: If a number
                               // is divisible by 4, the 2 lowest bits are not
                               // set therefore (number & 3) should be 0.
  // set length of one complete row in destination data (doesn't need to equal
  // img.cols)
  glPixelStorei(GL_PACK_ROW_LENGTH, img.step / img.elemSize());
  glReadPixels(0, 0, img.cols, img.rows, format, GL_UNSIGNED_BYTE, img.data);
  //    cv::flip(img, img, 0);

  glBindTexture(GL_TEXTURE_2D, _maskTextureID);
  glPixelStorei(GL_PACK_ALIGNMENT,
                (maskEquirectangular.step & 3)
                    ? 1
                    : 4); // smart way to check divisible by 4: If a number is
                          // divisible by 4, the 2 lowest bits are not set
                          // therefore (number & 3) should be 0.
  // set length of one complete row in destination data (doesn't need to equal
  // img.cols)
  glPixelStorei(GL_PACK_ROW_LENGTH,
                maskEquirectangular.step / maskEquirectangular.elemSize());
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE,
                maskEquirectangular.ptr());
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Single360degreeCameraHandler::getImage_BGR(cv::OutputArray image) const {
  const OpenGLHandler &oglHandler = OpenGLHandler::getInstance();
  const SHADER_PROGRAM &shader = oglHandler.get360VideoShaderProgram();
  // render to FBO
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const glm::mat4 &projection = _cameraController.getProjection();
  const glm::mat4 &view = _cameraController.getViewMatrix();

  shader.use();
  shader.setUniformValue("projection", projection);
  shader.setUniformValue("view", view);

  glActiveTexture(GL_TEXTURE0);
  shader.setUniformValue("tex", 0);
  // and finally bind the texture
  glBindTexture(GL_TEXTURE_2D, _equirectangularImgTextureID);

  shader.setUniformValue("computeMask", false);

  const CAMERA_CONTROLLER::VIEWPORT &vp = _cameraController.getViewport();

  float xZnear_max, xZnear_min, yZnear_max, yZnear_min;
  float zNear = _cameraController.getZNear();
  if (vp.width() >= vp.height()) {
    float aspect = vp.width() / vp.height();
    yZnear_max = zNear * tan(_cameraController.getFovY() *
                             (float)3.141592653589793 / (float)360.0);
    yZnear_min = -yZnear_max;
    xZnear_min = yZnear_min * aspect;
    xZnear_max = yZnear_max * aspect;
  } else {
    float aspect = vp.height() / vp.width();
    xZnear_max = zNear * tan(_cameraController.getFovY() *
                             (float)3.141592653589793 / (float)360.0);
    xZnear_min = -xZnear_max;
    yZnear_min = xZnear_min * aspect;
    yZnear_max = xZnear_max * aspect;
  }

  float scale = 1.001f;
  const glm::vec3 QuadWorld[4] = {
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_max, yZnear_min, -zNear)),
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_max, yZnear_max, -zNear)),
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_min, yZnear_max, -zNear)),
      _cameraController.cam2world(scale *
                                  glm::vec3(xZnear_min, yZnear_min, -zNear))};

  glBindBuffer(GL_ARRAY_BUFFER, _VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QuadWorld), QuadWorld, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(
      _VAO); // seeing as we only have a single VAO there's no need to bind it
             // every time, but we'll do so to keep things a bit more organized
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindVertexArray(0);

  image.create(cv::Size(vp.width(), vp.height()),
               CV_MAKETYPE(CV_8U, _equirectangularImgChannels));

  cv::Mat img = image.getMat();
  //    img.setTo(0);

  GLenum format = 0;
  if (_equirectangularImgChannels == 1)
    format = GL_RED;
  else if (_equirectangularImgChannels == 3)
    format = GL_BGR;
  else if (_equirectangularImgChannels == 4)
    format = GL_BGRA;

  // use fast 4-byte alignment (default anyway) if possible
  glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
  // set length of one complete row in destination data (doesn't need to equal
  // img.cols)
  glPixelStorei(GL_PACK_ROW_LENGTH, img.step / img.elemSize());
  glReadPixels(0, 0, img.cols, img.rows, format, GL_UNSIGNED_BYTE, img.data);
  //    cv::flip(img, img, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Single360degreeCameraHandler::setViewportDirection(double longitude_rad,
                                                        double latitude_rad) {
  const double PI = 3.14159265358979323846264338327950288;
  this->resetOrientation();
  this->pitch(glm::degrees(latitude_rad - (PI / 2.)));
  this->yaw(-glm::degrees(longitude_rad - PI));
}
