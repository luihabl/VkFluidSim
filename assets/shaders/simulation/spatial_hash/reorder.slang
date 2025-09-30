
static const uint group_size = 256;

struct ReorderPushConstants {
    uint* sorted_indices;
    float3* buffer;
    float3* sort_target;
    uint n;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void Reorder(uint id: SV_DispatchThreadID, uniform ReorderPushConstants k) {
    if (id >= k.n)
        return;

    uint sorted_index = k.sorted_indices[id];
    k.sort_target[id] = k.buffer[sorted_index];
}
