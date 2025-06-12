#include "LibraryGen.h"
#include "Grid.h"

/*
    * LibraryGen.cpp
    * This file contains the implementation of the LibraryGen class, which is responsible for generating a library layout.
    * It uses a grid-based approach to place shelves and paths, and utilizes Delaunay triangulation for pathfinding.
    *
    *
    */

void LibraryGen::generate(glm::ivec2 size, glm::vec3 worldOrigin, glm::vec3 spawnPos, glm::vec2 bossEntrDir) {

    std::cout << "Grid size: " << size.x << ", " << size.y << std::endl;

    this->LibraryworldOrigin = worldOrigin; // Set the world origin for the grid


    grid = Grid<Cell>(size, Cell(CellType::NONE)); // Initialize the grid with the given size and offset

    int i = mapXtoGridX(spawnPos.x);
    int j = mapZtoGridY(spawnPos.z);

    this->gridSize = size; // Store the grid size

    spawnPosinGrid = glm::vec2(i, j); // Convert the spawn position to grid coordinates

    if (grid.inBounds(spawnPosinGrid)) {
        grid[spawnPosinGrid] = CellType::SPAWN; // Mark the spawn position in the grid
        std::cout << "Spawn position in grid: " << spawnPosinGrid.x << ", " << spawnPosinGrid.y << std::endl;
    } else {
        std::cerr << "Spawn not in grid bounds: " << spawnPosinGrid.x << ", " << spawnPosinGrid.y << std::endl;
    }

    bossEntranceDir = bossEntrDir; // Set the boss entrance direction

    std::cout << "boss entrance direction: " << bossEntranceDir.x << ", " << bossEntranceDir.y << std::endl;

    std::cout << "Generating library layout..." << std::endl;

    seedGen.seed(std::random_device()());

    clusterCenters.clear(); // Clear any existing cluster centers
    avoidPoints.clear(); // Clear any existing avoid points

    placeBorder();

    // int numberOfClusters = size.x * size.y / 50; // Destiny controls the number of clusters
    int numberOfClusters = 40;

    placeClusters(numberOfClusters);
    enemySpawnPositions.clear(); // Clear any existing enemy spawn positions
    placeEnemies(Config::NUM_ENEMIES); // Place enemies in the library

    std::cout << "Placed " << clusterCenters.size() << " clusters." << std::endl;

    // triangulateClusters();

    // generatePaths();

    // addShelfWalls();
}

