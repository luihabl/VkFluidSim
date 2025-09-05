
const uint GROUP_SIZE = 256;

layout(buffer_reference, scalar) readonly buffer BufferRefUInt{ 
	uint data[];
};

layout( push_constant ) uniform PushConstants {	
    BufferRefUInt sorted_keys;
    BufferRefUInt offsets;
    uint num_inputs;
} pc;