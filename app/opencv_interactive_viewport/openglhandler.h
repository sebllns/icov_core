/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/
#ifndef OPENGLHANDLER_H
#define OPENGLHANDLER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_program.h"

class OpenGLHandler // singleton class (we need to have one and only one object of opengl context in system) to be sure that we have only one opengl context
{
private:
    OpenGLHandler() {initGL();}
    void initGL();
    GLFWwindow* _window;
    SHADER_PROGRAM* _shader360;
    SHADER_PROGRAM* _shader360_cubeMap;
    SHADER_PROGRAM* _shader360_healpix;
    SHADER_PROGRAM* _shaderMesh;
    ~OpenGLHandler()
    {
        glfwSetWindowShouldClose(_window, true);
        if(_shader360)
            delete _shader360;
        _shader360 = NULL;

        if(_shader360_cubeMap)
            delete _shader360_cubeMap;
        _shader360_cubeMap = NULL;

        if(_shader360_healpix)
            delete _shader360_healpix;
        _shader360_healpix = NULL;

        if(_shaderMesh)
            delete _shaderMesh;
        _shaderMesh = NULL;

        glfwTerminate();    // properly clean/delete all resources that were allocated

    }
public:

    OpenGLHandler(OpenGLHandler const&) = delete;   // to make sure they are unacceptable otherwise you may accidentally get copies of your singleton appearing.
    OpenGLHandler& operator=(OpenGLHandler const&) = delete;  // to make sure they are unacceptable otherwise you may accidentally get copies of your singleton appearing.
    static const OpenGLHandler& getInstance()
    {
        static OpenGLHandler instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    const SHADER_PROGRAM& get360VideoShaderProgram() const {return (*_shader360);}
    const SHADER_PROGRAM& get360CubeMapShaderProgram() const {return (*_shader360_cubeMap);}
    const SHADER_PROGRAM& get360HealpixShaderProgram() const {return (*_shader360_healpix);}
    const SHADER_PROGRAM& getMeshShaderProgram() const {return (*_shaderMesh);}
};

#endif // OPENGLHANDLER_H
