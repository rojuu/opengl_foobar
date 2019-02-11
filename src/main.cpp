#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "imgui/imgui_demo.cpp"

#include "ImGuizmo/ImGuizmo.h"

//TODO: Maybe don't use stl as much. Just faster to setup for now.
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

#define arrayCount(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

typedef unsigned int uint;

#include "shader.cpp"
#include "model_loading.cpp"

struct RenderContext {
    GLFWwindow* window;
    uint width;
    uint height;
};

static RenderContext g_renderContext;

#include "camera.cpp"

struct Entity {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Model* model;
    Shader shader;
};

struct Sphere {
    glm::vec3 c;
    float r;
};

static int selectedEntity = 0;
static std::vector<Entity> entities;

static bool
intersectRaySphere(glm::vec3 p, glm::vec3 d, Sphere sphere)
{
    glm::vec3 m = p - sphere.c;
    float b = glm::dot(m, d);
    float c = glm::dot(m, m) - sphere.r * sphere.r;

    // Exit if r’s origin outside sphere (c > 0) and r pointing away from sphere (b > 0)
    if (c > 0.0f && b > 0.0f) return false;
    float discr = b*b - c;

    // A negative discriminant corresponds to ray missing sphere
    if (discr < 0.0f) return false;

    return true;
}

static inline void
resizeView(RenderContext* renderContext, uint width, uint height) {
    renderContext->width = width;
    renderContext->height = height;
    glViewport(0, 0, width, height);
}

static void
windowSizeCallback(GLFWwindow* window, int width, int height) {
    resizeView(&g_renderContext, (uint)width, (uint)height);
}

static float lastMouseX;
static float lastMouseY;
static bool firstMouse = true;

static Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

static bool cameraMousePressed = false;

static void
mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastMouseX;
    float yoffset = lastMouseY - ypos; // reversed since y-coordinates go from bottom to top

    lastMouseX = xpos;
    lastMouseY = ypos;

    if(!cameraMousePressed) return;
    camera.processMouseMovement(xoffset, yoffset);
}

static float entityPickerSize = 0.1f;

static glm::vec3 testIndicatorPos;

static void
mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        cameraMousePressed = true;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        cameraMousePressed = false;

    // Shoot ray from mouse pos trough camera and see what we hit.
    // If we hit an entity, select it.
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        glm::vec4 pos = glm::vec4(lastMouseX, lastMouseY, 0.f, 0.f);

        glm::mat4 matProjection = camera.getProjectionMatrix() * camera.getViewMatrix();
        glm::mat4 matInverse = glm::inverse(matProjection);

        pos.x=(2.0f*((float)pos.x/(float)g_renderContext.width))-1.0f,
        pos.y=1.0f-(2.0f*((float)pos.y/(float)g_renderContext.height));
        pos.z=-1;
        pos.w=1.0;

        pos = matInverse * pos;

        pos.w = 1.0 / pos.w;
        pos.x *= pos.w;
        pos.y *= pos.w;
        pos.z *= pos.w;

        glm::vec3 rayDir = glm::normalize(glm::vec3(pos.x, pos.y, pos.z) - camera.Position);
        glm::vec3 rayPos = camera.Position;

        int entityIndex = -1;
        float smallestDistance = INFINITY;
        for(int i = 0; i < entities.size(); i++) {
            auto* entity = &entities[i];
            Sphere sphere;
            sphere.c = entity->position;
            sphere.r = entityPickerSize + entityPickerSize*0.1f;
            bool result = intersectRaySphere(rayPos, rayDir, sphere);
            if(result) {
                float distance = glm::distance(camera.Position, entity->position);
                if(distance < smallestDistance) {
                    entityIndex = i;
                }
            }
        }

        if(entityIndex >= 0) {
            selectedEntity = entityIndex;
        }
    }
}

static void
scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(yoffset);
}

static void
drawEntity(Entity* entity) {
    use(entity->shader);
    setMat4(entity->shader, "projection", camera.getProjectionMatrix());
    setMat4(entity->shader, "view", camera.getViewMatrix());

    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, entity->position);
    modelMat = glm::scale(modelMat, glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z));
    modelMat = modelMat * glm::yawPitchRoll(glm::radians(entity->rotation.y), glm::radians(entity->rotation.x), glm::radians(entity->rotation.z));

    setMat4(entity->shader, "model", modelMat);

    drawModel(entity->model, entity->shader);
}

