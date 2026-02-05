#include "../Header/DigitRenderer.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <iostream>

DigitRenderer::DigitRenderer() : VAO(0), VBO(0) {}

DigitRenderer::~DigitRenderer() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

void DigitRenderer::init() {
    float vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void DigitRenderer::drawSegment(float x, float y, float w, float h, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    if (VAO == 0) init();

    glm::mat4 model = parentModel;
    model = glm::translate(model, glm::vec3(x, y, 0.02f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(color));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DigitRenderer::drawDigit(int digit, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    float t = 0.1f * scale;
    float w = 0.5f * scale;
    float h = 1.0f * scale;
    float sh = h / 2.0f;

    bool s[7] = {false};
    switch(digit) {
        case 0: s[0]=1; s[1]=1; s[2]=1; s[3]=1; s[4]=1; s[5]=1; break;
        case 1: s[1]=1; s[2]=1; break;
        case 2: s[0]=1; s[1]=1; s[6]=1; s[4]=1; s[3]=1; break;
        case 3: s[0]=1; s[1]=1; s[6]=1; s[2]=1; s[3]=1; break;
        case 4: s[5]=1; s[6]=1; s[1]=1; s[2]=1; break;
        case 5: s[0]=1; s[5]=1; s[6]=1; s[2]=1; s[3]=1; break;
        case 6: s[0]=1; s[5]=1; s[4]=1; s[3]=1; s[2]=1; s[6]=1; break;
        case 7: s[0]=1; s[1]=1; s[2]=1; break;
        case 8: s[0]=1; s[1]=1; s[2]=1; s[3]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 9: s[0]=1; s[1]=1; s[2]=1; s[3]=1; s[5]=1; s[6]=1; break;
    }

    if(s[0]) drawSegment(x, y + h - t, w, t, color, shader, parentModel);
    if(s[1]) drawSegment(x + w - t, y + sh, t, sh, color, shader, parentModel);
    if(s[2]) drawSegment(x + w - t, y, t, sh, color, shader, parentModel);
    if(s[3]) drawSegment(x, y, w, t, color, shader, parentModel);
    if(s[4]) drawSegment(x, y, t, sh, color, shader, parentModel);
    if(s[5]) drawSegment(x, y + sh, t, sh, color, shader, parentModel);
    if(s[6]) drawSegment(x, y + sh - t/2.0f, w, t, color, shader, parentModel);
}

void DigitRenderer::drawNumber(int number, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    std::string str = std::to_string(number);
    float spacing = 0.6f * scale; 
    
    for(const char& c : str) {
        drawDigit(c - '0', x, y, scale, color, shader, parentModel);
        x += spacing;
    }
}

void DigitRenderer::drawTime(int hh, int mm, int ss, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    float spacing = 0.6f * scale;
    float colonSpacing = 0.3f * scale;
    
    drawDigit(hh / 10, x, y, scale, color, shader, parentModel); x += spacing;
    drawDigit(hh % 10, x, y, scale, color, shader, parentModel); x += spacing;
    
    drawColon(x, y, scale, color, shader, parentModel); x += colonSpacing;
    
    drawDigit(mm / 10, x, y, scale, color, shader, parentModel); x += spacing;
    drawDigit(mm % 10, x, y, scale, color, shader, parentModel); x += spacing;
    
    drawColon(x, y, scale, color, shader, parentModel); x += colonSpacing;
    
    drawDigit(ss / 10, x, y, scale, color, shader, parentModel); x += spacing;
    drawDigit(ss % 10, x, y, scale, color, shader, parentModel); x += spacing;
}

void DigitRenderer::drawColon(float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    float size = 0.1f * scale;
    float h = 1.0f * scale;
    drawSegment(x, y + 0.6f * h, size, size, color, shader, parentModel);
    drawSegment(x, y + 0.3f * h, size, size, color, shader, parentModel);
}

void DigitRenderer::drawPercent(float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    float size = 0.1f * scale;
    float h = 1.0f * scale;
    float w = 0.5f * scale;
    drawSegment(x, y + 0.2f * h, size, size, color, shader, parentModel);
    drawSegment(x + w, y + 0.8f * h, size, size, color, shader, parentModel);
}

void DigitRenderer::drawChar(char c, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    float t = 0.1f * scale;
    float w = 0.5f * scale;
    float h = 1.0f * scale;
    float sh = h / 2.0f;   

    // 7-segment: 0=top, 1=top-right, 2=bottom-right, 3=bottom, 4=bottom-left, 5=top-left, 6=middle
    bool s[7] = {false};

    char cu = (c >= 'a' && c <= 'z') ? (c - 32) : c;

    switch(cu) {
        case 'A': s[0]=1; s[1]=1; s[2]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'B': s[2]=1; s[3]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'C': s[0]=1; s[3]=1; s[4]=1; s[5]=1; break;
        case 'D': s[1]=1; s[2]=1; s[3]=1; s[4]=1; s[6]=1; break;
        case 'E': s[0]=1; s[3]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'F': s[0]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'G': s[0]=1; s[2]=1; s[3]=1; s[4]=1; s[5]=1; break;
        case 'H': s[1]=1; s[2]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'I': s[1]=1; s[2]=1; break;
        case 'J': s[1]=1; s[2]=1; s[3]=1; s[4]=1; break;
        case 'K': s[1]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'L': s[3]=1; s[4]=1; s[5]=1; break;
        case 'M': s[0]=1; s[1]=1; s[2]=1; s[4]=1; s[5]=1; break;
        case 'N': s[2]=1; s[4]=1; s[6]=1; break;
        case 'O': s[0]=1; s[1]=1; s[2]=1; s[3]=1; s[4]=1; s[5]=1; break;
        case 'P': s[0]=1; s[1]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'Q': s[0]=1; s[1]=1; s[2]=1; s[5]=1; s[6]=1; break;
        case 'R': s[4]=1; s[6]=1; break;
        case 'S': s[0]=1; s[2]=1; s[3]=1; s[5]=1; s[6]=1; break;
        case 'T': s[3]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'U': s[1]=1; s[2]=1; s[3]=1; s[4]=1; s[5]=1; break;
        case 'V': s[2]=1; s[3]=1; s[4]=1; break;
        case 'W': s[1]=1; s[2]=1; s[3]=1; s[4]=1; s[5]=1; break;
        case 'X': s[1]=1; s[2]=1; s[4]=1; s[5]=1; s[6]=1; break;
        case 'Y': s[1]=1; s[2]=1; s[3]=1; s[5]=1; s[6]=1; break;
        case 'Z': s[0]=1; s[1]=1; s[3]=1; s[4]=1; s[6]=1; break;
        case '-': s[6]=1; break;
        case '_': s[3]=1; break;
        case ' ': break;
        case ':':
            drawColon(x, y, scale, color, shader, parentModel);
            return;
        default:
            if (c >= '0' && c <= '9') {
                drawDigit(c - '0', x, y, scale, color, shader, parentModel);
                return;
            }
            break;
    }

    if(s[0]) drawSegment(x, y + h - t, w, t, color, shader, parentModel);
    if(s[1]) drawSegment(x + w - t, y + sh, t, sh, color, shader, parentModel);
    if(s[2]) drawSegment(x + w - t, y, t, sh, color, shader, parentModel);
    if(s[3]) drawSegment(x, y, w, t, color, shader, parentModel);
    if(s[4]) drawSegment(x, y, t, sh, color, shader, parentModel);
    if(s[5]) drawSegment(x, y + sh, t, sh, color, shader, parentModel);
    if(s[6]) drawSegment(x, y + sh - t/2.0f, w, t, color, shader, parentModel);
}

void DigitRenderer::drawText(const char* text, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel) {
    float spacing = 0.6f * scale;
    float startX = x;

    while (*text) {
        if (*text == '\n') {
            y -= 1.2f * scale;
            x = startX;
        } else {
            drawChar(*text, x, y, scale, color, shader, parentModel);
            x += spacing;
        }
        text++;
    }
}