void LibraryGen::placeClusters(int count) {
    // Randomly place cluster centers in the grid
    std::uniform_int_distribution<int> distX(0, grid.getSize().x - 1);
    std::uniform_int_distribution<int> distY(0, grid.getSize().y - 1);

    std::uniform_int_distribution<int> clusterTypeDist(0, clusterOptions.size() - 1);

    std::cout << "Placing clusters..." << std::endl;

    int attempts = 0;
    int maxAttempts = grid.getSize().x * grid.getSize().y / 5; // Limit attempts to avoid infinite loop
    bool placedBookstand = false; // Flag to check if bookshelf is placed
    bool hasPlacedGlowingShelf = false; // Flag to check if glowing shelf is placed

    for (auto& obj : objAmount) {
        obj.second = 0; // Reset object amounts
    }

    while (clusterCenters.size() < count && attempts < maxAttempts) {
        attempts++;
        glm::ivec2 pos{distX(seedGen), distY(seedGen)};

        // Ensure minimum spacing between clusters
        bool valid = true;

        std::cout << "Trying to place cluster at: " << pos.x << ", " << pos.y << std::endl;

        if (grid[pos].type == CellType::SPAWN ||
            grid[pos].type == CellType::BOSS_ENTRANCE) {
            valid = false; // Position is already occupied by spawn or wall no need to check further
        }

        // Check if the position is already occupied or too close to existing clusters
        for (const auto& center : clusterCenters) {
            // check if the distance between the new position and existing cluster centers is too small
            // and if the new position is either the spawn position or too close to it
            float requiredSpacing = objMinSpacing[grid[center].clusterType]; // Get the required spacing for the cluster type
            for (const auto& avoidPoint : avoidPoints) {
                if (glm::distance(glm::vec2(pos), glm::vec2(avoidPoint)) < requiredSpacing) {
                    valid = false;
                    break;
                }
            }

            // checks the distance between the walls/borders of the grid and the new cluster
            if (glm::distance(glm::vec2(center), glm::vec2(pos)) < requiredSpacing ||
                glm::distance(glm::vec2(pos), glm::vec2(pos.x, gridSize.y - 1)) < requiredSpacing || // check distance against all 4 borders
                glm::distance(glm::vec2(pos), glm::vec2(pos.x, 0)) < requiredSpacing ||
                glm::distance(glm::vec2(pos), glm::vec2(gridSize.x - 1, pos.y)) < requiredSpacing ||
                glm::distance(glm::vec2(pos), glm::vec2(0, pos.y)) < requiredSpacing ||
                glm::distance(glm::vec2(pos), glm::vec2(spawnPosinGrid)) < 3.0f) {
                valid = false;
                break;
            }
        }

        // Check if the position is within bounds
        if (valid) {

            // if the position is valid, add it to the cluster centers
            clusterCenters.push_back(pos);
            ClusterType randomClusterType;

            // ensures that bookstand is always placed first
            if (!placedBookstand) {
                randomClusterType = ClusterType::ONLY_BOOKSTAND;
                placedBookstand = true;
            } else if (!hasPlacedGlowingShelf) {
                randomClusterType = ClusterType::GLOWING_SHELF1; // ensures that glowing shelf is placed first
                hasPlacedGlowingShelf = true;
            } else {
                randomClusterType = clusterOptions[clusterTypeDist(seedGen)];
            }

            // Limits the number of objects of each type based of MaxobjAmount
            while (MaxobjAmount.count(randomClusterType) && objAmount[randomClusterType] >= MaxobjAmount[randomClusterType]) {
                randomClusterType = clusterOptions[clusterTypeDist(seedGen)];
            }
            // add if the cluster type is in the map
            if (objAmount.count(randomClusterType)) {
                objAmount[randomClusterType]++;
            }


            if (randomClusterType == ClusterType::SHELF1) {
                for (int dx = -1; dx <= 1; ++dx) {
                    glm::ivec2 shelfPos = pos + glm::ivec2(dx, 0);
                    if (grid.inBounds(shelfPos)) {
                        grid[shelfPos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSHELF); // Mark the cluster position in the grid
                        grid[shelfPos].transformData.scale = glm::vec3(2.0f); // Scale the object
                    }
                }
            } else if (randomClusterType == ClusterType::SHELF2) {
                // Place a different type of shelf cluster
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType); // Mark the cluster position in the grid
                }
            } else if (randomClusterType == ClusterType::SHELF3) {
                // Place another type of shelf cluster
                // will be rotated in drawwing function
                for (int dx = -1; dx <= 1; ++dx) {
                    glm::ivec2 shelfPos = pos + glm::ivec2(0, dx);
                    if (grid.inBounds(shelfPos)) {
                        grid[shelfPos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::ROTATED_BOOKSHELF); // Mark the cluster position in the grid
                        grid[shelfPos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                        grid[shelfPos].transformData.scale = glm::vec3(2.0f); // Scale the object
                    }
                }
            } else if (randomClusterType == ClusterType::LAYOUT1) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::TABLE_AND_CHAIR1);
                    grid[pos].transformData.scale = glm::vec3(0.35f);
                }
                glm::ivec2 layoutpos = pos + glm::ivec2(1, 0);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::ROTATED_BOOKSHELF);
                    grid[layoutpos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f);
                }
                layoutpos = pos + glm::ivec2(1, 1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::ROTATED_BOOKSHELF);
                    grid[layoutpos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f);
                }
                layoutpos = pos + glm::ivec2(1, -1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::ROTATED_BOOKSHELF);
                    grid[layoutpos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f);
                }
                layoutpos = pos + glm::ivec2(0, -2);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSHELF);
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f);
                }
                layoutpos = pos + glm::ivec2(0, 2);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSHELF);
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f);
                }
                layoutpos = pos + glm::ivec2(0, 1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::CHEST);
                    grid[layoutpos].transformData.scale = glm::vec3(0.25f);
                }
                layoutpos = pos + glm::ivec2(0, -1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::CANDELABRA);
                    grid[layoutpos].transformData.scale = glm::vec3(0.5f);
                }
            } else if (randomClusterType == ClusterType::LAYOUT2) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::TABLE_AND_CHAIR2);
                    grid[pos].transformData.scale = glm::vec3(0.35f);
                }
                glm::ivec2 layoutpos = pos + glm::ivec2(0, 1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSHELF);
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f);
                }
            } else if (randomClusterType == ClusterType::LAYOUT3) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::TABLE);
                    grid[pos].transformData.scale = glm::vec3(0.35f);
                }
                glm::ivec2 layoutpos = pos + glm::ivec2(1, 1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::CHAIR);
                    grid[layoutpos].transformData.scale = glm::vec3(0.35f);
                }
            } else if (randomClusterType == ClusterType::ONLY_CANDELABRA) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::CANDELABRA); // Mark the cluster position in the grid
                    grid[pos].transformData.scale = glm::vec3(0.5f); // Scale the object
                }
            } else if (randomClusterType == ClusterType::ONLY_CLOCK) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::GRANDFATHER_CLOCK); // Mark the cluster position in the grid
                    grid[pos].transformData.scale = glm::vec3(0.5f); // Scale the object
                }
            } else if (randomClusterType == ClusterType::ONLY_CHEST) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::CHEST); // Mark the cluster position in the grid
                    grid[pos].transformData.scale = glm::vec3(0.25f); // Scale the object
                }
            } else if (randomClusterType == ClusterType::ONLY_TABLE) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::TABLE_AND_CHAIR1); // Mark the cluster position in the grid
                    grid[pos].transformData.scale = glm::vec3(0.35f); // Scale the object
                }
            } else if (randomClusterType == ClusterType::ONLY_BOOKSTAND) {
                glm::ivec2 layoutpos = glm::ivec2(spawnPosinGrid.x, spawnPosinGrid.y) + glm::ivec2(1, 0);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSTAND); // Mark the cluster position in the grid
                    grid[layoutpos].transformData.scale = glm::vec3(0.75f); // Scale the object
                }
            } else if (randomClusterType == ClusterType::GLOWING_SHELF1) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::SHELF_WITH_ABILITY); // Mark the cluster position in the grid
                    grid[pos].transformData.scale = glm::vec3(2.0f); // Scale the object
                }
                glm::ivec2 layoutpos = pos + glm::ivec2(1, 0);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSHELF); // Mark the cluster position in the grid
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f); // Scale the object
                }
                layoutpos = pos + glm::ivec2(-1, 0);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::BOOKSHELF); // Mark the cluster position in the grid
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f); // Scale the object
                }
            } else if (randomClusterType == ClusterType::GLOWING_SHELF2) {
                if (grid.inBounds(pos)) {
                    grid[pos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::SHELF_WITH_ABILITY_ROTATED); // Mark the cluster position in the grid
                    grid[pos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                    grid[pos].transformData.scale = glm::vec3(2.0f); // Scale the object
                }
                glm::ivec2 layoutpos = pos + glm::ivec2(0, 1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::ROTATED_BOOKSHELF); // Mark the cluster position in the grid
                    grid[layoutpos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f); // Scale the object
                }
                layoutpos = pos + glm::ivec2(0, -1);
                if (grid.inBounds(layoutpos)) {
                    grid[layoutpos] = Cell(CellType::CLUSTER, randomClusterType, CellObjType::ROTATED_BOOKSHELF); // Mark the cluster position in the grid
                    grid[layoutpos].transformData.rotation = glm::radians(90.0f); // Rotate the object
                    grid[layoutpos].transformData.scale = glm::vec3(2.0f); // Scale the object
                }
            }
            else {
                std::cerr << "Unknown cluster type: " << static_cast<int>(randomClusterType) << std::endl;
            }

            // if (grid.inBounds(pos)) {
            //     grid[pos] = Cell(CellType::CLUSTER, ClusterType::SHELF1); // Mark the cluster position in the grid
            // }

            attempts = 0; // Reset attempts if a cluster is successfully placed
            std::cout << "Placed cluster at: " << pos.x << ", " << pos.y << std::endl;
            std::cout << "Cluster type: " << static_cast<int>(randomClusterType) << std::endl;
        }

    }
}

