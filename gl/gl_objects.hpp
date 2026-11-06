#pragma once
// RAII wrappers for GL objects. Every object owns exactly one GL name and
// releases it in its destructor; none of them are copyable.

#include "gl_loader.hpp"

#include <string>

namespace enigma::gl {

class Buffer {
public:
    Buffer() { glGenBuffers(1, &id_); }
    ~Buffer() {
        if (id_ != 0) {
            glDeleteBuffers(1, &id_);
        }
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept : id_(other.id_) { other.id_ = 0; }
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (id_ != 0) {
                glDeleteBuffers(1, &id_);
            }
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }

    [[nodiscard]] GLuint id() const { return id_; }
    void bind() const { glBindBuffer(GL_ARRAY_BUFFER, id_); }
    void upload(GLsizeiptr sizeBytes, const void* data) {
        bind();
        glBufferData(GL_ARRAY_BUFFER, sizeBytes, data, GL_STATIC_DRAW);
    }
    void bindAsElements() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_); }
    void uploadElements(GLsizeiptr sizeBytes, const void* data) {
        bindAsElements();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeBytes, data, GL_STATIC_DRAW);
    }

private:
    GLuint id_ = 0;
};

class VertexArray {
public:
    VertexArray() { glGenVertexArrays(1, &id_); }
    ~VertexArray() {
        if (id_ != 0) {
            glDeleteVertexArrays(1, &id_);
        }
    }
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    VertexArray& operator=(VertexArray&& other) noexcept {
        if (this != &other) {
            if (id_ != 0) {
                glDeleteVertexArrays(1, &id_);
            }
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }

    [[nodiscard]] GLuint id() const { return id_; }
    void bind() const { glBindVertexArray(id_); }
    void bindAttribute(GLuint location, GLint components, GLsizei strideBytes,
                       std::size_t offsetBytes) const {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, components, GL_FLOAT, GL_FALSE,
                              strideBytes,
                              reinterpret_cast<const void*>(offsetBytes));
    }
    // Records an element-array binding inside this VAO. The VAO must
    // already be bound; the buffer must outlive this vertex array.
    void bindIndexBuffer(GLuint bufferId) const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferId);
    }

private:
    GLuint id_ = 0;
};

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram() {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
    }
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Compiles and links the two sources; returns false and fills log() on
    // any compile or link error.
    bool build(const char* vertexSource, const char* fragmentSource) {
        program_ = glCreateProgram();

        const GLuint vertex = compileStage(GL_VERTEX_SHADER, vertexSource);
        const GLuint fragment = compileStage(GL_FRAGMENT_SHADER, fragmentSource);
        if (vertex == 0 || fragment == 0) {
            if (vertex != 0) {
                glDeleteShader(vertex);
            }
            if (fragment != 0) {
                glDeleteShader(fragment);
            }
            return false;
        }
        glAttachShader(program_, vertex);
        glAttachShader(program_, fragment);
        glLinkProgram(program_);
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (linked != GL_FALSE) {
            return true;
        }
        char log[1024];
        GLsizei length = 0;
        glGetProgramInfoLog(program_, sizeof(log), &length, log);
        log_.assign(log, static_cast<size_t>(length));
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    [[nodiscard]] const std::string& log() const { return log_; }
    void use() const { glUseProgram(program_); }
    [[nodiscard]] GLint uniform(const char* name) const {
        return glGetUniformLocation(program_, name);
    }

    // Typed uniform upload helpers (column-major matrices, no transpose).
    void setMatrix4(GLint location, const float* columnMajor16) const {
        glUniformMatrix4fv(location, 1, GL_FALSE, columnMajor16);
    }
    void setVec3(GLint location, float x, float y, float z) const {
        glUniform3f(location, x, y, z);
    }
    void setFloat(GLint location, float value) const {
        glUniform1f(location, value);
    }

private:
    GLuint compileStage(GLenum stage, const char* source) {
        const GLuint shader = glCreateShader(stage);
        const GLchar* sources[] = {source};
        glShaderSource(shader, 1, sources, nullptr);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_FALSE) {
            char log[1024];
            GLsizei length = 0;
            glGetShaderInfoLog(shader, sizeof(log), &length, log);
            log_.assign(log, static_cast<size_t>(length));
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint program_ = 0;
    std::string log_;
};

}  // namespace enigma::gl
