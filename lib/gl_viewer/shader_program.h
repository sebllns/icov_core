/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Inria research institute
**************************************************************************/
#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H


#include <string>
#include <vector>
#include <map>

#include <glad/glad.h> // include glad to get all the required OpenGL headers
#include <glm/glm.hpp>


class SHADER_PROGRAM
{
public:
    enum SHADER_TYPE{VERTEX_SHADER=0, FARGMENT_SHADER, GEOMETRY_SHADER};    // enum for shader type

protected:
    // the program ID
    GLuint _ID;
    bool _linked;

    std::map<SHADER_TYPE, GLuint> _shadersMap;
public:
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    SHADER_PROGRAM();
    SHADER_PROGRAM(const SHADER_PROGRAM& other);
    ~SHADER_PROGRAM();

    SHADER_PROGRAM& operator=(const SHADER_PROGRAM& other);
    bool link(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath="");
    bool link();
    void release();

    bool addShaderFile(SHADER_TYPE shaderType, const std::string& filePath);
    bool addShaderSourceCode(SHADER_TYPE shaderType, const std::string& sourceCode);

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(_ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------

    void setUniformValue(const std::string &name, GLint value) const
    {
        glUniform1i(getUniformLocation(name), value);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, GLfloat value) const
    {
        glUniform1f(getUniformLocation(name), value);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(getUniformLocation(name), 1, &value[0]);
    }
    void setUniformValue(const std::string &name, GLfloat x, GLfloat y) const
    {
        glUniform2f(getUniformLocation(name), x, y);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(getUniformLocation(name), 1, &value[0]);
    }
    void setUniformValue(const std::string &name, GLfloat x, GLfloat y, GLfloat z) const
    {
        glUniform3f(getUniformLocation(name), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(getUniformLocation(name), 1, &value[0]);
    }
    void setUniformValue(const std::string &name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const
    {
        glUniform4f(getUniformLocation(name), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setUniformValue(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    inline GLuint ID() const
    {
        return _ID;
    }
    GLint getUniformLocation(const std::string &name) const;
    inline bool isLinked() const
    {
        return _linked;
    }

protected:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    bool checkCompileErrors(GLuint shader, std::string type);
    bool init();
};

#endif // SHADER_PROGRAM_H