void LibraryGen::placeEnemies(int numEnemies) {
    int gridWidth = grid.getSize().x;
int gridHeight = grid.getSize().y;

// Calculate how many rows and cols of sectors to create
int sectorCols = std::ceil(std::sqrt(numEnemies));
int sectorRows = std::ceil(float(numEnemies) / sectorCols);

int sectorWidth = gridWidth / sectorCols;
int sectorHeight = gridHeight / sectorRows;

vector<glm::ivec2> enemyPos; // Vector to store enemy positions

int placed = 0;
for (int row = 0; row < sectorRows && placed < numEnemies; ++row) {
    for (int col = 0; col < sectorCols && placed < numEnemies; ++col) {
        // Sector bounds
        int xStart = col * sectorWidth;
        int xEnd = std::min((col + 1) * sectorWidth - 1, gridWidth - 1);
        int yStart = row * sectorHeight;
        int yEnd = std::min((row + 1) * sectorHeight - 1, gridHeight - 1);

        std::uniform_int_distribution<int> distX(xStart, xEnd);
        std::uniform_int_distribution<int> distY(yStart, yEnd);

        int tries = 0;
        bool placedInSector = false;

        while (!placedInSector && tries < 20) {
            glm::ivec2 pos{distX(seedGen), distY(seedGen)};
            tries++;

            bool valid = true;

            if (grid[pos].type == CellType::SPAWN ||
                grid[pos].type == CellType::ENEMY_SPAWN ||
                grid[pos].type == CellType::BORDER) {
                valid = false;
            }

            if (glm::distance(glm::vec2(pos), glm::vec2(spawnPosinGrid)) < 4.0f) {
                valid = false;
            }

            if (valid) {
                enemyPos.push_back(pos);
                grid[pos] = Cell(CellType::ENEMY_SPAWN);
                std::cout << "Placed enemy at: " << pos.x << ", " << pos.y << std::endl;
                placedInSector = true;
                placed++;
            }
        }
    }
}


    for (const auto& pos : enemyPos) {
        // Add enemy positions to the list
        float worldX = grid.mapGridXtoWorldX(pos.x);
        float worldZ = grid.mapGridYtoWorldZ(pos.y);
        enemySpawnPositions.push_back(glm::vec3(worldX, 0, worldZ));
    }

    // exit(0);
}

