implementing "../simulation_3d.slang";

static const int3 offsets_3d[27] = {
    int3(-1, -1, -1), int3(0, -1, -1), int3(1, -1, -1), int3(-1, 0, -1), int3(0, 0, -1),
    int3(1, 0, -1),   int3(-1, 1, -1), int3(0, 1, -1),  int3(1, 1, -1),  int3(-1, -1, 0),
    int3(0, -1, 0),   int3(1, -1, 0),  int3(-1, 0, 0),  int3(0, 0, 0),   int3(1, 0, 0),
    int3(-1, 1, 0),   int3(0, 1, 0),   int3(1, 1, 0),   int3(-1, -1, 1), int3(0, -1, 1),
    int3(1, -1, 1),   int3(-1, 0, 1),  int3(0, 0, 1),   int3(1, 0, 1),   int3(-1, 1, 1),
    int3(0, 1, 1),    int3(1, 1, 1)
};

static const uint hash_k1 = 15823;
static const uint hash_k2 = 9737333;
static const uint hash_k3 = 440817757;

int3 GetCell3D(float3 position, float radius) {
    return (int3)floor(position / radius);
}

uint HashCell3D(int3 cell) {
    const uint blockSize = 50;
    uint3 ucell = (uint3)(cell + blockSize / 2);

    uint3 localCell = ucell % blockSize;
    uint3 blockID = ucell / blockSize;
    uint blockHash = blockID.x * hash_k1 + blockID.y * hash_k2 + blockID.z * hash_k3;
    return localCell.x + blockSize * (localCell.y + blockSize * localCell.z) + blockHash;
}

uint KeyFromHash(uint hash, uint tableSize) {
    return hash % tableSize;
}
