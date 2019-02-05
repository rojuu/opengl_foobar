#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    uint id;
    std::string type;
    std::string path;
};

struct Mesh {
    uint VAO;
    std::vector<Vertex> vertices;
    std::vector<uint> indices;
    std::vector<Texture> textures;
};

struct Model {
    std::vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;
};

static Mesh
setupMesh(std::vector<Vertex> vertices, std::vector<uint> indices, std::vector<Texture> textures) {
    Mesh mesh = {};
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.textures = textures;

    uint VBO, EBO;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);

    return mesh;
}

void drawMesh(Mesh* mesh, Shader shader) {
    uint diffuseNr  = 1;
    uint specularNr = 1;
    uint normalNr   = 1;
    uint heightNr   = 1;

    use(shader);
    for(uint i = 0; i < mesh->textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string number;
        std::string name = mesh->textures[i].type;
        if(name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if(name == "texture_specular")
            number = std::to_string(specularNr++);
        else if(name == "texture_normal")
            number = std::to_string(normalNr++);
            else if(name == "texture_height")
            number = std::to_string(heightNr++);

        setInt(shader, (name + number).c_str(), i);
        glBindTexture(GL_TEXTURE_2D, mesh->textures[i].id);
    }

    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}

static void
drawModel(Model* model, Shader shader) {
    for(int i = 0; i < model->meshes.size(); i++) {
        drawMesh(&model->meshes[i], shader);
    }
}

static uint
textureFromFile(const char *path, std::string directory, bool gamma) {
    std::string filename = directory + '/' + std::string(path);

    uint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

static std::vector<Texture>
loadMaterialTextures(Model* model, aiMaterial *mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    for(int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for(int j = 0; j < model->textures_loaded.size(); j++) {
            if(std::strcmp(model->textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(model->textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if(!skip) {
            Texture texture;
            texture.id = textureFromFile(str.C_Str(), model->directory, model->gammaCorrection);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            model->textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
        }
    }
    return textures;
}

static Mesh
processMesh(Model* model, aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<uint> indices;
    std::vector<Texture> textures;

    for(int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex = {};
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;

        if(mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }

        if(mesh->mTangents) {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
        }

        if(mesh->mBitangents) {
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        vertices.push_back(vertex);
    }

    for(int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture> diffuseMaps = loadMaterialTextures(model, material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<Texture> specularMaps = loadMaterialTextures(model, material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<Texture> normalMaps = loadMaterialTextures(model, material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    std::vector<Texture> heightMaps = loadMaterialTextures(model, material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    return setupMesh(vertices, indices, textures);
}

static void
processNode(Model* model, aiNode *node, const aiScene *scene) {
    for(int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model->meshes.push_back(processMesh(model, mesh, scene));
    }

    for(int i = 0; i < node->mNumChildren; i++) {
        processNode(model, node->mChildren[i], scene);
    }

}

static Model
loadModel(std::string path, bool gammaCorrection = false) {
    Model model = {};
    model.gammaCorrection = gammaCorrection;

    Assimp::Importer importer;
    // Scene is freed during Importer's destructor
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return model;
    }

    model.directory = path.substr(0, path.find_last_of('/'));

    processNode(&model, scene->mRootNode, scene);

    return model;
}

