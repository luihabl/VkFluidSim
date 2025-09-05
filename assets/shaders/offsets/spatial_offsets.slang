
struct Constants {
    uint* sorted_keys;
    uint* offsets;
    uint num_inputs;
}

static const uint group_size = 256;

[shader("compute")]
[numthreads(group_size, 1, 1)]
void init_offsets(uint id: SV_DispatchThreadID, uniform Constants k) {
    if (id >= k.num_inputs)
        return;

    k.offsets[id] = k.num_inputs;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void calculate_offsets(uint id: SV_DispatchThreadID, uniform Constants k) {
    if (id >= k.num_inputs)
        return;

    uint key = k.sorted_keys[id];
    uint key_prev = id == 0 ? k.num_inputs : k.sorted_keys[id - 1];

    if (key != key_prev) {
        k.offsets[key] = id;
    }
}
