//TODO: Make a temp buffer for all these strings and don't use std::string, should make a generic temp stack allocator I guess
struct Shader{
    uint m_shaderProgram;
    Shader(std::string vertexPath, std::string fragmentPath, std::string geometryPath = ""){
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;

        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try{
            vShaderFile.open("data/shaders/" + vertexPath);
            fShaderFile.open("data/shaders/" + fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();

            if(!geometryPath.empty()){
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure e){
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char * fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        unsigned int geometry;
        if(!geometryPath.empty()){
            const char * gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }

        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertex);
        glAttachShader(m_shaderProgram, fragment);
        if(!geometryPath.empty()){
            glAttachShader(m_shaderProgram, geometry);
        }
        glLinkProgram(m_shaderProgram);
        checkCompileErrors(m_shaderProgram, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if(!geometryPath.empty()){
            glDeleteShader(geometry);
        }
    }

    void use(){
        glUseProgram(m_shaderProgram);
    }

    void setBool(const std::string &name, bool value) const{
        glUniform1i(glGetUniformLocation(m_shaderProgram, name.c_str()), (int)value);
    }

    void setInt(const std::string &name, int value) const{
        glUniform1i(glGetUniformLocation(m_shaderProgram, name.c_str()), value);
    }

    void setFloat(const std::string &name, float value) const{
        glUniform1f(glGetUniformLocation(m_shaderProgram, name.c_str()), value);
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const{
        glUniform2fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, &value[0]);
    }

    void setVec2(const std::string &name, float x, float y) const{
        glUniform2f(glGetUniformLocation(m_shaderProgram, name.c_str()), x, y);
    }

    void setVec3(const std::string &name, const glm::vec3 &value) const{
        glUniform3fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, &value[0]);
    }

    void setVec3(const std::string &name, float x, float y, float z) const{
        glUniform3f(glGetUniformLocation(m_shaderProgram, name.c_str()), x, y, z);
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const{
        glUniform4fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string &name, float x, float y, float z, float w){
        glUniform4f(glGetUniformLocation(m_shaderProgram, name.c_str()), x, y, z, w);
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const{
        glUniformMatrix2fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const{
        glUniformMatrix3fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const{
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void checkCompileErrors(GLuint shader, std::string type){
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM"){
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success){
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }else{
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success){
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};