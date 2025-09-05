
const uint GROUP_SIZE = 256;

layout(buffer_reference, scalar) readonly buffer BufferRefUInt {
    uint data[];
};

layout(push_constant) uniform PushConstants {
    BufferRefUInt input_items;
    BufferRefUInt input_keys;
    BufferRefUInt sorted_items;
    BufferRefUInt sorted_keys;
    BufferRefUInt counts;
    uint item_count;
} pc;
