/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/
#include "camera_controller.h"

#include <glm/gtc/matrix_transform.hpp>
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/rotate_vector.hpp>
//#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <vector>

// Default camera values
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.001f;
const float FOVY_DEGREE = 0.0f;

CAMERA_CONTROLLER::CAMERA_CONTROLLER()
{


    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 X_camInWorld = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 Y_camInWorld = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 Z_camInWorld = glm::vec3(0.0f, -1.0f, 0.0f);

    setPosition(position);
    setOrientation(X_camInWorld, Y_camInWorld, Z_camInWorld);

    _movementSpeed = SPEED;
    _mouseSensitivity = SENSITIVITY;
    setupPinholeCamera(VIEWPORT(0.0f, 100.0f, 0.0f, 100.0f), 0.01f, 100.0f, FOVY_DEGREE);

}

CAMERA_CONTROLLER::CAMERA_CONTROLLER(const CAMERA_CONTROLLER &other)
{
    (*this) = other;
}

CAMERA_CONTROLLER &CAMERA_CONTROLLER::operator=(const CAMERA_CONTROLLER &other)
{
    if(this!=&other) // self-assignment check expected
    {
        this->_fovY_degree = other._fovY_degree;
        this->_zNear = other._zNear;
        this->_zFar = other._zFar;
        this->_isOrtho = other._isOrtho;
        this->_viewport = other._viewport;

        this->_projection = other._projection;
        this->_world2cam = other._world2cam;
        this->_cam2world = other._cam2world;

        this->_movementSpeed = other._movementSpeed;
        this->_mouseSensitivity = other._mouseSensitivity;
    }

    return *this;
}
void CAMERA_CONTROLLER::setPosition(const glm::vec3 &position)
{
    _position = position;
    invalidateTransformation();
}

void CAMERA_CONTROLLER::setOrientation(const glm::vec3 &X_camInWorld, const glm::vec3 &Y_camInWorld, const glm::vec3 &Z_camInWorld)
{
    glm::mat3 rot_camInWorld;
    rot_camInWorld[0] = X_camInWorld;
    rot_camInWorld[1] = Y_camInWorld;
    rot_camInWorld[2] = Z_camInWorld;

    _qOrientation_camInWorld = glm::quat_cast(rot_camInWorld);
    invalidateTransformation();
}

void CAMERA_CONTROLLER::move(const glm::vec3 &m_inCameraCoordinate)
{
    // translating only affects _position. Therefore:
//    _position += _X_camInWorld * m[0] + _Y_camInWorld * m[1] + _Z_camInWorld * m[2];
    const glm::mat4& camInWorld = cam2world();
    _position += glm::vec3(camInWorld[0] * m_inCameraCoordinate[0] +
                           camInWorld[1] * m_inCameraCoordinate[1] +
                           camInWorld[2] * m_inCameraCoordinate[2]);

    // The above equation is equal to the following
    /*
    glm::mat4 T1(1.0f);
    T1[3] = glm::vec4(m, 1.0f);
    const glm::mat4& cam2world = this->cam2world();
    cam2world = cam2world * T1;
    _position = glm::vec3(cam2world[3]);
    */
    invalidateTransformation();
}

void CAMERA_CONTROLLER::yaw(float angle_degree)
{
    float angle_radian = glm::radians(angle_degree);
//    _X_camInWorld = glm::rotate(_X_camInWorld, angle_radian, _Y_camInWorld);
//    _Z_camInWorld = glm::rotate(_Z_camInWorld, angle_radian, _Y_camInWorld);

    const glm::mat4& camInWorld = cam2world();
    glm::quat q = glm::angleAxis(angle_radian, glm::vec3(camInWorld[1]));

    //order matters,update camera_quat
    _qOrientation_camInWorld = glm::normalize(q * _qOrientation_camInWorld);

    invalidateTransformation();
}

void CAMERA_CONTROLLER::pitch(float angle_degree)
{
    float angle_radian = glm::radians(angle_degree);
//    _Y_camInWorld = glm::rotate(_Y_camInWorld, angle_radian, _X_camInWorld);
//    _Z_camInWorld = glm::rotate(_Z_camInWorld, angle_radian, _X_camInWorld);

    const glm::mat4& camInWorld = cam2world();
    glm::quat q = glm::angleAxis(angle_radian, glm::vec3(camInWorld[0]));

    //order matters,update camera_quat
    _qOrientation_camInWorld = glm::normalize(q * _qOrientation_camInWorld);

    invalidateTransformation();
}

