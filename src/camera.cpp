typedef Direction CameraMovement;

struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float fov;

    static const float NearPlane;
    static const float FarPlane;
};

const float Camera::NearPlane = 0.1f;
const float Camera::FarPlane  = 100.0f;

static void updateCameraVectors(Camera* camera);

static Camera
constructCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
{
    Camera camera = {};

    camera.movementSpeed = 10.f;
    camera.mouseSensitivity = 0.1f;
    camera.fov = 45.0f;

    camera.position = glm::vec3(posX, posY, posZ);
    camera.worldUp = glm::vec3(upX, upY, upZ);
    camera.yaw = yaw;
    camera.pitch = pitch;

    updateCameraVectors(&camera);

    return camera;
}

static void
updateCameraVectors(Camera* camera) {
    glm::vec3 front;
    front.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    front.y = sin(glm::radians(camera->pitch));
    front.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    camera->front = glm::normalize(front);

    camera->right = glm::normalize(glm::cross(camera->front, camera->worldUp));
    camera->up    = glm::normalize(glm::cross(camera->right, camera->front));
}

static glm::mat4
calculateProjectionMatrix(Camera* camera) {
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera->fov), (float)g_renderContext.width / (float)g_renderContext.height, Camera::NearPlane, Camera::FarPlane);
    return projectionMatrix;
}

static glm::mat4
calculateViewMatrix(Camera* camera) {
    glm::mat4 viewMatrix = glm::lookAt(camera->position, camera->position + camera->front, camera->up);
    return viewMatrix;
}

static void
processKeyboard(Camera* camera, CameraMovement direction, float deltaTime) {
    float velocity = camera->movementSpeed * deltaTime;
    if (direction == FORWARD)
        camera->position += camera->front * velocity;
    if (direction == BACKWARD)
        camera->position -= camera->front * velocity;
    if (direction == LEFT)
        camera->position -= camera->right * velocity;
    if (direction == RIGHT)
        camera->position += camera->right * velocity;

    glm::vec3 up = glm::cross(camera->front, -camera->right);
    if (direction == UP)
        camera->position += up * velocity;
    if (direction == DOWN)
        camera->position += -up * velocity;
}

static void
processMouseMovement(Camera* camera, float xoffset, float yoffset, GLboolean constrainPitch = true) {
    xoffset *= camera->mouseSensitivity;
    yoffset *= camera->mouseSensitivity;

    camera->yaw  += xoffset;
    camera->pitch += yoffset;

    if (constrainPitch) {
        if (camera->pitch > 89.0f)
            camera->pitch = 89.0f;
        if (camera->pitch < -89.0f)
            camera->pitch = -89.0f;
    }

    updateCameraVectors(camera);
}

static void
processMouseScroll(Camera* camera, float yoffset) {
    camera->fov -= yoffset;

    const float maxFov = 180.0f;
    const float minFov = 1.0f;
    if (camera->fov < minFov)
        camera->fov = minFov;
    if (camera->fov > maxFov)
        camera->fov = maxFov;
}

