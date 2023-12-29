/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/
#ifndef SINGLE360DEGREECAMERAHANDLER_H
#define SINGLE360DEGREECAMERAHANDLER_H
#include "camera_controller.h"
#include "shader_program.h"
#include <opencv2/core.hpp>

class Single360degreeCameraHandler
{
protected:
    CAMERA_CONTROLLER _cameraController;
    unsigned int _VBO, _VAO;
    unsigned int _fboTextureID, _fbo;   // "handles" for framebuffer and it's texture
    GLuint _equirectangularImgTextureID, _maskTextureID;
    cv::Size _equirectangularImgSize;
    int _equirectangularImgChannels;
    bool _computeMask;

public:
    Single360degreeCameraHandler(const cv::Size& resolution, float fovY, bool computeMask);
    ~Single360degreeCameraHandler();

    void resetOrientation();
    inline void setFovY(float fovY_degree) { _cameraController.setFovY(fovY_degree);}
    void setResolution(const cv::Size& resolution);
    void release();
    void loadEquirectangularImage(cv::InputArray image);
    void getImage_BGR(cv::OutputArray image, cv::OutputArray mask) const;
    void getImage_BGR(cv::OutputArray image) const;
    inline void yaw(float angle_degree) {_cameraController.rotateAxis(glm::vec3(0, 0, 1), angle_degree);}
    inline void pitch(float angle_degree) {_cameraController.pitch(angle_degree);}
    inline void rotate(const glm::quat& q) {_cameraController.rotate(q);}
    void setViewportDirection(double longitude_rad, double latitude_rad);
    const glm::quat& getOrientation() const {return _cameraController.getOrientation();}
    void setOrientation(const glm::quat& quat) {_cameraController.setOrientation(quat);}
    void rotateAxis(const glm::vec3 &axis, float angle_degree) {_cameraController.rotateAxis(axis, angle_degree);}
    const CAMERA_CONTROLLER& get_cameraController() const {return _cameraController;}
    const cv::Size& getEquirectangularImageSize() const {return _equirectangularImgSize;}

protected:
    void releaseFBO();
    void init();
};

#endif // SINGLE360DEGREECAMERAHANDLER_H
