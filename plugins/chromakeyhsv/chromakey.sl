uniform sampler2D tex;
uniform float red;
uniform float green;
uniform float blue;
uniform float in_slope;
uniform float out_slope;
uniform float tolerance;
uniform float tolerance_in;
uniform float tolerance_out;
uniform float sat;
uniform float min_s;
uniform float min_s_in;
uniform float min_s_out;
uniform float min_v;
uniform float min_v_in;
uniform float min_v_out;
uniform float max_v;
uniform float max_v_in;
uniform float max_v_out;
uniform float spill_threshold;
uniform float spill_amount;
uniform bool scale_spill;
uniform float alpha_offset;
uniform float hue_key;
uniform float saturation_key;
uniform float value_key;

void main()
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
/* Contribution to alpha from each component */
	float ah = 1.0;
	float as = 1.0;
	float av = 1.0;
    float a = 1.0;
	vec4 color2;
	
/* Convert to HSV */
	color2 = yuv_to_rgb(color);
	color2 = rgb_to_hsv(color2);
    float h = color2.r;
    float s = color2.g;
    float v = color2.b;

/* Get the difference between the current hue & the hue key */
	float h_diff = h - hue_key;
    if(h_diff < -180.0) h_diff += 360.0;
    else
    if(h_diff > 180.0) h_diff -= 360.0;
    h_diff = abs(h_diff);

// alpha contribution from hue difference
// outside wedge < tolerance_out < tolerance_in < inside wedge < tolerance_in < tolerance_out < outside wedge
	if (tolerance_out > 0.0)
    {
// completely inside the wedge
		if (h_diff < tolerance_in)
			ah = 0.0;
        else
// between the outer & inner slope
 		if(h_diff < tolerance_out)
			ah = (h_diff - tolerance_in) / (tolerance_out - tolerance_in);
        if(ah > 1.0) ah = 1.0;
    }

// alpha contribution from saturation
// outside wedge < min_s_out < min_s_in < inside wedge
    float s2 = s - sat;
    if(s2 > min_s_out)
    {
// saturation with offset applied
// completely inside the wedge
        if(s2 > min_s_in)
            as = 0.0;
// inside the gradient
        if(s2 >= min_s_out)
			as = (min_s_in - s2) / (min_s_in - min_s_out);
    }


// alpha contribution from brightness range
// brightness range is defined by 4 in/out variables
// outside wedge < min_v_out < min_v_in < inside wedge < max_v_in < max_v_out < outside wedge
    if(v > min_v_out)
    {
        if(v < min_v_in)
            av = (min_v_in - v) / (min_v_in - min_v_out);
        else
        if(v <= max_v_in)
            av = 0.0;
        else
        if(v <= max_v_out)
            av = (v - max_v_in) / (max_v_out - max_v_in);
    }

// combine the alpha contribution of every component into a single alpha
    a = max(as, ah);
    a = max(a, av);

// Spill light processing
// desaturate a wedge around the hue key
// hue_key/s=0 < spill_amount < spill_threshold < no spill/s=1
	if (h_diff < spill_threshold)
	{
        float s_scale = 0.0;
        if(h_diff > spill_threshold)
            s_scale = 1.0;
        else
        if(!scale_spill && h_diff > spill_amount)
            s_scale = (h_diff - spill_amount) / (spill_threshold - spill_amount);
        else
        if(scale_spill)
            s_scale = h_diff / spill_threshold;

        if(scale_spill)
        {
            float s2 = s * s_scale;
            s = s * (1.0 - spill_amount) + s2 * spill_amount;
        }
        else
            s *= s_scale;

/* convert back to native colormodel */
        color2.r = h;
        color2.g = s;
        color2.b = v;
		color2 = hsv_to_rgb(color2);
		color.rgb = rgb_to_yuv(color2).rgb;
    }

	a += alpha_offset;
    a = min(a, 1.0);
    color.a = a;
/* Convert mask into image */
	gl_FragColor = show_mask(color, color2);
}


