#include "pch.h"
#include "Camera.h"

Camera::Camera()
{
    Rm = { 1.0f };
    LeftAlt_on = { false };
    first_click = { false };
    first_cur_x = { 0.0f };
    first_cur_y = { 0.0f };
    camera_horizontal_angle = { 0.0f };
    camera_vertical_angle = { 0.0f };
    start_pos = { 30.0f };
    finish_pos = { 0.0f };

}

void Camera::Update()
{
    if (start)
    {
        Starting();
    }
    
    HandleMouseMovement(cur_x, cur_y);
    SetAngle();
}

void Camera::HandleAltKey(bool pressed) {
    LeftAlt_on = pressed;
    if (pressed) {
        first_click = true;
    }
}

void Camera::HandleMouseMovement(double cur_x, double cur_y) {
    if (!FirstPersonView)
    {
        if (LeftAlt_on) {
            if (first_click) {
                first_cur_x = cur_x;
                first_cur_y = cur_y;
                camera_horizontal_angle = 0.0f;
                camera_vertical_angle = atan2(10.0f, 5.0f);
                first_click = false;
            }
            else {
                float delta_x = -(cur_x - first_cur_x) * 0.0025f;
                float delta_y = (cur_y - first_cur_y) * 0.0025f;
                camera_horizontal_angle += delta_x;
                camera_vertical_angle = glm::clamp(
                    camera_vertical_angle + delta_y,
                    -(PI / 314.0f) + 0.02f,
                    (PI / 2.0f) - 0.01f
                );
                first_cur_x = cur_x;
                first_cur_y = cur_y;
            }
        }
        else
        {
            first_cur_x = cur_x;
            first_cur_y = cur_y;
            camera_horizontal_angle = 0.0f;
            camera_vertical_angle = atan2(10.0f, 5.0f);
            first_click = false;
        }
    }
    else
    {
        float delta_x = -(cur_x - first_cur_x) * 0.001f;
        float delta_y = (cur_y - first_cur_y) * 0.001f;

        camera_horizontal_angle += delta_x;
        camera_vertical_angle = glm::clamp(
            camera_vertical_angle + delta_y,
            -PI / 3.0f,
            PI / 3.0f
        );

        first_cur_x = cur_x;
        first_cur_y = cur_y;
    }
}

void Camera::HandleScroll(double yoffset) 
{
    if (!FirstPersonView /*&& !finish*/)
    {
        if (yoffset == -1) 
        {
            if (Rm < 1.0f)
            {
                Rm += 0.05f;
            }
        }
        else if (yoffset == 1) 
        {
            if (Rm > 0.5f)
            {
                Rm -= 0.05f;
            }
        }
    }
}

void Camera::Starting()
{
    if (start_pos > 0.0f)
        start_pos -= 0.1f;
    else if (start_pos < 0.0f)
    {
        start_pos = 0.0f;

        //startbgm->setIsPaused(true);
        //basebgm->setIsPaused(false);
        //cloud_go = true;
    }
}

