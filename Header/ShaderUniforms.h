#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Cached shader uniform locations for performance
struct ShaderUniforms {
    GLint uM;
    GLint uV;
    GLint uP;
    GLint uViewPos;
    GLint uMaterial_kD;
    GLint uMaterial_kA;
    GLint uMaterial_kS;
    GLint uMaterial_shine;
    GLint uUseTexture;
    GLint uTexture;
    GLint uUseFog;
    GLint uFogColor;
    GLint uFogDensity;
    GLint uLight_pos;
    GLint uLight_kA;
    GLint uLight_kD;
    GLint uLight_kS;
    GLint uWatchLight_pos;
    GLint uWatchLight_kA;
    GLint uWatchLight_kD;
    GLint uWatchLight_kS;
    GLint uUseWatchLight;

    void init(unsigned int shader) {
        uM = glGetUniformLocation(shader, "uM");
        uV = glGetUniformLocation(shader, "uV");
        uP = glGetUniformLocation(shader, "uP");
        uViewPos = glGetUniformLocation(shader, "uViewPos");
        uMaterial_kD = glGetUniformLocation(shader, "uMaterial.kD");
        uMaterial_kA = glGetUniformLocation(shader, "uMaterial.kA");
        uMaterial_kS = glGetUniformLocation(shader, "uMaterial.kS");
        uMaterial_shine = glGetUniformLocation(shader, "uMaterial.shine");
        uUseTexture = glGetUniformLocation(shader, "uUseTexture");
        uTexture = glGetUniformLocation(shader, "uTexture");
        uUseFog = glGetUniformLocation(shader, "uUseFog");
        uFogColor = glGetUniformLocation(shader, "uFogColor");
        uFogDensity = glGetUniformLocation(shader, "uFogDensity");
        uLight_pos = glGetUniformLocation(shader, "uLight.pos");
        uLight_kA = glGetUniformLocation(shader, "uLight.kA");
        uLight_kD = glGetUniformLocation(shader, "uLight.kD");
        uLight_kS = glGetUniformLocation(shader, "uLight.kS");
        uWatchLight_pos = glGetUniformLocation(shader, "uWatchLight.pos");
        uWatchLight_kA = glGetUniformLocation(shader, "uWatchLight.kA");
        uWatchLight_kD = glGetUniformLocation(shader, "uWatchLight.kD");
        uWatchLight_kS = glGetUniformLocation(shader, "uWatchLight.kS");
        uUseWatchLight = glGetUniformLocation(shader, "uUseWatchLight");
    }

    // Helper methods for common operations
    void setModelMatrix(const glm::mat4& m) const {
        glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(m));
    }

    void setViewMatrix(const glm::mat4& v) const {
        glUniformMatrix4fv(uV, 1, GL_FALSE, glm::value_ptr(v));
    }

    void setProjectionMatrix(const glm::mat4& p) const {
        glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(p));
    }

    void setMaterial(const glm::vec3& kD, const glm::vec3& kA, const glm::vec3& kS, float shine) const {
        glUniform3fv(uMaterial_kD, 1, glm::value_ptr(kD));
        glUniform3fv(uMaterial_kA, 1, glm::value_ptr(kA));
        glUniform3fv(uMaterial_kS, 1, glm::value_ptr(kS));
        glUniform1f(uMaterial_shine, shine);
    }

    void setTexture(bool use, GLuint texId = 0) const {
        glUniform1i(uUseTexture, use ? 1 : 0);
        if (use && texId != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texId);
            glUniform1i(uTexture, 0);
        }
    }

    void setFog(bool use, const glm::vec3& color = glm::vec3(0), float density = 0) const {
        glUniform1i(uUseFog, use ? 1 : 0);
        if (use) {
            glUniform3fv(uFogColor, 1, glm::value_ptr(color));
            glUniform1f(uFogDensity, density);
        }
    }
};
