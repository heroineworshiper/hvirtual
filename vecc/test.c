// Since the same code is compiled into C and assembly there is
// no need for run time switching between C and assembly.


VECC_BEGIN




alpha_blend(uint8_t* output, uint8_t* input)
{
// Load top red into a vector.
// Use uint_16x4_t instead of uint16_t[4] to prevent pointer arithmetic
// from being done but we still want offsets for uint8_t*.
    uint_16x4_t in_r = { input[0], input[4], input[8], input[12] };

// Load top green into a vector
    uint_16x4_t in_g = { input[1], input[5], input[9], input[13] };

// Load top blue into a vector
    uint_16x4_t in_b = { input[2], input[6], input[10], input[14] };

// Load top opacity into a vector
    uint_16x4_t in_opacity = { input[3], input[7], input[11], input[15] };

// Load bottom red into a vector
    uint_16x4_t out_r = { output[0], output[4], output[8], output[12] };

// Load bottom green into a vector
    uint_16x4_t out_g = { output[1], output[5], output[9], output[13] };

// Load bottom blue into a vector
    uint_16x4_t out_b = { output[2], output[6], output[10], output[14] };

// Load bottom opacity into a vector
    uint_16x4_t out_opacity = { output[3], output[7], output[11], output[15] };

// Calculate top transparency
    uint_16x4_t in_transparency = { 0xffff, 0xffff, 0xffff, 0xffff };
    in_transparency -= in_opacity;

// Fixed point multiply top red by top opacity and store in top red
    in_r high*= in_opacity;

// Fixed point multiply top green by top opacity and store in top green
    in_g high*= in_opacity;

// Fixed point multiply top blue by top opacity and store in top blue
    in_b high*= in_opacity;

// Fixed point multiply bottom red by top transparency and store in bottom red
    out_r high*= in_transparency;

// Fixed point multiply bottom green by top transparency and store in bottom green
    out_g high*= in_transparency;

// Fixed point multiply bottom blue by top transparency and store in bottom blue
    out_b high*= in_transparency;

// Add opaque and transparent components into bottom components
    out_r += in_r;
    out_g += in_g;
    out_b += in_b;

// Store greater of top opacity and bottom opacity in bottom opacity
    out_opacity max= in_opacity;

// Align output components in output vector.
// Because of their types, the vector subscripts will return vector members instead
// of offsetting.
    uint_8x8_t output_temp = { out_r[0], out_g[0], out_b[0], out_opacity[0], out_r[1], out_g[1], out_b[1], out_opacity[1] };

// Aligned memory copy.  Pointer is dereferenced since it wasn't declared
// as a vector.
    output[0] = output_temp;


    output_temp = { out_r[2], out_g[2, out_b[2], out_opacity[2], out_r[3], out_g[3], out_b[3], out_opacity[3] };
    output[1] = output_temp;
}





VECC_END
