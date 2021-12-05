#include "world/tile_map.h"

#include <array>
#include <random>

namespace world {

TileMap::TileMap(glm::uvec3 shape, const TileSet& tileSet) : m_shape{shape} {
    m_map.reserve(m_shape.x * m_shape.y * m_shape.z);
    solveChunk(glm::uvec3{0}, glm::uvec3{16, 16, 0}, tileSet);
}

void TileMap::solveChunk(glm::uvec3 offset, glm::uvec3 size,
                         const TileSet& tileSet) {
    std::mt19937_64 rand{};
    using Entry = std::vector<uint32_t>;

    const auto& constraints = tileSet.placementConstraints();
    Entry initialState{};
    for (size_t t{0}; t < constraints.size(); t += 4) {
        for (size_t i{0}; i < 4; i++) {
            initialState.push_back(t + i);
        }
    }

    std::vector<Entry> chunkState{};
    std::unordered_set<size_t> openPositions{};
    chunkState.reserve(size.x * size.y);
    for (size_t y{0}; y < size.y; y++) {
        for (size_t x{0}; x < size.x; x++) {
            chunkState.emplace_back(initialState);
            openPositions.insert(chunkState.size() - 1);
        }
    }
    Map chunk{};
    chunk.resize(size.x * size.y);

    auto comparePos = [&](size_t a, size_t b) {
        return chunkState[a].size() < chunkState[b].size();
    };

    std::array<glm::uvec2, 4> offsets{glm::uvec2{1, 0}, glm::uvec2{-1, 0},
                                      glm::uvec2{0, 1}, glm::uvec2{0, -1}};

    while (!openPositions.empty()) {
        auto min = std::min_element(openPositions.begin(), openPositions.end(),
                                    comparePos);
        auto pos = *min;
        openPositions.erase(min);

        auto& state = chunkState[pos];
        size_t choice = state[rand() % state.size()];
        uint32_t tileID = choice / 4;
        uint32_t tileOrientation = choice % 4;
        chunk[pos] = {tileID, tileOrientation};
        for (auto& offset : offsets) {
            auto& neighbor = chunkState[pos + size.x * offset.y + offset.x];
            if (!neighbor.empty()) {
                updatePosition(neighbor, tileID, tileOrientation);
            }
        }
    }
}

void TileMap::updatePosition(std::vector<uint32_t>& candidates,
                             uint32_t neighbor, uint32_t neighborOrientation) {}

}  // namespace world