void LibraryGen::placeBorder() {
    // Place a border around the grid
    std::cout << "Placing border..." << std::endl;

    for (int x = 0; x < grid.getSize().x; ++x) {
        grid[glm::ivec2(x, 0)] = Cell(CellType::BORDER, BorderType::BOTTOM_BORDER); // Bottom border
        grid[glm::ivec2(x, grid.getSize().y - 1)] = Cell(CellType::BORDER, BorderType::TOP_BORDER); // Top border
    }

    for (int y = 0; y < grid.getSize().y; ++y) {
        grid[glm::ivec2(0, y)] = Cell(CellType::BORDER, BorderType::LEFT_BORDER); // Left border
        grid[glm::ivec2(grid.getSize().x - 1, y)] = Cell(CellType::BORDER, BorderType::RIGHT_BORDER); // Right border
    }

    if (bossEntranceDir.x > 0) {
        // in middle of the right wall assuming even number of cells
        grid[glm::ivec2(grid.getSize().x - 1, grid.getSize().y / 2)] = Cell(CellType::BOSS_ENTRANCE); // Right entrance
        grid[glm::ivec2(grid.getSize().x - 1, grid.getSize().y / 2 - 1)] = Cell(CellType::BOSS_ENTRANCE); // Right entrance
         grid[glm::ivec2(grid.getSize().x - 1, grid.getSize().y / 2 + 1)] = Cell(CellType::BORDER, BorderType::RIGHT_OF_BOSS_ENTRANCE); // Right of boss entrance
        grid[glm::ivec2(grid.getSize().x - 1, grid.getSize().y / 2 - 2)] = Cell(CellType::BORDER, BorderType::LEFT_OF_BOSS_ENTRANCE); // Left of boss entrance
        avoidPoints.push_back(glm::vec2(grid.getSize().x - 1, grid.getSize().y / 2)); // Add to avoid points
        avoidPoints.push_back(glm::vec2(grid.getSize().x - 1, grid.getSize().y / 2 - 1)); // Add to avoid points
    } else if (bossEntranceDir.x < 0) {
        grid[glm::ivec2(0, grid.getSize().y / 2)] = Cell(CellType::BOSS_ENTRANCE); // Left entrance
        grid[glm::ivec2(0, grid.getSize().y / 2 - 1)] = Cell(CellType::BOSS_ENTRANCE); // Left entrance
        grid[glm::ivec2(0, grid.getSize().y / 2 + 1)] = Cell(CellType::BORDER, BorderType::RIGHT_OF_BOSS_ENTRANCE); // Right of boss entrance
        grid[glm::ivec2(0, grid.getSize().y / 2 - 2)] = Cell(CellType::BORDER, BorderType::LEFT_OF_BOSS_ENTRANCE); // Right of boss entrance
        avoidPoints.push_back(glm::vec2(0, grid.getSize().y / 2)); // Add to avoid points
        avoidPoints.push_back(glm::vec2(0, grid.getSize().y / 2 - 1)); // Add to avoid points
    } else if (bossEntranceDir.y > 0) {
        grid[glm::ivec2(grid.getSize().x / 2, grid.getSize().y - 1)] = Cell(CellType::BOSS_ENTRANCE); // Top entrance
        grid[glm::ivec2(grid.getSize().x / 2 - 1, grid.getSize().y - 1)] = Cell(CellType::BOSS_ENTRANCE); // Top entrance
        grid[glm::ivec2(grid.getSize().x / 2 + 1, grid.getSize().y - 1)] = Cell(CellType::BORDER, BorderType::RIGHT_OF_BOSS_ENTRANCE); // Right of boss entrance
        grid[glm::ivec2(grid.getSize().x / 2 - 2, grid.getSize().y - 1)] = Cell(CellType::BORDER, BorderType::LEFT_OF_BOSS_ENTRANCE); // Right of boss entrance
        avoidPoints.push_back(glm::vec2(grid.getSize().x / 2, grid.getSize().y - 1)); // Add to avoid points
        avoidPoints.push_back(glm::vec2(grid.getSize().x / 2 - 1, grid.getSize().y - 1)); // Add to avoid points
    } else if (bossEntranceDir.y < 0) {
        grid[glm::ivec2(grid.getSize().x / 2, 0)] = Cell(CellType::BOSS_ENTRANCE); // Bottom entrance
        grid[glm::ivec2(grid.getSize().x / 2 - 1, 0)] = Cell(CellType::BOSS_ENTRANCE); // Bottom entrance
        grid[glm::ivec2(grid.getSize().x / 2 + 1, 0)] = Cell(CellType::BORDER, BorderType::RIGHT_OF_BOSS_ENTRANCE); // Right of boss entrance
        grid[glm::ivec2(grid.getSize().x / 2 - 2, 0)] = Cell(CellType::BORDER, BorderType::LEFT_OF_BOSS_ENTRANCE); // Right of boss entrance
        avoidPoints.push_back(glm::vec2(grid.getSize().x / 2, 0)); // Add to avoid points
        avoidPoints.push_back(glm::vec2(grid.getSize().x / 2 - 1, 0)); // Add to avoid points
    }
}

