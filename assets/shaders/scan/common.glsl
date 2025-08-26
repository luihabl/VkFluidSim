
const uint GROUP_SIZE = 256;
const uint ITEMS_PER_GROUP = 2 * GROUP_SIZE;

layout (local_size_x = GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(buffer_reference, scalar) readonly buffer BufferRefVec2{ 
	vec2 data[];
};

layout(buffer_reference, scalar) readonly buffer BufferRefUInt{ 
	uint data[];
};

layout( push_constant ) uniform PushConstants {	
    BufferRefUInt elements;
    BufferRefUInt group_sums;
    uint item_count;
} pc;