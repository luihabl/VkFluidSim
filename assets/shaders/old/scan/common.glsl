
const uint GROUP_SIZE = 256;
const uint ITEMS_PER_GROUP = 2 * GROUP_SIZE;

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