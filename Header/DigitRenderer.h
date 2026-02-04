#pragma once
#include <vector>
#include <glm/glm.hpp>

class DigitRenderer {
public:
    DigitRenderer();
    ~DigitRenderer();

    void init();
    
    // Updated signature to include parentModel for hierarchical transformation
    void drawNumber(int number, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel = glm::mat4(1.0f));
    void drawTime(int h, int m, int s, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel = glm::mat4(1.0f));
    void drawColon(float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel = glm::mat4(1.0f));
    void drawPercent(float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel = glm::mat4(1.0f));
    void drawText(const char* text, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel = glm::mat4(1.0f));

private:
    unsigned int VAO, VBO;

    void drawDigit(int digit, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel);
    void drawChar(char c, float x, float y, float scale, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel);
    void drawSegment(float x, float y, float w, float h, const glm::vec3& color, unsigned int shader, const glm::mat4& parentModel);
};
