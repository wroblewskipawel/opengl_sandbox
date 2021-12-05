#include "graphics/gl/block/shared.h"

namespace gl {

namespace shared {

void Block::queryUniformLayouts(GLuint glProgram) {
    GLint blockCount{};
    GLchar glNameBuffer[MAX_GL_NAME_LENGTH]{};

    glGetProgramInterfaceiv(glProgram, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES,
                            &blockCount);

    for (GLint index{0}; index < blockCount; index++) {
        glGetActiveUniformBlockName(glProgram, index, 512, NULL, glNameBuffer);
        std::string blockName{glNameBuffer};

        auto info = blockLayouts.find(blockName);
        if (info == blockLayouts.end()) {
            GLint byteSize{};
            GLint uniformCount{};

            glGetActiveUniformBlockiv(glProgram, index,
                                      GL_UNIFORM_BLOCK_DATA_SIZE, &byteSize);
            glGetActiveUniformBlockiv(glProgram, index,
                                      GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
                                      &uniformCount);
            std::vector<GLuint> uniformIndices(uniformCount);
            glGetActiveUniformBlockiv(
                glProgram, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                reinterpret_cast<GLint*>(uniformIndices.data()));

            auto uniformInfo =
                getUniformInfo(glProgram, blockName, uniformIndices);

            BlockInfo blockInfo{};
            blockInfo.byteSize = byteSize;
            blockInfo.uniforms = std::move(uniformInfo);
            const auto& [item, _] =
                blockLayouts.insert({std::move(blockName), blockInfo});
        }
    }
}

std::unordered_map<std::string, Block::UniformInfo> Block::getUniformInfo(
    GLuint glProgram, const std::string& blockName,
    const std::vector<GLuint>& uniformIndices) {
    size_t uniformCount{uniformIndices.size()};
    std::vector<GLint> uniformParams(uniformCount * 5);
    std::vector<UniformInfo> uniformInfos(uniformCount);

    auto* offsets = &uniformParams[0 * uniformCount];
    auto* types = &uniformParams[1 * uniformCount];
    auto* matrixStrides = &uniformParams[2 * uniformCount];
    auto* arrayStrides = &uniformParams[3 * uniformCount];
    auto* arraySizes = &uniformParams[4 * uniformCount];

    glGetActiveUniformsiv(glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_OFFSET, offsets);
    glGetActiveUniformsiv(glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_TYPE, types);
    glGetActiveUniformsiv(glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_SIZE, arraySizes);
    glGetActiveUniformsiv(glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_ARRAY_STRIDE, arrayStrides);
    glGetActiveUniformsiv(glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_MATRIX_STRIDE, matrixStrides);

    for (size_t i{0}; i < uniformCount; i++) {
        uniformInfos[i].type = types[i];
        uniformInfos[i].offset = offsets[i];
        if (matrixStrides[i] > 0)
            uniformInfos[i].matrixStride = matrixStrides[i];
        if (arrayStrides[i] > 0)
            uniformInfos[i].arrayParams = {arraySizes[i], arrayStrides[i]};
    }

    GLchar glNameBuffer[MAX_GL_NAME_LENGTH]{};
    std::unordered_map<std::string, UniformInfo> namedUniformInfos;
    namedUniformInfos.reserve(uniformCount);
    for (size_t i{0}; i < uniformCount; i++) {
        glGetActiveUniformName(glProgram, uniformIndices[i], 512, NULL,
                               glNameBuffer);
        std::string uniformName{&glNameBuffer[blockName.size()]};
        auto bracketPos = uniformName.rfind("[");
        if (bracketPos != std::string::npos) {
            uniformName.erase(uniformName.begin() + bracketPos,
                              uniformName.end());
        }
        namedUniformInfos.insert({std::move(uniformName), uniformInfos[i]});
    }
    return namedUniformInfos;
}

void Block::printUniformsLayouts() {
    for (const auto& [blockName, blockInfo] : blockLayouts) {
        std::cout << blockName;
        std::cout << " [num_uniforms: " << blockInfo.uniforms.size()
                  << "; byte_size: " << blockInfo.byteSize << "]\n";
        for (const auto& [uniformName, uniformInfo] : blockInfo.uniforms) {
            std::cout << '\t' << uniformName;
            std::cout << " [type: " << uniformInfo.type
                      << "; offset: " << uniformInfo.offset;
            if (uniformInfo.matrixStride.has_value()) {
                std::cout << "; matrix stride: "
                          << uniformInfo.matrixStride.value();
            }
            if (uniformInfo.arrayParams.has_value()) {
                std::cout << "; aray elements: "
                          << uniformInfo.arrayParams->first << "; aray stride: "
                          << uniformInfo.arrayParams->second;
            }
            std::cout << "]\n";
        }
        std::cout << "\n";
    }
}

}  // namespace shared

}  // namespace gl