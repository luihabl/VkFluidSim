implementing "../simulation_2d.slang";

public static const int2 offsets_2d[9] = {
    int2(-1, 1), int2(0, 1),   int2(1, 1),  int2(-1, 0), int2(0, 0),
    int2(1, 0),  int2(-1, -1), int2(0, -1), int2(1, -1),
};

static const uint hash_k1 = 15823;
static const uint hash_k2 = 9737333;

public int2 GetCell2D(float2 pos, float radius) {
    return int2(floor(pos / radius));
}

public uint HashCell2D(int2 cell) {
    uint a = cell.x * hash_k1;
    uint b = cell.y * hash_k2;
    return (a + b);
}

public uint KeyFromHash(uint hash, uint table_size) {
    return hash % table_size;
}