void LibraryGen::triangulateClusters() {
    // Create a Delaunay triangulation of the cluster centers
    std::vector<Delaunay2D::DelaunayVertex> delaunayVertices;


    if (clusterCenters.empty()) {
        std::cerr << "No cluster centers to triangulate." << std::endl;
        return;
    } else if (clusterCenters.size() < 3) {
        std::cerr << "Not enough cluster centers to triangulate." << std::endl;
        return;
    }

    std::cout << "Creating Delaunay triangulation from "<< clusterCenters.size() << " cluster centers." << std::endl;

    // Store mapping from vertex to cluster center index
    std::unordered_map<Delaunay2D::DelaunayVertex, int, Delaunay2D::DelaunayVertexHash> vertexToClusterIndex;

    for (size_t i = 0; i < clusterCenters.size(); ++i) {
        const auto& center = clusterCenters[i];
        Delaunay2D::DelaunayVertex vertex(center.x, center.y);
        delaunayVertices.push_back(vertex);
        vertexToClusterIndex[vertex] = static_cast<int>(i); // Store the index of the cluster center
    }

    std::cout << "Running triangulation with " << delaunayVertices.size() << " vertices." << std::endl;

    // Perform Delaunay triangulation
    Delaunay2D delaunay = Delaunay2D::triangulate(delaunayVertices);

    std::cout << "Triangulation complete." << std::endl;

    // Convert edges to path connections
    selectedEdges.clear();

    if (delaunay.edges.empty()) {
        std::cerr << "No edges found in triangulation." << std::endl;
        return;
    }

    for (const auto& edge : delaunay.edges) {
        auto uIt = vertexToClusterIndex.find(edge.u);
        auto vIt = vertexToClusterIndex.find(edge.v);

        if (uIt == vertexToClusterIndex.end() || vIt == vertexToClusterIndex.end()) {
            std::cerr << "Could not find cluster index for edge vertex." << std::endl;
            continue;
        }

        int uId = uIt->second;
        int vId = vIt->second;

        if (uId < 0 || uId >= static_cast<int>(clusterCenters.size()) ||
            vId < 0 || vId >= static_cast<int>(clusterCenters.size())) {
            std::cerr << "Invalid cluster index: " << uId << ", " << vId << std::endl;
            continue;
        }

        std::cout << "Connecting cluster " << uId << " to cluster " << vId << std::endl;
        // If Ids are valid, add the edge to the selected edges
        selectedEdges.emplace_back(
            glm::ivec2(clusterCenters[uId]),
            glm::ivec2(clusterCenters[vId])
        );
        std::cout << "Edge added: " << edge.u.position.x << ", " << edge.u.position.y << " to "
                  << edge.v.position.x << ", " << edge.v.position.y << std::endl;
    }

    std::cout << "Selected " << selectedEdges.size() << " edges for path generation." << std::endl;
}

