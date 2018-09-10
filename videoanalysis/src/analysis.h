#include <glib.h>
#include <gst/gl/gl.h>

struct Accumulator {
        GLfloat frozen;
        GLfloat black;
        GLfloat bright;
        GLfloat diff;
        GLfloat noize;
        GLint   visible;
};

static const char* shader_source =
        "#version 430\n"
        "#extension GL_ARB_compute_shader : enable\n"
        "#extension GL_ARB_shader_image_load_store : enable\n"
        "#extension GL_ARB_shader_storage_buffer_object : enable\n"
        "\n"
        "//precision lowp float;\n"
        "\n"
        "#define WHT_LVL 0.90196078\n"
        "// 210\n"
        "#define BLK_LVL 0.15625\n"
        "// 40\n"
        "#define WHT_DIFF 0.0234375\n"
        "// 6\n"
        "#define GRH_DIFF 0.0078125\n"
        "// 2\n"
        "#define KNORM 4.0\n"
        "#define L_DIFF 5\n"
        "\n"
        "#define BLOCK_SIZE 8\n"
        "\n"
        "struct Noize {\n"
        "        float frozen;\n"
        "        float black;\n"
        "        float bright;\n"
        "        float diff;\n"
        "        float noize;\n"
        "        int   visible;\n"
        "};\n"
        "\n"
        "layout (r8) uniform image2D tex;\n"
        "layout (r8) uniform image2D tex_prev;\n"
        "uniform int width;\n"
        "uniform int height;\n"
        "uniform int stride;\n"
        "uniform int black_bound;\n"
        "uniform int freez_bound;\n"
        "\n"
        "layout (std430, binding=10) buffer Interm {\n"
        "         Noize noize_data [];\n"
        "};\n"
        "\n"
        "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in; \n"
        "\n"
        "float compute_noize (ivec2 pos) {\n"
        "        float lvl;\n"
        "        float pix       = imageLoad(tex, pos).r;\n"
        "        float pix_right = imageLoad(tex, ivec2(pos.x + 1, pos.y)).r;\n"
        "        float pix_down  = imageLoad(tex, ivec2(pos.x, pos.y + 1)).r;\n"
        "        /* Noize */\n"
        "        float res = 0.0;\n"
        "        if ((pix < WHT_LVL) && (pix > BLK_LVL)) {\n"
        "                lvl = GRH_DIFF;\n"
        "        } else {\n"
        "                lvl = WHT_DIFF;\n"
        "        }\n"
        "        if (abs(pix - pix_right) >= lvl) {\n"
        "                res += 1.0/(8.0*8.0*2.0);\n"
        "        }\n"
        "        if (abs(pix - pix_down) >= lvl) {\n"
        "                res += 1.0/(8.0*8.0*2.0);\n"
        "        }\n"
        "        return res;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "        uint block_pos = (gl_WorkGroupID.y * (width / BLOCK_SIZE)) + gl_WorkGroupID.x;\n"
        "        /* Block init */\n"
        "\n"
        "        noize_data[block_pos].noize = 0.0;\n"
        "        noize_data[block_pos].black = 0.0;\n"
        "        noize_data[block_pos].frozen = 0.0;\n"
        "        noize_data[block_pos].bright = 0.0;\n"
        "        noize_data[block_pos].diff = 0.0;\n"
        "        noize_data[block_pos].visible = 0;\n"
        "        \n"
        "        for (int i = 0; i < BLOCK_SIZE; i++) {\n"
        "                for (int j = 0; j < BLOCK_SIZE; j++) {\n"
        "                        float diff_loc;\n"
        "                        ivec2 pix_pos = ivec2(gl_WorkGroupID.x * BLOCK_SIZE + i,\n"
        "                                              gl_WorkGroupID.y * BLOCK_SIZE + j);\n"
        "                        float pix = imageLoad(tex, pix_pos).r;\n"
        "                        /* Noize */\n"
        "                        noize_data[block_pos].noize += compute_noize(pix_pos);\n"
        "                        /* Brightness */\n"
        "                        noize_data[block_pos].bright += float(pix);\n"
        "                        /* Black */\n"
        "                        if (pix <= float(black_bound / 255.0)) {\n"
        "                                noize_data[block_pos].black += 1.0;\n"
        "                        }\n"
        "                        /* Diff */\n"
        "                        diff_loc = abs(pix - imageLoad(tex_prev, pix_pos).r);\n"
        "                        noize_data[block_pos].diff += diff_loc;\n"
        "                        /* Frozen */\n"
        "                        if (diff_loc <= float(freez_bound / 255.0)) {\n"
        "                                noize_data[block_pos].frozen += 1.0;\n"
        "                        }\n"
        "                }\n"
        "        }\n"
        "}\n";