void CAMERA_CONTROLLER::roll(float angle_degree)
{
    float angle_radian = glm::radians(angle_degree);
//    _X_camInWorld = glm::rotate(_X_camInWorld, angle_radian, _Z_camInWorld);
//    _Y_camInWorld = glm::rotate(_Y_camInWorld, angle_radian, _Z_camInWorld);

    const glm::mat4& camInWorld = cam2world();
    glm::quat q = glm::angleAxis(angle_radian, glm::vec3(camInWorld[2]));

    //order matters,update camera_quat
    _qOrientation_camInWorld = glm::normalize(q * _qOrientation_camInWorld);

    invalidateTransformation();
}

void CAMERA_CONTROLLER::rotateAxis(const glm::vec3 &axis, float angle_degree)
{
    float angle_radian = glm::radians(angle_degree);
//    _X_camInWorld = glm::rotate(_X_camInWorld, angle_radian, axis);
//    _Y_camInWorld = glm::rotate(_Y_camInWorld, angle_radian, axis);
//    _Z_camInWorld = glm::rotate(_Z_camInWorld, angle_radian, axis);

    glm::vec3 n = normalize(axis);
    glm::quat q = angleAxis(angle_radian, n);

    //order matters,update camera_quat
    _qOrientation_camInWorld = glm::normalize(q * _qOrientation_camInWorld);

    invalidateTransformation();
}

void CAMERA_CONTROLLER::rotate(const glm::quat &q)
{
    //order matters,update camera_quat
    _qOrientation_camInWorld = glm::normalize(q * _qOrientation_camInWorld);

    invalidateTransformation();
}

void CAMERA_CONTROLLER::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    const glm::mat4& camInWorld = cam2world();
    // this is an special case of move function where two components of m out of 3 are zero;
    float velocity = _movementSpeed * deltaTime;
    if (direction == FORWARD)
        _position += glm::vec3(-camInWorld[2]) * velocity;
    if (direction == BACKWARD)
        _position -= glm::vec3(-camInWorld[2]) * velocity;
    if (direction == LEFT)
        _position -= glm::vec3(camInWorld[0]) * velocity;
    if (direction == RIGHT)
        _position += glm::vec3(camInWorld[0]) * velocity;

//    std::cout << "position= (" << camInWorld[3][0] << ", " << camInWorld[3][1] << ", " << camInWorld[3][2] << ", " << camInWorld[3][3] << "), " <<
//                        "r= "  << std::sqrt(camInWorld[3][0]*camInWorld[3][0] + camInWorld[3][1]*camInWorld[3][1]+ camInWorld[3][2]*camInWorld[3][2]) << std::endl;

    invalidateTransformation();
}

void CAMERA_CONTROLLER::ProcessMouseMovement(float xoffset, float yoffset, Mouse_Navigation mode)
{
    xoffset *= _mouseSensitivity;
    yoffset *= _mouseSensitivity;

    switch (mode) {
    case NAV_DRAG:
    {
        pitch( yoffset *  getFovY());
        rotateAxis(glm::vec3(0, 0, 1), xoffset * getFovY() );
//        yaw(xoffset * getFovY());
    }
        break;
    case NAV_FLY:
    {
        pitch( yoffset );
        rotateAxis(glm::vec3(0, 0, 1), xoffset );
//        std::cout << "pitch offset= " << yoffset << ", yaw offset= " << xoffset << std::endl;
//        yaw(xoffset);
    }
        break;
    default:
        break;
    }
    invalidateTransformation();
}

void CAMERA_CONTROLLER::ProcessMouseScroll(float yoffset)
{
    if (_fovY_degree >= 1.0f && _fovY_degree < 180.0f)
        _fovY_degree -= yoffset;
    if (_fovY_degree <= 1.0f)
        _fovY_degree = 1.0f;
    if (_fovY_degree >= 180.0f)
        _fovY_degree = 180.0f;
    std::cout << "yoffset= " << yoffset << ", fovy= " << _fovY_degree << std::endl;
//    invalidateMatrices();
    invalidateProjection();
}