// void LibraryGen::generatePaths() {
//     // Check if there are enough centers to create paths
//     if (selectedEdges.empty()) {
//         std::cerr << "No edges selected for path generation." << std::endl;
//         return;
//     }

//     std::cout << "Generating paths for " << selectedEdges.size() << " edges." << std::endl;


//     Pathfinder pathfinder(grid.getSize());

//     // Connect all clusters with paths
//     for (const auto& edge : selectedEdges) {
//         glm::ivec2 start = edge.first;
//         glm::ivec2 end = edge.second;

//         // Ensure the start and end points are in bounds
//         if (!grid.inBounds(start) || !grid.inBounds(end)) {
//             std::cerr << "Start or end point out of bounds: " << start.x << ", " << start.y << " to " << end.x << ", " << end.y << std::endl;
//             continue;
//         }

//         std::cout << "Finding path from " << start.x << ", " << start.y << " to " << end.x << ", " << end.y << std::endl;

//         auto path = pathfinder.findPath(start, end, [this](Pathfinder::Node* from, Pathfinder::Node* to) {
//             return calcCost(from->position, to->position);
//             }
//         );

//         if (path.empty()) {
//             std::cerr << "No path found from " << start.x << ", " << start.y << " to " << end.x << ", " << end.y << std::endl;
//             continue;
//         }

