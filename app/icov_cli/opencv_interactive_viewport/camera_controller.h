/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/
#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods



class CAMERA_CONTROLLER
{
public:
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    enum Mouse_Navigation {
        NAV_NONE = 0,
        NAV_DRAG,   // LMB: drag screen
        NAV_FLY     // RMB: scroll screen
    };

    struct VIEWPORT // 2D viewport size
    {
        float x_min, x_max, y_min, y_max;

        VIEWPORT(float x0, float x1, float y0, float y1)
        {
            set(x0,x1,y0, y1);
        }
        VIEWPORT()
        {
            set(0.0f, 0.0f, 0.0f, 0.0f);
        }

        void set(float x0, float x1, float y0, float y1)
        {
            if(x0 < x1)
            {
                x_min = x0;
                x_max = x1;
            }
            else
            {
                x_min = x1;
                x_max = x0;
            }
            if( y0 < y1)
            {
                y_min = y0;
                y_max = y1;
            }
            else
            {
                y_min = y1;
                y_max = y0;
            }
        }

        float width() const {return x_max - x_min;}
        float height() const {return y_max - y_min;}
        float aspect() const {return width()/height();}
    };

protected:
//    // Camera Attributes
//    glm::vec3 _position;
//    glm::vec3 _X_camInWorld;
//    glm::vec3 _Y_camInWorld;
//    glm::vec3 _Z_camInWorld;
    glm::vec3 _position; // Location of camera in world coordinates.
    glm::quat _qOrientation_camInWorld; // quaternion Orientation of camera basis vectors specified in world coordinates.


    float _fovY_degree;
    float _zNear;
    float _zFar;
    bool _isOrtho;
    VIEWPORT _viewport;



    mutable std::pair<bool, glm::mat4> _projection;
    mutable std::pair<bool, glm::mat4> _cam2world;
    mutable std::pair<bool, glm::mat4> _world2cam;

    // Camera options
    float _movementSpeed;
    float _mouseSensitivity;


public:
    CAMERA_CONTROLLER();
    CAMERA_CONTROLLER(const CAMERA_CONTROLLER& other);
    CAMERA_CONTROLLER &operator=(const CAMERA_CONTROLLER& other);

    void setFovY(float fovY_degree);
    inline float getFovY() const {return _fovY_degree;}
    void setupPinholeCamera(const VIEWPORT& vp, float zNear, float zFar, float fovY_degree);
    void setupOrthographicCamera(const VIEWPORT& vp, float zNear, float zFar);
    inline bool isOrthographicCamera() const {return _isOrtho;}
    void changeViewport(const VIEWPORT& vp);



    const glm::mat4& cam2world() const;
    glm::vec3 cam2world(const glm::vec3 &c) const;
    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    const glm::mat4& getViewMatrix() const;

    const glm::mat4& world2cam() const;
    const glm::mat4& getProjection() const;


    inline const glm::vec3& getPosition() const {return _position;}
    void setPosition(const glm::vec3 &position);
    void setOrientation(const glm::vec3 &X_camInWorld, const glm::vec3 &Y_camInWorld, const glm::vec3 &Z_camInWorld);
    inline void setOrientation(const glm::quat& quat) {_qOrientation_camInWorld = quat; invalidateTransformation();}
    void move(const glm::vec3 &m_inCameraCoordinate);

    // rotations
    void yaw(float angle_degree);
    void pitch(float angle_degree);
    void roll(float angle_degree);
    void rotateAxis(const glm::vec3 &axis, float angle_degree);
    void rotate(const glm::quat& q);


    // process inputs

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, Mouse_Navigation mode);

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

    float getZNear() const    { return _zNear; }
    float getZFar() const    { return _zFar;    }
    const VIEWPORT& getViewport() const    { return _viewport;}
    inline const glm::quat& getOrientation() const {return _qOrientation_camInWorld;}

protected:
    inline void invalidateProjection()    {_projection.first = true; }
    inline void invalidateTransformation()    { _world2cam.first = true; _cam2world.first = true; }
//    inline void invalidateMatrices();

};

#endif // CAMERA_CONTROLLER_H
