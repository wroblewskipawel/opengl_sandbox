#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "world/tile_set.h"

namespace world {

class TileMap {
   public:
    using Map = std::vector<std::pair<uint32_t, uint32_t>>;

    TileMap(glm::uvec3 shape, const TileSet& tileSet);

    const glm::uvec3& shape() const { return m_shape; }
    const Map& map() const { return m_map; }

   private:
    void solveChunk(glm::uvec3 offset, glm::uvec3 size, const TileSet& tileSet);
    void updatePosition(std::vector<uint32_t>& candidates, uint32_t neighbor,
                        uint32_t neighborOrientation);

    glm::uvec3 m_shape;
    Map m_map;
};

}  // namespace world