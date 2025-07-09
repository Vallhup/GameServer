#include "pch.h"
#include "BoundingBox.h"

BoundingBox::BoundingBox()
{
    SetupHitboxBuffers();
    SetupHitboxShaders("Shaders/HitboxVert.glsl", "Shaders/HitboxFrag.glsl");
}

BoundingBox::~BoundingBox()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(Hitboxsh);
}

void BoundingBox::RenderHitbox(float angle, const glm::vec3& pos, const glm::mat4& view, const glm::mat4& projection)
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, pos);  // 먼저 위치 이동   
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(Hitboxsh);

    glUniformMatrix4fv(glGetUniformLocation(Hitboxsh, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(Hitboxsh, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(Hitboxsh, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(Hitboxsh, "color"), 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 24);  // 12개의 선분 (24개 정점)
}

void BoundingBox::SetupHitboxBuffers()
{
    
    vector<float> vertices = {
        -0.25f, 0.95f, -0.2f,  0.25f, 0.95f, -0.2f,
         0.25f, 0.95f, -0.2f,  0.25f, 0.95f, 0.2f,
         0.25f, 0.95f,  0.2f, -0.25f, 0.95f, 0.2f,
        -0.25f, 0.95f,  0.2f, -0.25f, 0.95f, -0.2f,

        -0.25f, 0.0f, -0.2f,   0.25f, 0.0f, -0.2f,
         0.25f, 0.0f, -0.2f,   0.25f, 0.0f,  0.2f,
         0.25f, 0.0f,  0.2f,  -0.25f, 0.0f,  0.2f,
        -0.25f, 0.0f,  0.2f,  -0.25f, 0.0f, -0.2f,

        -0.25f, 0.95f, -0.2f, -0.25f,  0.0f, -0.2f,
         0.25f, 0.95f, -0.2f,  0.25f,  0.0f, -0.2f,
         0.25f, 0.95f,  0.2f,  0.25f,  0.0f,  0.2f,
        -0.25f, 0.95f,  0.2f, -0.25f,  0.0f,  0.2f,
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void BoundingBox::SetupHitboxShaders(const char* vertexName, const char* fragmentName)
{
    SetupShader(vertexName, fragmentName, Hitboxsh);
}