//         // Mark the path in the grid
//         for (const auto& pos : path) {
//             if (grid[pos] == NONE) {
//                 grid[pos] = PATH;
//             } else {
//                 std::cerr << "Path overlaps with existing shelf at: " << pos.x << ", " << pos.y << std::endl;
//             }
//         }

//         std::cout << "Path found with " << path.size() << " steps." << std::endl;
//     }
// }

// void LibraryGen::addShelfWalls() {
//     std::cout << "Adding walls around shelves..." << std::endl;

//     // Add walls around the shelves - Direction for the 4-connected neighbors
//     const std::vector<glm::ivec2> directions = {
//         {1, 0}, {-1, 0}, {0, 1}, {0, -1}
//     };

//     // Temporary grid to track new shelves/walls
//     Grid<CellType> newShelves(grid.getSize(), {0, 0}, NONE);

//     // Add walls around the paths
//     int wallsAdded = 0;
//     for (int y = 0; y < grid.getSize().y; ++y) {
//         for (int x = 0; x < grid.getSize().x; ++x) {
//             glm::ivec2 pos(x, y);

//             // Verify if the position is in bounds (should be, but just in case)
//             if (!grid.inBounds(pos)) {
//                 continue;
//             }

//             if (grid[pos] == PATH) {
//                 // Check all 4 directions
//                 for (const auto& dir : directions) {
//                     glm::ivec2 neighborPos = pos + dir;
//                     if (grid.inBounds(neighborPos) && grid[neighborPos] == NONE) {
//                         // If the neighbor is empty, mark it as a wall
//                         newShelves[neighborPos] = SHELF;
//                         wallsAdded++;
//                     }
//                 }
//             }
//         }
//     }

//     // Merge new shelves into the main grid
//     for (int y = 0; y < grid.getSize().y; ++y) {
//         for (int x = 0; x < grid.getSize().x; ++x) {
//             glm::ivec2 pos(x, y);

//             // Verify if the position is in bounds (should be, but just in case)
//             if (!newShelves.inBounds(pos)) {
//                 continue;
//             }

//             if (newShelves[pos] == SHELF) {
//                 grid[pos] = SHELF;
//             }
//         }
//     }

//     std::cout << "Added " << wallsAdded << " walls around shelves." << std::endl;

// }


// Pathfinder::PathCost LibraryGen::calcCost(const glm::ivec2& from, const glm::ivec2& to) {
//     Pathfinder::PathCost cost;
//     cost.traversable = grid.inBounds(to) && grid[to] != SHELF;

//     if (grid[to] == PATH) {
//         cost.cost = 1.0f; // Path cost
//     } else if (grid[to] == NONE) {
//         cost.cost = 5.0f; // Empty space cost
//     } else {
//         cost.cost = 0.0f; // Shelf cost (not traversable)
//     }

//     return cost;
// }



