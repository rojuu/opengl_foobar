#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.cpp"
#include "imgui/imgui_impl_opengl3.cpp"
#include "imgui/imgui_impl_glfw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_draw.cpp"

#include "ImGuizmo/ImGuizmo.cpp"

//TODO: Maybe don't use stl as much. Just faster to setup for now.
//TODO: Should make a temp stack allocator for all the temp allocs at least, like strings
//TODO: Get rid of std::string entirely and have custom one
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
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

enum Direction {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

#include "camera.cpp"

struct Entity {
    glm::mat4 modelMatrix;
    // glm::vec3 position;
    // glm::vec3 rotation;
    // glm::vec3 scale;

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
intersectRaySphere(glm::vec3 p, glm::vec3 d, Sphere sphere) {
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

static Camera g_camera;

static bool cameraMousePressed = false;

static void
mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;

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
    processMouseMovement(&g_camera, xoffset, yoffset);
}

static float entityPickerSize = 0.1f;

static glm::vec3 testIndicatorPos;

static void
setPos(glm::mat4* matrix, glm::vec3 pos) {
    (*matrix)[3][0] = pos.x;
    (*matrix)[3][1] = pos.y;
    (*matrix)[3][2] = pos.z;
}

static glm::vec3
getPos(glm::mat4& matrix) {
    glm::vec3 pos = glm::vec3(matrix[3]);
    return pos;
}

static glm::vec3
getScale(glm::mat4 matrix) {
    glm::vec3 scale = glm::vec3(
        glm::length(glm::vec4(matrix[0])),
        glm::length(glm::vec4(matrix[1])),
        glm::length(glm::vec4(matrix[2]))
    );
    return scale;
}

static int
findEntityUnderScreenPos(float mouseX, float mouseY) {
    // Raycast from screenpos and see if we hit an entity, return its index, if so

    glm::vec4 pos = glm::vec4(mouseX, mouseY, 0.f, 0.f);

    glm::mat4 matProjection = calculateProjectionMatrix(&g_camera) * calculateViewMatrix(&g_camera);
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

    glm::vec3 rayDir = glm::normalize(glm::vec3(pos.x, pos.y, pos.z) - g_camera.position);
    glm::vec3 rayPos = g_camera.position;

    int entityIndex = -1;
    float smallestDistance = INFINITY;
    for(int i = 0; i < entities.size(); i++) {
        auto* entity = &entities[i];
        glm::vec3 entityPos = getPos(entity->modelMatrix);
        Sphere sphere;
        sphere.c = entityPos;
        sphere.r = entityPickerSize + entityPickerSize*0.1f;
        bool result = intersectRaySphere(rayPos, rayDir, sphere);
        if(result) {
            float distance = glm::distance(g_camera.position, entityPos);
            if(distance < smallestDistance) {
                entityIndex = i;
            }
        }
    }

    return entityIndex;
}

static void
mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        cameraMousePressed = true;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        cameraMousePressed = false;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        int entityIndex = findEntityUnderScreenPos(lastMouseX, lastMouseY);
        if(entityIndex >= 0) {
            selectedEntity = entityIndex;
        }
    }
}

static void
scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    processMouseScroll(&g_camera, yoffset);
}

static void
drawEntity(Entity* entity) {
    use(entity->shader);
    setMat4(entity->shader, "projection", calculateProjectionMatrix(&g_camera));
    setMat4(entity->shader, "view", calculateViewMatrix(&g_camera));

    // glm::mat4 modelMat = glm::mat4(1.0f);
    // modelMat = glm::translate(modelMat, entity->position);
    // modelMat = glm::scale(modelMat, glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z));
    // modelMat = glm::rotate(modelMat, glm::radians(entity->rotation.x), glm::vec3(1.f, 0.f, 0.f));
    // modelMat = glm::rotate(modelMat, glm::radians(entity->rotation.y), glm::vec3(0.f, 1.f, 0.f));
    // modelMat = glm::rotate(modelMat, glm::radians(entity->rotation.z), glm::vec3(0.f, 0.f, 1.f));

    setMat4(entity->shader, "model", entity->modelMatrix);

    drawModel(entity->model, entity->shader);
}

static void
editTransform(GLFWwindow* window, Camera* camera, glm::mat4& matrix) {
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        mCurrentGizmoOperation = ImGuizmo::SCALE;

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
    ImGui::InputFloat3("Rt", matrixRotation, 3);
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
    ImGuizmo::Manipulate((float*)glm::value_ptr(calculateViewMatrix(camera)), (float*)glm::value_ptr(calculateProjectionMatrix(camera)), mCurrentGizmoOperation, mCurrentGizmoMode, (float*)glm::value_ptr(matrix));
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    g_camera = constructCamera(0.0f, 8.0f, 15.0, 0, 1, 0, -90.f, -25.f);

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Shader basicShader = compileShader("basic.vs", "basic.fs");
    Shader greenShader = compileShader("basic.vs", "green.fs");
    Shader redShader   = compileShader("basic.vs", "red.fs");

    Model nanosuitModel = loadModel("data/nanosuit/nanosuit.obj");
    Model sphereModel = loadModel("data/sphere/sphere.obj");

    Entity greenIndicator;
    greenIndicator.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(entityPickerSize));
    greenIndicator.model = &sphereModel;
    greenIndicator.shader = greenShader;

    glm::vec3 clearColor = glm::vec3(0.2f, 0.3f, 0.3f);

    float deltaTime = 0.f;
    float lastFrame = glfwGetTime();

    bool hideAllDebugMenusPressed = false;
    bool hideAllDebugMenus = false;
    bool running = true;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            processKeyboard(&g_camera, FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            processKeyboard(&g_camera, BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            processKeyboard(&g_camera, LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            processKeyboard(&g_camera, RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            processKeyboard(&g_camera, UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            processKeyboard(&g_camera, DOWN, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !hideAllDebugMenusPressed) {
            hideAllDebugMenus = !hideAllDebugMenus;
            hideAllDebugMenusPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
            hideAllDebugMenusPressed = false;


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        bool entityEditorOpen;
        if(!hideAllDebugMenus)
        {
            entityEditorOpen = ImGui::Begin("Entity editor");
            if(entityEditorOpen && entities.size() > 0)
            {
                if(selectedEntity < 0) selectedEntity = 0;
                if(selectedEntity >= entities.size()) selectedEntity = entities.size() -1;

                auto* entity = &entities[selectedEntity];
                ImGui::Text("Selected entity: %i", selectedEntity);
                editTransform(window, &g_camera, entity->modelMatrix);
            }
            ImGui::End();

            ImGui::Begin("Entity spawner");
            if(ImGui::Button("Add nanosuit at camera")) {
                Entity entity = {};
                glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));;
                setPos(&mat, g_camera.front * 10.f + g_camera.position);
                entity.modelMatrix = mat;
                entity.model = &nanosuitModel;
                entity.shader = basicShader;
                entities.push_back(entity);
            }
            ImGui::End();
        }

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = calculateProjectionMatrix(&g_camera);
        glm::mat4 view = calculateViewMatrix(&g_camera);

        for(int i = 0; i < entities.size(); i++) {
            auto* entity = &entities[i];
            drawEntity(entity);
        }

        if(!hideAllDebugMenus && entityEditorOpen) {
            for(int i = 0; i < entities.size(); i++) {
                auto* entity = &entities[i];
                 glDisable(GL_DEPTH_TEST);
                setPos(&greenIndicator.modelMatrix, getPos(entity->modelMatrix));
                drawEntity(&greenIndicator);
                glEnable(GL_DEPTH_TEST);
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
