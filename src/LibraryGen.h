#ifndef LIBRARYGEN_H
#define LIBRARYGEN_H

#include <vector>
#include <glm/glm.hpp>
#include "Delaunay2D.h"
#include "Grid.h"
#include "Pathfinder.h"
#include <random>
#include <iostream>
#include <map>
#include "Enemy.h"
#include "Config.h"

class LibraryGen {
    public:
        LibraryGen() : grid(glm::ivec2(1, 1), Cell(CellType::NONE)) {}

        std::vector<glm::vec3> getEnemySpawnPositions() const {
            return enemySpawnPositions;
        }

        // enum CellType {NONE, SHELF, PATH, SPAWN, BOSS_ENTRANCE, TOP_BORDER, BOTTOM_BORDER, LEFT_BORDER, RIGHT_BORDER};

        enum class CellType {
            NONE, // Empty space
            CLUSTER,
            PATH,
            SPAWN,
            BOSS_ENTRANCE,
            BORDER,
            ENEMY_SPAWN,
        };

        inline const char* toString(CellType type) {
            switch (type) {
                case CellType::NONE: return "NONE";
                case CellType::CLUSTER: return "CLUSTER";
                case CellType::PATH: return "PATH";
                case CellType::SPAWN: return "SPAWN";
                case CellType::BOSS_ENTRANCE: return "BOSS_ENTRANCE";
                case CellType::BORDER: return "BORDER";
                default: return "UNKNOWN";
            }
        };

        enum class ClusterType {
            NONE,
            SHELF1, // Cluster types for shelves maybe different layouts or different models
            SHELF2,
            SHELF3,
            GLOWING_SHELF1,
            GLOWING_SHELF2,
            LAYOUT1,
            LAYOUT2,
            LAYOUT3,
            ONLY_TABLE,
            ONLY_CLOCK,
            ONLY_CANDELABRA,
            ONLY_CHEST,
            ONLY_BOOKSTAND,
        };

        enum class BorderType {
            NONE,
            TOP_BORDER,
            BOTTOM_BORDER,
            LEFT_BORDER,
            RIGHT_BORDER,
            LEFT_OF_BOSS_ENTRANCE,
            RIGHT_OF_BOSS_ENTRANCE,
        };

        enum class CellObjType {
            NONE,
            BOOKSHELF,
            TABLE,
            CHAIR,
            TABLE_AND_CHAIR1,
            TABLE_AND_CHAIR2,
            DOOR,
            CANDELABRA,
            BOOK,
            GRANDFATHER_CLOCK,
            CHEST,
            ROTATED_BOOKSHELF,
            SHELF_WITH_ABILITY,
            SHELF_WITH_ABILITY_ROTATED,
            BOOKSTAND,
        };

        struct transform {
            glm::vec3 position;
            float rotation; // Rotation in degrees
            glm::vec3 scale;
        };

        struct Cell {
            CellType type;
            // union {
            //     ClusterType clusterType; // if type == CLUSTER
            //     BorderType borderType; // if type == BORDER
            // };
            ClusterType clusterType = ClusterType::NONE; // Default to NONE
            BorderType borderType = BorderType::NONE; // Default to NONE
            CellObjType objectType = CellObjType::NONE; // Default to NONE
            transform transformData = {glm::vec3(0), glm::radians(0.0f), glm::vec3(1)}; // Default transform data

            Cell() : type(CellType::NONE) {} // Default constructor
            Cell(CellType t) : type(t) {} // Constructor with type
            Cell(CellType t, ClusterType ct) : type(t), clusterType(ct) {} // Constructor with type and cluster type
            Cell(CellType t, BorderType bt) : type(t), borderType(bt) {} // Constructor with type and border type
            Cell(CellType t, ClusterType ct, CellObjType ot) : type(t), clusterType(ct), objectType(ot) {} // Constructor with type, cluster type and object type
            Cell(CellType t, CellObjType ot) : type(t), objectType(ot) {} // Constructor with type and object type
        };


        // };

        void generate(glm::ivec2 size, glm::vec3 worldOrigin = glm::vec3(0, 0, 0),
            glm::vec3 spawnPos = {0, 0, 0}, glm::vec2 bossEntrDir = {1, 0});
        const Grid<Cell>& getGrid() const { return grid; }
        // Cell getCell(const glm::ivec2& pos) const { return grid.getCell(pos); }
        std::mt19937& getSeedGen() { return seedGen; }

        int mapXtoGridX(float x) const {
            // float worldXwidth = size.x;
            // return (x-(-(size.x))/worldXwidth) * size.x;
            float worldXWidth = grid.getSize().x * 2;
            float localX = x - LibraryworldOrigin.x;
            // return (x - (-size.x)) /  (worldXWidth / (size.x - 1));
            return static_cast<int>((localX - (-grid.getSize().x)) / (worldXWidth / (grid.getSize().x - 1)));
        }

