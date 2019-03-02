struct Shader {
    uint ID;
};

static bool
checkShaderCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    return success;
}

static Shader
compileShader(std::string vertexPath, std::string fragmentPath, std::string geometryPath = "") {
    Shader result = {};

    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    //TODO: Get rid of exceptions
    try {
        const std::string shaderFolder = "data/shaders/";
        vShaderFile.open(shaderFolder + vertexPath);
        fShaderFile.open(shaderFolder + fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        if (!geometryPath.empty()) {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char * fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkShaderCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkShaderCompileErrors(fragment, "FRAGMENT");

    unsigned int geometry;
    if (!geometryPath.empty()) {
        const char * gShaderCode = geometryCode.c_str();
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        checkShaderCompileErrors(geometry, "GEOMETRY");
    }

    result.ID = glCreateProgram();
    glAttachShader(result.ID, vertex);
    glAttachShader(result.ID, fragment);
    if (!geometryPath.empty()) {
        glAttachShader(result.ID, geometry);
    }
    glLinkProgram(result.ID);
    checkShaderCompileErrors(result.ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (!geometryPath.empty()) {
        glDeleteShader(geometry);
    }

    return result;
}

static void
use(Shader shader) {
    glUseProgram(shader.ID);
}

static void
setBool(Shader shader, const std::string &name, bool value) {
    glUniform1i(glGetUniformLocation(shader.ID, name.c_str()), (int)value);
}

static void
setInt(Shader shader, const std::string &name, int value) {
    glUniform1i(glGetUniformLocation(shader.ID, name.c_str()), value);
}

static void
setFloat(Shader shader, const std::string &name, float value) {
    glUniform1f(glGetUniformLocation(shader.ID, name.c_str()), value);
}

static void
setVec2(Shader shader, const std::string &name, const glm::vec2 &value) {
    glUniform2fv(glGetUniformLocation(shader.ID, name.c_str()), 1, &value[0]);
}

static void
setVec2(Shader shader, const std::string &name, float x, float y) {
    glUniform2f(glGetUniformLocation(shader.ID, name.c_str()), x, y);
}

static void
setVec3(Shader shader, const std::string &name, const glm::vec3 &value) {
    glUniform3fv(glGetUniformLocation(shader.ID, name.c_str()), 1, &value[0]);
}

static void
setVec3(Shader shader, const std::string &name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(shader.ID, name.c_str()), x, y, z);
}

static void
setVec4(Shader shader, const std::string &name, const glm::vec4 &value) {
    glUniform4fv(glGetUniformLocation(shader.ID, name.c_str()), 1, &value[0]);
}

static void
setVec4(Shader shader, const std::string &name, float x, float y, float z, float w) {
    glUniform4f(glGetUniformLocation(shader.ID, name.c_str()), x, y, z, w);
}

static void
setMat2(Shader shader, const std::string &name, const glm::mat2 &mat) {
    glUniformMatrix2fv(glGetUniformLocation(shader.ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

static void
setMat3(Shader shader, const std::string &name, const glm::mat3 &mat) {
    glUniformMatrix3fv(glGetUniformLocation(shader.ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

static void
setMat4(Shader shader, const std::string &name, const glm::mat4 &mat) {
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
