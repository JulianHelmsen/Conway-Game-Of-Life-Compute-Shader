#version 430 core

#define TILE_SIZE  30
#define TILE_SIZE_PADDED 32

layout(local_size_x=TILE_SIZE_PADDED, local_size_y=TILE_SIZE_PADDED, local_size_z=1) in;

layout(r32i, binding = 0) uniform writeonly iimage2D dst;
layout(r32i, binding = 1) uniform readonly iimage2D src;


void main() {
    shared int CELLS[TILE_SIZE_PADDED][TILE_SIZE_PADDED];

    // load the pixel into shared memory

    ivec2 tile_orig = ivec2(gl_WorkGroupID.xy * TILE_SIZE);
    ivec2 shared_pixel_pos = ivec2(gl_LocalInvocationID.xy);
    ivec2 global_pixel_pos = tile_orig + ivec2(gl_LocalInvocationID.xy) - ivec2(1);

    if(global_pixel_pos.x >= 0 || global_pixel_pos.y >= 0 && global_pixel_pos.x < imageSize(src).x && global_pixel_pos.y < imageSize(src).y) {
        CELLS[shared_pixel_pos.x][shared_pixel_pos.y] = imageLoad(src, global_pixel_pos).x;
    }else{
        CELLS[shared_pixel_pos.x][shared_pixel_pos.y] = 0;
    }

    barrier();
    

    ivec2 in_tile = ivec2(gl_LocalInvocationID.xy) - ivec2(1);

    if(in_tile.x >= 0 && in_tile.x < TILE_SIZE && in_tile.y >= 0 && in_tile.y < TILE_SIZE) {
        int next = CELLS[shared_pixel_pos.x][shared_pixel_pos.y];
        int neighbours =    CELLS[shared_pixel_pos.x - 1][shared_pixel_pos.y - 1] + 
                            CELLS[shared_pixel_pos.x + 0][shared_pixel_pos.y - 1] + 
                            CELLS[shared_pixel_pos.x + 1][shared_pixel_pos.y - 1] + 
                            CELLS[shared_pixel_pos.x + 1][shared_pixel_pos.y + 0] + 
                            CELLS[shared_pixel_pos.x + 1][shared_pixel_pos.y + 1] + 
                            CELLS[shared_pixel_pos.x + 0][shared_pixel_pos.y + 1] + 
                            CELLS[shared_pixel_pos.x - 1][shared_pixel_pos.y + 1] + 
                            CELLS[shared_pixel_pos.x - 1][shared_pixel_pos.y + 0];


        if(neighbours == 3) {
            // living
            next = 1;
        }else if(neighbours != 2){
            next = 0;
        }

        imageStore(dst, global_pixel_pos, ivec4(next));
        // imageStore(dst, global_pixel_pos, ivec4((gl_WorkGroupID.x & 1) ^(gl_WorkGroupID.y & 1)));
    }
}