        int mapZtoGridY(float z) const {
            // float worldZwidth = size.y;
            // return (z-(-(size.y))/worldZwidth) * size.y;
            float worldZwidth = grid.getSize().y * 2;
            // return (z - (-size.y)) / (worldZwidth / (size.y - 1));
            float localZ = z - LibraryworldOrigin.z;
            return static_cast<int>((localZ - (-grid.getSize().y)) / (worldZwidth / (grid.getSize().y - 1)));
        }

        float mapGridXtoWorldX(int x) const {
            float worldXwidth = grid.getSize().x * 2;
            // return (x * (worldXwidth / (size.x - 1))) - size.x;
            float localX = (-grid.getSize().x) + (x * (worldXwidth / (grid.getSize().x - 1)));
            return LibraryworldOrigin.x + localX;
        }

        float mapGridYtoWorldZ(int y) const {
            float worldZwidth = grid.getSize().y * 2;
            // return (y * (worldZwidth / (size.y - 1))) - size.y;
            float localZ = (-grid.getSize().y) + (y * (worldZwidth / (grid.getSize().y - 1)));
            return LibraryworldOrigin.z + localZ;
        }

    private:
        Grid<Cell> grid; // Grid of CellType
        std::vector<glm::vec2> clusterCenters;
        // std::vector<Enemy> libraryEnemies;
        std::vector<glm::vec3> enemySpawnPositions;
        std::mt19937 seedGen;
        glm::vec2 spawnPosinGrid;
        glm::vec2 bossEntranceDir;
        std::vector<glm::vec2> avoidPoints;
        glm::ivec2 gridSize;
        glm::vec3 LibraryworldOrigin = glm::vec3(0, 0, 0); // World origin for the grid

        // struct Layout {
        //     std::vector<glm::ivec2> relativePositions;
        //     std::vector<CellType> objectTypes;
        // };

        std::map<ClusterType, float> objMinSpacing = {
            {ClusterType::SHELF1, 5.0f},
            {ClusterType::SHELF2, 3.0f},
            {ClusterType::SHELF3, 3.0f},
            {ClusterType::LAYOUT1, 7.0f},
            {ClusterType::LAYOUT2, 7.0f},
            {ClusterType::LAYOUT3, 7.0f},
            {ClusterType::ONLY_TABLE, 3.0f},
            {ClusterType::ONLY_CLOCK, 1.0f},
            {ClusterType::ONLY_CANDELABRA, 1.0f},
            {ClusterType::ONLY_CHEST, 1.0f},
            {ClusterType::ONLY_BOOKSTAND, 0.5f},
            {ClusterType::GLOWING_SHELF1, 3.0f},
            {ClusterType::GLOWING_SHELF2, 3.0f},
        };

        std::map<ClusterType, int> MaxobjAmount = {
            // {ClusterType::SHELF1, 1},
            // {ClusterType::SHELF2, 1},
            // {ClusterType::SHELF3, 1},
            {ClusterType::LAYOUT1, 2},
            // {ClusterType::LAYOUT2, 2},
            // {ClusterType::LAYOUT3, 2},
            {ClusterType::ONLY_TABLE, 1},
            {ClusterType::ONLY_CLOCK, 2},
            {ClusterType::ONLY_CANDELABRA, 4},
            {ClusterType::ONLY_CHEST, 1},
            {ClusterType::ONLY_BOOKSTAND, 1},
            {ClusterType::GLOWING_SHELF1, 2},
            {ClusterType::GLOWING_SHELF2, 2},
        };

        std::map<ClusterType, int> objAmount = {
            // {ClusterType::SHELF1, 0},
            // {ClusterType::SHELF2, 0},
            // {ClusterType::SHELF3, 0},
            {ClusterType::LAYOUT1, 0},
            // {ClusterType::LAYOUT2, 0},
            // {ClusterType::LAYOUT3, 0},
            {ClusterType::ONLY_TABLE, 0},
            {ClusterType::ONLY_CLOCK, 0},
            {ClusterType::ONLY_CANDELABRA, 0},
            {ClusterType::ONLY_CHEST, 0},
            {ClusterType::ONLY_BOOKSTAND, 0},
            {ClusterType::GLOWING_SHELF1, 0},
            {ClusterType::GLOWING_SHELF2, 0},
        };

        std::vector<ClusterType> clusterOptions = {
            ClusterType::SHELF1,
            // ClusterType::SHELF2,
            ClusterType::SHELF3,
            ClusterType::LAYOUT1,
            // ClusterType::LAYOUT2,
            // ClusterType::LAYOUT3,
            // ClusterType::ONLY_TABLE,
            ClusterType::ONLY_CLOCK,
            ClusterType::ONLY_CANDELABRA,
            ClusterType::ONLY_CHEST,
            ClusterType::ONLY_BOOKSTAND,
            ClusterType::GLOWING_SHELF1,
            ClusterType::GLOWING_SHELF2,
        };

        std::vector<std::pair<glm::ivec2, glm::ivec2>> selectedEdges;

        void placeClusters(int count);
        void placeEnemies(int numEnemies);
        void triangulateClusters();
        void generatePaths();
        void addShelfWalls();
        void placeBorder();

        Pathfinder::PathCost calcCost(const glm::ivec2& from, const glm::ivec2& to);
};

#endif // LIBRARYGEN_H