glm::mat4 Camera::Get1stPersonViewMatrix(const glm::vec3& targetPos) {
    const float EYE_HEIGHT = 0.45f * Rm;
    glm::vec3 look_direction = glm::vec3(
        sin(camera_horizontal_angle) * cos(camera_vertical_angle),
        -sin(camera_vertical_angle),
        cos(camera_horizontal_angle) * cos(camera_vertical_angle)
    );

    glm::vec3 forward_offset = look_direction * 0.5f;
    glm::vec3 eye_pos = glm::vec3(
        targetPos.x + forward_offset.x,
        targetPos.y + EYE_HEIGHT,
        targetPos.z + forward_offset.z
    );

    return glm::lookAt(
        eye_pos,
        eye_pos + look_direction,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

glm::mat4 Camera::Get3rdPersonViewMatrix(const glm::vec3& targetPos) {
    if (LeftAlt_on) {
        float radius = sqrt(10.0f * 10.0f + 5.0f * 5.0f) * Rm;
        float height_offset = radius * sin(camera_vertical_angle);
        float radius_xz = radius * cos(camera_vertical_angle);

        glm::vec3 camera_pos = glm::vec3(
            targetPos.x + radius_xz * sin(camera_horizontal_angle),
            targetPos.y + height_offset,
            targetPos.z + radius_xz * cos(camera_horizontal_angle)
        );

        return glm::lookAt(
            camera_pos,
            targetPos,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    else {
        //if (!finish)
        //{
            return glm::lookAt(
                glm::vec3(targetPos.x + ((targetPos.x * 0.6f) * (start_pos / 30.0f)), 10.0f * Rm + (5.5f * (start_pos / 30.0f)), targetPos.z + 5.0f * Rm + ((targetPos.z * 0.35f) * (start_pos / 30.0f))),
                glm::vec3(targetPos.x - (targetPos.x * (start_pos / 30.0f)), 0.0f + (3.5f * (start_pos / 30.0f)), targetPos.z - (targetPos.z * (start_pos / 30.0f))),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
        //}
        //else
            //return glm::lookAt(
            //    glm::vec3(-44.0f - (finish_pos / 2.0f), 10.0f * Rm + (finish_pos / 4.0f), -50.5f + 5.0f * Rm - (finish_pos / 2.0f)),
            //    glm::vec3(-44.0f + (44.0f * (finish_pos / 30.0f)), 0.0f + (finish_pos / 30.0f), -50.5f + (50.5f * (finish_pos / 30.0f))),
            //    glm::vec3(0.0f, 1.0f, 0.0f)
            //);
    }
}

glm::vec3 Camera::GetMouseWorldDirection(float cur_x, float cur_y, const glm::mat4& projection,
    const glm::mat4& view, const glm::vec3& targetPos) {
    if (!FirstPersonView)
    {
        float x = (2.0f * cur_x) / WIN_W - 1.0f;
        float y = 1.0f - (2.0f * cur_y) / WIN_H;
        glm::vec4 screenPos = glm::vec4(x, y, 1.0f, 0.0f);
        glm::mat4 invProjView = glm::inverse(projection * view);
        glm::vec4 worldPos = invProjView * screenPos;
        worldPos /= worldPos.w;
        glm::vec3 cameraPos = glm::vec3(targetPos.x, 10.0f * Rm, targetPos.z + 5.0f * Rm);
        return glm::normalize(glm::vec3(worldPos) - cameraPos);
    }
    else
    {
        return glm::vec3(
            sin(camera_horizontal_angle) * cos(camera_vertical_angle),
            -sin(camera_vertical_angle),
            cos(camera_horizontal_angle) * cos(camera_vertical_angle));
    }
}

glm::vec3 Camera::GetMousePicking(float mouseX, float mouseY,
    const glm::mat4& projection, const glm::mat4& view)
{
    float x = (2.0f * mouseX) / WIN_W - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / WIN_H;

    glm::vec4 rayStart_NDC(x, y, -1.0f, 1.0f);
    glm::vec4 rayEnd_NDC(x, y, 1.0f, 1.0f);

    glm::mat4 invProjView = glm::inverse(projection * view);
    glm::vec4 rayStart_world = invProjView * rayStart_NDC;
    glm::vec4 rayEnd_world = invProjView * rayEnd_NDC;

    rayStart_world /= rayStart_world.w;
    rayEnd_world /= rayEnd_world.w;

    glm::vec3 rayOrigin = glm::vec3(rayStart_world);
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayEnd_world - rayStart_world));

    float t = (0.45f - rayOrigin.y) / rayDir.y;     // 3ÀÎÄª¿¡¼­ ÃÑ¾Ë ³ôÀÌ°¡ 0.45f
    glm::vec3 planeIntersection = rayOrigin + rayDir * t;

    return planeIntersection;
}

//void Camera::addfinishpos()
//{
//    if (finish_pos < 30.0f)
//        finish_pos += 0.01f;
//
//    if (finish_pos > 30.0f)
//        finish_pos = 30.0f;
//}

void Camera::SetInitialDirection(const glm::vec3& direction)
{
    camera_horizontal_angle = (float)PI + atan2(direction.x, direction.z);
    camera_vertical_angle = 0.0f;
}

glm::vec3 Camera::SetMouseWorldDirection(float cur_x, float cur_y, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& targetPos)
{
    return GetMouseWorldDirection(cur_x, cur_y, projection, view, targetPos);
}

glm::vec3 Camera::GetPosition(const glm::vec3& targetPos) {
    if (FirstPersonView) {
        const float EYE_HEIGHT = 0.7f * Rm;
        glm::vec3 look_direction = glm::vec3(
            sin(camera_horizontal_angle) * cos(camera_vertical_angle),
            -sin(camera_vertical_angle),
            cos(camera_horizontal_angle) * cos(camera_vertical_angle)
        );
        glm::vec3 forward_offset = look_direction * 0.5f;
        return glm::vec3(
            targetPos.x + forward_offset.x,
            targetPos.y + EYE_HEIGHT,
            targetPos.z + forward_offset.z
        );
    }
    else {
        if (LeftAlt_on) {
            float radius = sqrt(10.0f * 10.0f + 5.0f * 5.0f) * Rm;
            float height_offset = radius * sin(camera_vertical_angle);
            float radius_xz = radius * cos(camera_vertical_angle);
            return glm::vec3(
                targetPos.x + radius_xz * sin(camera_horizontal_angle),
                targetPos.y + height_offset,
                targetPos.z + radius_xz * cos(camera_horizontal_angle)
            );
        }
        else {
            return glm::vec3(targetPos.x, 10.0f * Rm, targetPos.z + 5.0f * Rm);
        }
    }
}

glm::mat4 Camera::GetViewMatrix(const glm::vec3& targetPos)
{
    glm::mat4 view = glm::mat4(1.0f);

    if (FirstPersonView)
    {
        view = Get1stPersonViewMatrix(targetPos);
	}
	else
	{
		view = Get3rdPersonViewMatrix(targetPos);
	}

	return view;
}

void Camera::SetAngle()
{
    if (!FirstPersonView) {
        angle = (float)PI + atan2(mouseDir.x, mouseDir.z);
    }
    else {
        angle = GetHorizontalAngle();
    }
}