static const char* shader_source_block =
        "#version 430\n"
        "#extension GL_ARB_compute_shader : enable\n"
        "#extension GL_ARB_shader_image_load_store : enable\n"
        "#extension GL_ARB_shader_storage_buffer_object : enable\n"
        "\n"
        "//precision lowp float;\n"
        "\n"
        "#define WHT_LVL 0.90196078\n"
        "// 210\n"
        "#define BLK_LVL 0.15625\n"
        "// 40\n"
        "#define WHT_DIFF 0.0234375\n"
        "// 6\n"
        "#define GRH_DIFF 0.0078125\n"
        "// 2\n"
        "#define KNORM 4.0\n"
        "#define L_DIFF 5\n"
        "\n"
        "#define BLOCK_SIZE 8\n"
        "\n"
        "struct Noize {\n"
        "        float frozen;\n"
        "        float black;\n"
        "        float bright;\n"
        "        float diff;\n"
        "        float noize;\n"
        "        int   visible;\n"
        "};\n"
        "\n"
        "layout (r8, binding = 0) uniform image2D tex;\n"
        "\n"
        "uniform int width;\n"
        "uniform int height;\n"
        "\n"
        "layout (std430, binding=10) buffer Interm {\n"
        "         Noize noize_data [];\n"
        "};\n"
        "\n"
        "//const uint wht_coef[20] = {8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 11, 11, 12, 14, 17, 27};\n"
        "//const uint ght_coef[20] = {4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 10, 13, 23};\n"
        "//const uint wht_coef[20] = {10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 14, 16, 19, 29};	       \n"
        "//const uint ght_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
        "const uint wht_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
        "const uint ght_coef[20] = {2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 8, 11, 21};\n"
        "\n"
        "/* x = Edge pixel offset, y = Edge ID */\n"
        "layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;\n"
        "\n"
        "float get_coef(float noize, uint array[20]) {\n"
        "        uint ret_val;                                     \n"
        "        if((noize>100) || (noize<0))\n"
        "                ret_val = 0;                              \n"
        "        else                                              \n"
        "                ret_val = array[uint(noize/5)];                 \n"
        "        return float(ret_val/255.0);\n"
        "}\n"
        "\n"
        "void main() {\n"
        "        uint block_pos = (gl_WorkGroupID.y * (width / BLOCK_SIZE)) + gl_WorkGroupID.x;\n"
        "        /* l d r u */\n"
        "        uint pixel_off = gl_LocalInvocationID.x;\n"
        "        uint edge = gl_LocalInvocationID.y;\n"
        "        /* l (-1, 0) d (0, -1) r (1, 0) u (0, 1) */ \n"
        "        ivec2 edge_off = ivec2 ( mod(edge + 1, 2) * int(edge - 1),\n"
        "                                 mod(edge, 2) * int(edge - 2));\n"
        "\n"
        "        /* Would not compute blocks near the borders */\n"
        "        if (gl_WorkGroupID.x == 0\n"
        "            || gl_WorkGroupID.x >= (width / BLOCK_SIZE)\n"
        "            || gl_WorkGroupID.y == 0\n"
        "            || gl_WorkGroupID.y >= (height / BLOCK_SIZE))\n"
        "                return;\n"
        "\n"
        "        shared int loc_counter;\n"
        "        shared int vis[4];\n"
        "        /* Noize coeffs */\n"
        "        uint pos = block_pos + edge_off.x + (edge_off.y * width / BLOCK_SIZE);\n"
        "        float noize = 100.0 * max(noize_data[block_pos].noize, noize_data[pos].noize);\n"
        "        float white = get_coef(noize, wht_coef);\n"
        "        float grey  = get_coef(noize, ght_coef);\n"
        "\n"
        "        if (gl_LocalInvocationID.x == 0) {\n"
        "\n"
        "                loc_counter = 0;\n"
        "                atomicExchange(vis[edge], 0);\n"
        "\n"
        "        }\n"
        "\n"
        "        ivec2 zero = ivec2(gl_WorkGroupID.x * BLOCK_SIZE,\n"
        "                           gl_WorkGroupID.y * BLOCK_SIZE);\n"
        "        float pixel = imageLoad(tex, ivec2(zero.x + abs(edge_off.y) * pixel_off +\n"
        "                                           (edge_off.x == 1 ? (BLOCK_SIZE-1) : 0),\n"
        "                                           zero.y + abs(edge_off.x) * pixel_off +\n"
        "                                           (edge_off.y == 1 ? (BLOCK_SIZE-1) : 0))).r;\n"
        "        float prev  = imageLoad(tex, ivec2(zero.x + abs(edge_off.y) * pixel_off + \n"
        "                                           (edge_off.x == 1 ? (BLOCK_SIZE-2) : 0) + \n"
        "                                           (edge_off.x == -1 ? 1 : 0), \n"
        "                                           zero.y + abs(edge_off.x) * pixel_off + \n"
        "                                           (edge_off.y == 1 ? (BLOCK_SIZE-2) : 0) + \n"
        "                                           (edge_off.y == 1 ? 1 : 0))).r;\n"
        "        float next  = imageLoad(tex, ivec2(zero.x + abs(edge_off.y) * pixel_off + \n"
        "                                           (edge_off.x == 1 ? BLOCK_SIZE : 0) + \n"
        "                                           (edge_off.x == -1 ? -1 : 0),\n"
        "                                           zero.y + abs(edge_off.x) * pixel_off + \n"
        "                                           (edge_off.y == 1 ? BLOCK_SIZE : 0) + \n"
        "                                           (edge_off.y == 1 ? -1 : 0))).r;\n"
        "        float next_next  = imageLoad(tex, ivec2(zero.x + abs(edge_off.y) * pixel_off + \n"
        "                                                (edge_off.x == 1 ? (BLOCK_SIZE+1) : 0) + \n"
        "                                                (edge_off.x == -1 ? -2 : 0), \n"
        "                                                zero.y + abs(edge_off.x) * pixel_off + \n"
        "                                                (edge_off.y == 1 ? (BLOCK_SIZE+1) : 0) + \n"
        "                                                (edge_off.y == 1 ? -2 : 0))).r;\n"
        "        float coef = ((pixel < WHT_LVL) && (pixel > BLK_LVL)) ? grey : white;\n"
        "        float denom = round( (abs(prev-pixel) + abs(next-next_next)) / KNORM);\n"
        "        denom = (denom == 0.0) ? 1.0 : denom;\n"
        "        float norm = abs(next-pixel) / denom;\n"
        "        \n"
        "        if (norm > coef)\n"
        "                atomicAdd(vis[edge], 1);\n"
        "\n"
        "        /* counting visible blocks */\n"
        "\n"
        "        if (gl_LocalInvocationID.x == 0) {\n"
        "                if (vis[edge] > L_DIFF)\n"
        "                        atomicAdd(loc_counter, 1);\n"
        "        }\n"
        "\n"
        "        barrier();\n"
        "\n"
        "        if (gl_LocalInvocationID.xy == ivec2(0,0)) {\n"
        "                if (loc_counter >= 2)\n"
        "                        noize_data[block_pos].visible = 1;\n"
        "        }\n"
        "}\n";

