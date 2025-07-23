#include "pch.h"
#include "Fade.h"

void Fade::Init()
{
	SetupShader("Shaders/FadeVert.glsl", "Shaders/FadeFrag.glsl", ShaderProgram);

	mvpLocation = glGetUniformLocation(ShaderProgram, "mvp");
	colorLocation = glGetUniformLocation(ShaderProgram, "color");

    CreateQuad();
}

void Fade::Update()
{

}

void Fade::Render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos, const glm::vec3& frontDir)
{
    if (fadeAlpha <= 0.0f) return;  // 투명하면 그리지 않음

    // 블렌딩 활성화
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);  // 항상 앞에 그리기

    glUseProgram(ShaderProgram);

    // 카메라 앞에 사각형 위치 계산
    glm::vec3 quadCenter = cameraPos + frontDir * 0.2f;

    // 모델 매트릭스 (위치 + 크기)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, quadCenter);
    model = glm::scale(model, glm::vec3(2.0f, 2.2f, 1.0f));  // 크기 조절

    glm::mat4 mvp = projection * view * model;

    // Uniform 설정
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);
    glUniform4f(colorLocation, 0.0f, 0.0f, 0.0f, fadeAlpha);  // 검은색

    // 렌더링
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // 상태 복원
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void Fade::Release()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);  // <- EBO도 해제해야 함
    if (ShaderProgram) glDeleteProgram(ShaderProgram);

    VAO = VBO = EBO = ShaderProgram = 0;
}

void Fade::CreateQuad()
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  
         0.5f, -0.5f, 0.0f,  
         0.5f,  0.5f, 0.0f,  
        -0.5f,  0.5f, 0.0f   
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
