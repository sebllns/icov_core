/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/
#include "shader_program.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;




GLint SHADER_PROGRAM::getUniformLocation(const string &name) const
{
    if( _ID && _linked)
        return glGetUniformLocation(_ID, name.c_str());
    else
    {
        cerr << "Error SHADER_PROGRAM::getUniformLocation: shader program is not linked" << endl;
        return -1;
    }

}


SHADER_PROGRAM::SHADER_PROGRAM()
{
    release();  // for initialization. Keep in mind that before creating object from SHADER_PROGRAM the opengl context has to be created before, otherwise you will end up in segmentation fault

    init();
}

SHADER_PROGRAM::SHADER_PROGRAM(const SHADER_PROGRAM &other)
{
    release();  // for initialization Keep in mind that before creating object from SHADER_PROGRAM the opengl context has to be created before, otherwise you will end up in segmentation fault

    (*this) = other;
}

SHADER_PROGRAM::~SHADER_PROGRAM()
{
    release();
}

SHADER_PROGRAM &SHADER_PROGRAM::operator=(const SHADER_PROGRAM &other)
{
    if(this!=&other) // self-assignment check expected
    {
        this->_ID = other._ID;
        this->_linked = other._linked;
        this->_shadersMap = other._shadersMap;
    }

    return *this;
}

bool SHADER_PROGRAM::link(const string &vertexPath, const string &fragmentPath, const string &geometryPath)
{
    vector<string> shaderPaths = {vertexPath, fragmentPath};    // existence of vertex and fragment shader is obligatory
    if(!geometryPath.empty())   // existence of geometry shader is optional
        shaderPaths.push_back(geometryPath);

    // compile each shader
    for(unsigned int shaderType=0; shaderType < shaderPaths.size(); shaderType++)
    {
        if( !addShaderFile(static_cast<SHADER_TYPE>(shaderType), shaderPaths[shaderType]) && (shaderType <  GEOMETRY_SHADER) )   // only vertex shader and fragment shader is important. Others (geometry shader, TESS_CONTROL_SHADER, etc. ) can be neglected if not available. This line first apply addShader, then check if it fails and if it fails and the failure is related to vertex or fragment shader, it returns false
            return false;
    }

    return link();
}

bool SHADER_PROGRAM::link()
{
    if( (_shadersMap.find(SHADER_TYPE::VERTEX_SHADER) == _shadersMap.end()) || (_shadersMap.find(SHADER_TYPE::FARGMENT_SHADER) == _shadersMap.end()))   // if the shader type already exists
    {
        cout << "Error SHADER_PROGRAM::link : vertex shader and fragment shader must be complied before linking the program" << endl;
        return false;
    }

    // shader Program
    if(!init()) // failed to create the program
        return false;

    std::map<SHADER_TYPE, GLuint>::iterator it;

    for(it = _shadersMap.begin(); it !=_shadersMap.end(); it++)
        glAttachShader(_ID, it->second);

    glLinkProgram(_ID);
    if(!checkCompileErrors(_ID, "PROGRAM"))
        return false;

    // delete the shaders as they're linked into our program now and no longer necessery
    for(it = _shadersMap.begin(); it !=_shadersMap.end(); it++)
        glDeleteShader(it->second);

    _linked = true;
    return true;
}

void SHADER_PROGRAM::release()
{
    glUseProgram(0);
    _ID = 0;
    _linked = false;
    _shadersMap.clear();
}

bool SHADER_PROGRAM::addShaderFile(SHADER_PROGRAM::SHADER_TYPE shaderType, const string &filePath)
{
    string ShaderSourceCode;
    ifstream shaderFile;


    // 1. retrieve the shader source code from filePath
    cout << "compiling shader file: " << filePath << endl;
    shaderFile.open(filePath);
    if(!shaderFile.is_open())
    {
        cout << "Error: can not open file " << filePath << endl;
        return false;
    }


    stringstream shaderStream;
    // read file's buffer contents into streams
    shaderStream << shaderFile.rdbuf();
    // close file handlers
    shaderFile.close();
    // convert stream into string
    ShaderSourceCode = shaderStream.str();

    return addShaderSourceCode(shaderType, ShaderSourceCode);

}

bool SHADER_PROGRAM::addShaderSourceCode(SHADER_PROGRAM::SHADER_TYPE shaderType, const string &sourceCode)
{
    if(_shadersMap.find(shaderType) != _shadersMap.end())   // if the shader type already exists
    {
        cout << "Error SHADER_PROGRAM::addShader : shader type already exists" << endl;
        return false;
    }

    const char* shaderSourceCode_c_str = sourceCode.c_str();

    // 2. compile shaders
    GLuint shaderObj = 0;

    switch (shaderType) {
    case VERTEX_SHADER:
        shaderObj = glCreateShader(GL_VERTEX_SHADER);
        break;
    case FARGMENT_SHADER:
        shaderObj = glCreateShader(GL_FRAGMENT_SHADER);
        break;
    case GEOMETRY_SHADER:
        shaderObj = glCreateShader(GL_GEOMETRY_SHADER);
        break;
    }

    glShaderSource(shaderObj, 1, &shaderSourceCode_c_str, NULL);
    glCompileShader(shaderObj);
    if(!checkCompileErrors(shaderObj, "shader object"))
        return false;

    _shadersMap.insert(std::make_pair(shaderType, shaderObj));  // difference between operator [] and insert function: f specified key already existed in map then operator [] will silently change its value where as insert will not replace already added key instead it returns the information
    return true;
}

bool SHADER_PROGRAM::checkCompileErrors(GLuint shader, string type)
{
    GLint success;
    GLchar infoLog[1024];
    if(type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    return success;

}

bool SHADER_PROGRAM::init()
{
    if(_ID)    // already program object has been created
        return true;

    _ID = glCreateProgram();    // This function returns 0 if an error occurs creating the program object.
    return _ID; // change to bool. true : sucees creating program. false: failure

}