static void
editTransform(Camera* camera, glm::mat4& matrix) {
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents((float*)glm::value_ptr(matrix), matrixTranslation, matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation, 3);
    ImGui::InputFloat3("Rt", matrixRotation, 0.1f);
    ImGui::InputFloat3("Sc", matrixScale, 3);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, (float*)glm::value_ptr(matrix));

    /*if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
            mCurrentGizmoMode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
            mCurrentGizmoMode = ImGuizmo::WORLD;
    }*/
    ImGui::SameLine();
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate((float*)glm::value_ptr(camera->getViewMatrix()), (float*)glm::value_ptr(camera->getProjectionMatrix()), mCurrentGizmoOperation, mCurrentGizmoMode, (float*)glm::value_ptr(matrix));
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_renderContext.width = 1280;
    g_renderContext.height = 720;

    GLFWwindow* window = glfwCreateWindow(g_renderContext.width, g_renderContext.height, "opengl_foobar", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    g_renderContext.window = window;

    glfwSetWindowSizeCallback(window, windowSizeCallback);

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glewExperimental = GL_TRUE;
    glewInit();

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Shader basicShader = compileShader("basic.vs", "basic.fs");
    Shader greenShader = compileShader("basic.vs", "green.fs");
    Shader redShader   = compileShader("basic.vs", "red.fs");

    Model nanosuitModel = loadModel("data/nanosuit/nanosuit.obj");
    Model sphereModel = loadModel("data/sphere/sphere.obj");

    Entity greenIndicator;
    greenIndicator.scale = glm::vec3(entityPickerSize);
    greenIndicator.model = &sphereModel;
    greenIndicator.shader = greenShader;

    Entity testIndicator;
    testIndicator.scale = glm::vec3(0.05f);
    testIndicator.model = &sphereModel;
    testIndicator.shader = redShader;

    glm::vec3 clearColor = glm::vec3(0.2f, 0.3f, 0.3f);

    float deltaTime = 0.f;
    float lastFrame = glfwGetTime();

    bool running = true;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.processKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.processKeyboard(DOWN, deltaTime);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        bool entityEditorOpen = ImGui::Begin("Entity editor");
        if(entityEditorOpen && entities.size() > 0)
        {
            if(selectedEntity < 0) selectedEntity = 0;
            if(selectedEntity >= entities.size()) selectedEntity = entities.size() -1;

            auto* entity = &entities[selectedEntity];
            ImGui::Text("Selected entity: %i", selectedEntity);
            glm::mat4 matrix;
            ImGuizmo::RecomposeMatrixFromComponents((float*)&entity->position, (float*)&entity->rotation, (float*)&entity->scale, (float*)glm::value_ptr(matrix));
            editTransform(&camera, matrix);
            ImGuizmo::DecomposeMatrixToComponents((float*)glm::value_ptr(matrix), (float*)&entity->position, (float*)&entity->rotation, (float*)&entity->scale);
        }
        ImGui::Separator();
        if(ImGui::Button("Add nanosuit")) {
            Entity entity = {};
            entity.scale = glm::vec3(0.3f);
            entity.model = &nanosuitModel;
            entity.shader = basicShader;
            entities.push_back(entity);
        }
        ImGui::End();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        glm::mat4 projection = camera.getProjectionMatrix();
        glm::mat4 view = camera.getViewMatrix();

        for(int i = 0; i < entities.size(); i++) {
            auto* entity = &entities[i];
            drawEntity(entity);
        }

        if(entityEditorOpen) {
            for(int i = 0; i < entities.size(); i++) {
                auto* entity = &entities[i];
                glDisable(GL_DEPTH_TEST);
                greenIndicator.position = entity->position;
                drawEntity(&greenIndicator);
                glEnable(GL_DEPTH_TEST);
            }
        }

#if 0
        testIndicator.position = testIndicatorPos;
        glDisable(GL_DEPTH_TEST);
        drawEntity(&testIndicator);
        glEnable(GL_DEPTH_TEST);
#endif

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
