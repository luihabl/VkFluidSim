const ivec2 offsets_2d[9] =
{
	ivec2(-1, 1),
	ivec2(0, 1),
	ivec2(1, 1),
	ivec2(-1, 0),
	ivec2(0, 0),
	ivec2(1, 0),
	ivec2(-1, -1),
	ivec2(0, -1),
	ivec2(1, -1),
};

const uint hash_k1 = 15823;
const uint hash_k2 = 9737333;

ivec2 GetCell2D(vec2 pos, float radius) {
    return ivec2(floor(pos/radius));
}

uint HashCell2D(ivec2 cell) {
    uint a = cell.x * hash_k1;
    uint b = cell.y * hash_k2;
    return (a + b);
}

uint KeyFromHash(uint hash, uint table_size) {
    return hash % table_size;
}