static const char* shader_source_accum =
        "#version 430\n"
        "#extension GL_ARB_compute_shader : enable\n"
        "#extension GL_ARB_shader_image_load_store : enable\n"
        "#extension GL_ARB_shader_storage_buffer_object : enable\n"
        "\n"
        "//precision lowp float;\n"
        "\n"
        "#define LOCAL_SIZE 128\n"
        "#define BLOCK_SIZE 8\n"
        "\n"
        "#define BLACK 0\n"
        "#define LUMA 1\n"
        "#define FREEZE 2\n"
        "#define DIFF 3\n"
        "#define BLOCKY 4\n"
        "\n"
        "struct Noize {\n"
        "        float frozen;\n"
        "        float black;\n"
        "        float bright;\n"
        "        float diff;\n"
        "        float noize;\n"
        "        int   visible;\n"
        "};\n"
        "uniform int width;\n"
        "uniform int height;\n"
        "\n"
        "layout (std430, binding=10) buffer Interm {\n"
        "         Noize noize_data [];\n"
        "};\n"
        "layout (std430, binding=11) buffer Result {\n"
        "         float result [];\n"
        "};\n"
        "layout (local_size_x = LOCAL_SIZE, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "void main() {\n"
        "        uint pos = gl_LocalInvocationID.x; \n"
        "        uint local_size = gl_WorkGroupSize.x; \n"
        "        /* Fold array */\n"
        "        for (uint i = pos + local_size; i < (width * height / 64); i += local_size) { \n"
        "            noize_data[pos].frozen  += noize_data[i].frozen;\n"
        "            noize_data[pos].black   += noize_data[i].black;\n"
        "            noize_data[pos].bright  += noize_data[i].bright;\n"
        "            noize_data[pos].diff    += noize_data[i].diff;\n"
        "            noize_data[pos].visible += noize_data[i].visible;\n"
        "        }\n"
        "\n"
        "        groupMemoryBarrier();\n "
        "        memoryBarrier(); \n"
        "        barrier();\n"
        "        if (pos != 0) \n"
        "            return;\n"
        "\n"
        "        for (uint i = 1; i < local_size || i < (width * height / 64); i++) { \n"
        "            noize_data[0].frozen  += noize_data[i].frozen;\n"
        "            noize_data[0].black   += noize_data[i].black;\n"
        "            noize_data[0].bright  += noize_data[i].bright;\n"
        "            noize_data[0].diff    += noize_data[i].diff;\n"
        "            noize_data[0].visible += noize_data[i].visible;\n"
        "        }\n"
        "\n"
        "        result[FREEZE] = 100.0 * noize_data[0].frozen / float(height * width);\n"
        "        result[LUMA]   = 256.0 * noize_data[0].bright / float(height * width);\n"
        "        result[DIFF]   = 100.0 * noize_data[0].diff / float(height * width);\n"
        "        result[BLACK]  = 100.0 * noize_data[0].black / float(height * width);\n"
        "        result[BLOCKY] = 100.0 * float(noize_data[0].visible) / float(height * width / 64);\n"
        "}\n";

static const gchar *shader_source_unused =
        "#version 430\n"
        "#extension GL_ARB_compute_shader : enable\n"
        "#extension GL_ARB_shader_image_load_store : enable\n"
        "#extension GL_ARB_shader_storage_buffer_object : enable\n"
        "\n"
        "precision lowp float;\n"
        "\n"
        "#define WHT_LVL 0.90196078\n"
        "// 210\n"
        "#define BLK_LVL 0.15625\n"
        "// 40\n"
        "#define WHT_DIFF 0.0234375\n"
        "// 6\n"
        "#define GRH_DIFF 0.0078125\n"
        "// 2\n"
        "#define KNORM 4.0\n"
        "#define L_DIFF 5\n"
        "\n"
        "#define BLOCK_SIZE 8\n"
        "\n"
        "#define BLOCKY 0\n"
        "#define LUMA 1\n"
        "#define BLACK 2\n"
        "#define DIFF 3\n"
        "#define FREEZE 4\n"
        "\n"
        "struct Accumulator {\n"
        "        float frozen;\n"
        "        float black;\n"
        "        float bright;\n"
        "        float diff;\n"
        "        int   visible;\n"
        "};\n"
        "\n"
        "layout (r8, binding = 0) uniform lowp image2D tex;\n"
        "layout (r8, binding = 1) uniform lowp image2D tex_prev;\n"
        "uniform int width;\n"
        "uniform int height;\n"
        "uniform int stride;\n"
        "uniform int black_bound;\n"
        "uniform int freez_bound;\n"
        "\n"
        "layout (std430, binding=10) buffer Interm {\n"
        "         Accumulator noize_data [];\n"
        "};\n"
        "\n"
        "//const uint wht_coef[20] = {8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 11, 11, 12, 14, 17, 27};\n"
        "//const uint ght_coef[20] = {4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 10, 13, 23};\n"
        "const uint wht_coef[20] = {10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 14, 16, 19, 29};	       \n"
        "const uint ght_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
        "//const uint wht_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
        "//const uint ght_coef[20] = {2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 8, 11, 21};\n"
        "\n"
        "layout (local_size_x = 1, local_size_y = 1) in;\n"
        "\n"
        "float get_coef(float noize, uint array[20]) {\n"
        "        uint ret_val;                                     \n"
        "        if((noize>100) || (noize<0))\n"
        "                ret_val = 0;                              \n"
        "        else                                              \n"
        "                ret_val = array[uint(noize/5)];                 \n"
        "        return float(ret_val/255.0);\n"
        "}\n"
        "       \n"
        "\n"
        "float compute_noize (ivec2 pos) {\n"
        "        float lvl;\n"
        "        float pix       = imageLoad(tex, pos).r;\n"
        "        float pix_right = imageLoad(tex, ivec2(pos.x + 1, pos.y)).r;\n"
        "        float pix_down  = imageLoad(tex, ivec2(pos.x, pos.y + 1)).r;\n"
        "        /* Noize */\n"
        "        float res = 0.0;\n"
        "        if ((pix < WHT_LVL) && (pix > BLK_LVL)) {\n"
        "                lvl = GRH_DIFF;\n"
        "        } else {\n"
        "                lvl = WHT_DIFF;\n"
        "        }\n"
        "        if (abs(pix - pix_right) >= lvl) {\n"
        "                res += 1.0/(8.0*8.0*2.0);\n"
        "        }\n"
        "        if (abs(pix - pix_down) >= lvl) {\n"
        "                res += 1.0/(8.0*8.0*2.0);\n"
        "        }\n"
        "        return res;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "        uint block_pos = (gl_GlobalInvocationID.y * (width / BLOCK_SIZE)) + gl_GlobalInvocationID.x ;\n"
        "        /* Block init */\n"
        "        float noize = 0.0;\n"
        "        /* l r u d */\n"
        "        vec4 noize_v = vec4(0);\n"
        "        \n"
        "        noize_data[block_pos].black = 0.0;\n"
        "        noize_data[block_pos].frozen = 0.0;\n"
        "        noize_data[block_pos].bright = 0.0;\n"
        "        noize_data[block_pos].diff = 0.0;\n"
        "        noize_data[block_pos].visible = 0;\n"
        "        \n"
        "        for (int i = 0; i < BLOCK_SIZE; i++) {\n"
        "                for (int j = 0; j < BLOCK_SIZE; j++) {\n"
        "                        float diff_loc;\n"
        "                        ivec2 pix_pos = ivec2(gl_GlobalInvocationID.x * BLOCK_SIZE + i,\n"
        "                                              gl_GlobalInvocationID.y * BLOCK_SIZE + j);\n"
        "                        float pix = imageLoad(tex, pix_pos).r;\n"
        "                        /* Noize */\n"
        "                        noize += compute_noize(pix_pos);\n"
        "                        if (gl_GlobalInvocationID.x > 0)\n"
        "                                noize_v[0] += compute_noize(ivec2(pix_pos.x - BLOCK_SIZE, pix_pos.y));\n"
        "                        if (gl_GlobalInvocationID.y > 0)\n"
        "                                noize_v[3] += compute_noize(ivec2(pix_pos.x, pix_pos.y - BLOCK_SIZE));\n"
        "                        if (gl_GlobalInvocationID.x < (width / BLOCK_SIZE))\n"
        "                                noize_v[1] += compute_noize(ivec2(pix_pos.x + BLOCK_SIZE, pix_pos.y));\n"
        "                        if (gl_GlobalInvocationID.x < (height / BLOCK_SIZE))\n"
        "                                noize_v[2] += compute_noize(ivec2(pix_pos.x, pix_pos.y + BLOCK_SIZE));\n"
        "                        /* Brightness */\n"
        "                        noize_data[block_pos].bright += float(pix);\n"
        "                        /* Black */\n"
        "                        if (pix <= float(black_bound / 255.0)) {\n"
        "                                noize_data[block_pos].black += 1.0;\n"
        "                        }\n"
        "                        /* Diff */\n"
        "                        diff_loc = abs(pix - imageLoad(tex_prev, pix_pos).r);\n"
        "                        noize_data[block_pos].diff += diff_loc;\n"
        "                        /* Frozen */\n"
        "                        if (diff_loc <= float(freez_bound / 255.0)) {\n"
        "                                noize_data[block_pos].frozen += 1.0;\n"
        "                        }\n"
        "                }\n"
        "        }\n"
        "        /* Noize coeffs */\n"
        "        noize_v = 100.0 * vec4(max(noize_v[0], noize),\n"
        "                               max(noize_v[1], noize),\n"
        "                               max(noize_v[2], noize),\n"
        "                               max(noize_v[3], noize));\n"
        "        vec4 white_v = vec4(get_coef(noize_v[0], wht_coef),\n"
        "                            get_coef(noize_v[1], wht_coef),\n"
        "                            get_coef(noize_v[2], wht_coef),\n"
        "                            get_coef(noize_v[3], wht_coef));\n"
        "        vec4 grey_v = vec4(get_coef(noize_v[0], ght_coef),\n"
        "                           get_coef(noize_v[1], ght_coef),\n"
        "                           get_coef(noize_v[2], ght_coef),\n"
        "                           get_coef(noize_v[3], ght_coef));\n"
        "        /* compute borders */\n"
        "        if (gl_GlobalInvocationID.x == 0\n"
        "            || gl_GlobalInvocationID.x >= (width / BLOCK_SIZE)\n"
        "            || gl_GlobalInvocationID.y == 0\n"
        "            || gl_GlobalInvocationID.y >= (height / BLOCK_SIZE))\n"
        "                return;\n"
        "        ivec4 vis = {0,0,0,0};\n"
        "        for (int i = 0; i < BLOCK_SIZE; i++) {\n"
        "                /* l r u d */\n"
        "                ivec2 zero = ivec2(gl_GlobalInvocationID.x * BLOCK_SIZE,\n"
        "                                   gl_GlobalInvocationID.y * BLOCK_SIZE);\n"
        "                vec4 pixel = vec4(imageLoad(tex, ivec2(zero.x, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x+BLOCK_SIZE-1, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE-1)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y)).r );\n"
        "                vec4 prev  = vec4(imageLoad(tex, ivec2(zero.x + 1, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x+BLOCK_SIZE-2, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE-2)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y + 1)).r );\n"
        "                vec4 next  = vec4(imageLoad(tex, ivec2(zero.x - 1, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x+BLOCK_SIZE, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y - 1)).r );\n"
        "                vec4 next_next  = vec4(imageLoad(tex, ivec2(zero.x - 2, zero.y + i)).r,\n"
        "                                       imageLoad(tex, ivec2(zero.x+BLOCK_SIZE+1, zero.y + i)).r,\n"
        "                                       imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE+1)).r,\n"
        "                                       imageLoad(tex, ivec2(zero.x + i, zero.y - 2)).r );\n"
        "                vec4 coef = vec4((pixel[0] < WHT_LVL) && (pixel[0] > BLK_LVL) ?\n"
        "                                 grey_v[0] : white_v[0],\n"
        "                                 (pixel[1] < WHT_LVL) && (pixel[1] > BLK_LVL) ?\n"
        "                                 grey_v[1] : white_v[1],\n"
        "                                 (pixel[2] < WHT_LVL) && (pixel[2] > BLK_LVL) ?\n"
        "                                 grey_v[2] : white_v[2],\n"
        "                                 (pixel[3] < WHT_LVL) && (pixel[3] > BLK_LVL) ?\n"
        "                                 grey_v[3] : white_v[3]);\n"
        "                vec4 denom = round( (abs(prev-pixel) + abs(next-next_next)) / KNORM);\n"
        "                denom = vec4(denom[0] == 0.0 ? 1.0 : denom[0],\n"
        "                             denom[1] == 0.0 ? 1.0 : denom[1],\n"
        "                             denom[2] == 0.0 ? 1.0 : denom[2],\n"
        "                             denom[3] == 0.0 ? 1.0 : denom[3]);\n"
        "                vec4 norm = abs(next-pixel) / denom;\n"
        "                vis += ivec4( norm[0] > coef[0] ? 1 : 0,\n"
        "                              norm[1] > coef[1] ? 1 : 0,\n"
        "                              norm[2] > coef[2] ? 1 : 0,\n"
        "                              norm[3] > coef[3] ? 1 : 0 );\n"
        "        }\n"
        "        /* counting visible blocks */\n"
        "        int loc_counter = 0;\n"
        "        for (int side = 0; side < 4; side++) {\n"
        "                if (vis[side] > L_DIFF)\n"
        "                        loc_counter += 1;\n"
        "        }\n"
        "        if (loc_counter >= 2)\n"
        "                noize_data[block_pos].visible = 1;\n"
        "}\n";


