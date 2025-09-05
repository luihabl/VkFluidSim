
struct Constants {
    uint* input_items;
    uint* input_keys;
    uint* sorted_items;
    uint* sorted_keys;
    uint* counts;
    uint item_count;
}

static const uint group_size = 256;

[shader("compute")]
[numthreads(group_size, 1, 1)]
void clear_counts(uint id: SV_DispatchThreadID, uniform Constants k) {
    if (id >= k.item_count)
        return;

    k.counts[id] = 0;
    k.input_items[id] = id;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void calculate_counts(uint id: SV_DispatchThreadID, uniform Constants k) {
    if (id >= k.item_count)
        return;

    let key = k.input_keys[id];
    InterlockedAdd(k.counts[key], 1);
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void scatter_output(uint id: SV_DispatchThreadID, uniform Constants k) {
    if (id >= k.item_count)
        return;

    uint key = k.input_keys[id];
    uint sorted_index;
    InterlockedAdd(k.counts[key], 1, sorted_index);

    k.sorted_items[sorted_index] = k.input_items[id];
    k.sorted_keys[sorted_index] = key;
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void copy_back(uint id: SV_DispatchThreadID, uniform Constants k) {
    if (id >= k.item_count)
        return;

    k.input_items[id] = k.sorted_items[id];
    k.input_keys[id] = k.sorted_keys[id];
}