void CAMERA_CONTROLLER::setupPinholeCamera(const CAMERA_CONTROLLER::VIEWPORT &vp, float zNear, float zFar, float fovY_degree)
{
    _isOrtho = false;
    _fovY_degree = fovY_degree;
    _viewport = vp;
    _zNear = zNear;
    _zFar =zFar;
//    invalidateMatrices();
    invalidateProjection();
}


void CAMERA_CONTROLLER::setupOrthographicCamera(const CAMERA_CONTROLLER::VIEWPORT &vp, float zNear, float zFar)
{
    _isOrtho = true;
    _fovY_degree = 0.0f;
    _viewport = vp;
    _zNear = zNear;
    _zFar =zFar;
//    invalidateMatrices();
    invalidateProjection();
}

void CAMERA_CONTROLLER::changeViewport(const CAMERA_CONTROLLER::VIEWPORT &vp)
{
    if(_isOrtho)
        setupOrthographicCamera(vp, _zNear, _zFar);
    else
        setupPinholeCamera(vp, _zNear, _zFar, _fovY_degree);
}

const glm::mat4 &CAMERA_CONTROLLER::cam2world() const
{
    if(_cam2world.first)
    {
        glm::mat3 R_camInWorld = glm::mat3_cast(_qOrientation_camInWorld);

        _cam2world.second[0] = glm::vec4(R_camInWorld[0], 0.0f);
        _cam2world.second[1] = glm::vec4(R_camInWorld[1], 0.0f);
        _cam2world.second[2] = glm::vec4(R_camInWorld[2], 0.0f);
        _cam2world.second[3] = glm::vec4(_position, 1.0f);
        _cam2world.first = false;
    }

    return _cam2world.second;
}

glm::vec3 CAMERA_CONTROLLER::cam2world(const glm::vec3 &c) const
{
    glm::vec4 v(c,1.0f);
    glm::vec4 w = cam2world() * v;
    return glm::vec3(w[0]/w[3],w[1]/w[3],w[2]/w[3]);

    // TODO: simply replace (AND check) the above codes with the following code which is implemented by glm in "quaternion.inl". For more information that why this one is optimal, please refer to https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
//    return _qOrientation_camInWorld * c;
}

const glm::mat4 &CAMERA_CONTROLLER::getViewMatrix() const
{
    return world2cam();
}

const glm::mat4 &CAMERA_CONTROLLER::world2cam() const
{
    if(_world2cam.first)
    {
        // R_worldInCam = transpose(R_camInWorld) or q_worldInCam[0:2] = -q_camInWorld[0:2]
        // Compute inverse rotation q
        glm::quat q = _qOrientation_camInWorld;
        q.x *= -1.0f;
        q.y *= -1.0f;
        q.z *= -1.0f;
        glm::mat3 R_worldInCam = glm::mat3_cast(q);


        // P_worldInCam = -transpose(R_camInWorld) * P_camInWorld;
        glm::vec3 P_worldInCam= -R_worldInCam * _position;
        _world2cam.second[0] = glm::vec4(R_worldInCam[0], 0.0f);
        _world2cam.second[1] = glm::vec4(R_worldInCam[1], 0.0f);
        _world2cam.second[2] = glm::vec4(R_worldInCam[2], 0.0f);
        _world2cam.second[3] = glm::vec4(P_worldInCam, 1.0f);
        // the above is equal to:
//        _world2cam.second = glm::lookAt(_position, -_Z_camInWorld, _Y_camInWorld);
        _world2cam.first = false;

    }
    return _world2cam.second;
}

const glm::mat4 &CAMERA_CONTROLLER::getProjection() const
{
    if(_projection.first)
    {
        if(_isOrtho)
        {
            _projection.second = glm::ortho(_viewport.x_min, _viewport.x_max, _viewport.y_min, _viewport.y_max, _zNear, _zFar);
        }
        else
        {
            float aspect = _viewport.aspect();
//            if(aspect < 1.)
//                aspect = 1./aspect;
            _projection.second = glm::perspective(glm::radians(_fovY_degree), aspect, _zNear, _zFar);
        }

        _projection.first = false;
    }
   return  _projection.second;
}



//void CAMERA_CONTROLLER::invalidateMatrices()
//{
//    invalidateProjection();
//    invalidateTransformation();
//}

void CAMERA_CONTROLLER::setFovY(float fovY_degree)
{
    _fovY_degree = fovY_degree;
//    invalidateMatrices();
    invalidateProjection();
}

