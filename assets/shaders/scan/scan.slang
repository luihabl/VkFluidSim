
static const uint group_size = 256;
static const uint items_per_group = 2 * group_size;

struct Constants {
    uint* elements;
    uint* group_sums;
    uint item_count;
}

groupshared uint tmp[items_per_group];

[shader("compute")]
[numthreads(group_size, 1, 1)]
void scan_combine(uint thread_global: SV_DispatchThreadID,
                  uint group: SV_GroupID,
                  uniform Constants k) {
    uint global_a = thread_global * 2 + 0;
    uint global_b = thread_global * 2 + 1;

    if (global_a < k.item_count)
        k.elements[global_a] += k.group_sums[group];
    if (global_b < k.item_count)
        k.elements[global_b] += k.group_sums[group];
}

[shader("compute")]
[numthreads(group_size, 1, 1)]
void scan_block(uint thread_global: SV_DispatchThreadID,
                uint thread_local: SV_GroupThreadID,
                uint group: SV_GroupID,
                uniform Constants k) {
    uint local_a = thread_local * 2 + 0;
    uint local_b = thread_local * 2 + 1;
    uint global_a = thread_global * 2 + 0;
    uint global_b = thread_global * 2 + 1;

    bool has_a = global_a < k.item_count;
    bool has_b = global_b < k.item_count;

    tmp[local_a] = has_a ? k.elements[global_a] : 0;
    tmp[local_b] = has_b ? k.elements[global_b] : 0;

    uint offset = 1;
    uint num_active_threads;

    for (num_active_threads = group_size; num_active_threads > 0; num_active_threads /= 2) {
        GroupMemoryBarrierWithGroupSync();

        if (thread_local < num_active_threads) {
            uint index_a = offset * (local_a + 1) - 1;
            uint index_b = offset * (local_b + 1) - 1;
            tmp[index_b] = tmp[index_a] + tmp[index_b];
        }
        offset *= 2;
    }

    if (thread_local == 0) {
        k.group_sums[group] = tmp[items_per_group - 1];
        tmp[items_per_group - 1] = 0;
    }

    for (num_active_threads = 1; num_active_threads <= group_size; num_active_threads *= 2) {
        GroupMemoryBarrierWithGroupSync();

        offset /= 2;

        if (thread_local < num_active_threads) {
            uint index_a = offset * (local_a + 1) - 1;
            uint index_b = offset * (local_b + 1) - 1;
            uint sum = tmp[index_a] + tmp[index_b];
            tmp[index_a] = tmp[index_b];
            tmp[index_b] = sum;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (has_a)
        k.elements[global_a] = tmp[local_a];
    if (has_b)
        k.elements[global_b] = tmp[local_b];
}
