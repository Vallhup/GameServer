#pragma once

// vt
//class IA
//{
//public: 
//};
//
//class B : public IA
//{
//
//};
//
//class C : public IA
//{
//
//};

// vector<long long*> s = 

class Camera final {
public:
    Camera();

    void Update();
    void HandleAltKey(bool pressed);
    void HandleMouseMovement(double cur_x, double cur_y);
    void HandleScroll(double yoffset);
    void Starting();

    glm::mat4 Get1stPersonViewMatrix(const glm::vec3& targetPos);
    glm::mat4 Get3rdPersonViewMatrix(const glm::vec3& targetPos);
    glm::vec3 GetMouseWorldDirection(float cur_x, float cur_y, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& targetPos);

    bool IsAltPressed() const { return LeftAlt_on; }
    //float getZoomLevel() const { return Rm; }
    float GetHorizontalAngle() const { return camera_horizontal_angle; }
    float GetVerticalAngle() const { return camera_vertical_angle; }
    float Get_start_pos() const { return start_pos; }
    glm::vec3 GetMousePicking(float mouseX, float mouseY, const glm::mat4& projection, const glm::mat4& view);

    //void addfinishpos();
    void SetInitialDirection(const glm::vec3& direction);
    
    void ChangeViewType() { FirstPersonView = !FirstPersonView; }
	glm::vec3 SetMouseWorldDirection(float cur_x, float cur_y, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& targetPos);

	bool GetViewType() const { return FirstPersonView; }

    glm::vec3 GetPosition(const glm::vec3& targetPos);
	glm::mat4 GetViewMatrix(const glm::vec3& targetPos);

	float GetAngle() const { return angle; }

    void SetAngle();
	void SetStart(bool in) { start = in; }

private:
    // Camera camera;
    // LeftAlt_on

    // 멤버 변수는 헤더에 초기화하는 것을 추천.

    float Rm = { 1.0f };
    bool LeftAlt_on = { false };
    bool first_click = { false };
    double first_cur_x = { 0.0f };
    double first_cur_y = { 0.0f };
    float camera_horizontal_angle = { 0.0f };
    float camera_vertical_angle = { 0.0f };
    float start_pos = { 30.0f };
    float finish_pos = { 0.0f };

    bool FirstPersonView = { false };

	float angle = { 0.0f };
    float light_angle = { 0.0f };

    bool start{ false };
};