uniform sampler2D tex;
uniform float red;
uniform float green;
uniform float blue;
uniform float in_slope;
uniform float out_slope;
uniform float tolerance;
uniform float tolerance_in;
uniform float tolerance_out;
uniform float sat_x;
uniform float sat_y;
uniform float min_s;
uniform float min_s_in;
uniform float min_s_out;
uniform float min_v;
uniform float min_v_in;
uniform float min_v_out;
uniform float max_v;
uniform float max_v_in;
uniform float max_v_out;
uniform float spill_distance;
uniform float spill_x;
uniform float spill_y;
uniform float spill_tolerance;
uniform float min_h;
uniform float max_h;
uniform bool desaturate_only;
uniform float alpha_offset;
uniform float hue_key;
uniform float saturation_key;
uniform float value_key;



// shortest distance between 2 hues
float hue_distance(float h1, float h2)
{
	float result = h1 - h2;
    if(result < -180.0) result += 360.0;
    else
    if(result > 180.0) result -= 360.0;
    return result;
}

// shift H & S based on an X & Y offset
void shift_hs(out float h_shifted, 
    out float s_shifted, 
    float h, 
    float s, 
    float x_offset,
    float y_offset)
{
    float h_rad = radians(h);
    float x = cos(h_rad) * s;
    float y = sin(h_rad) * s;
    x += x_offset;
    y += y_offset;
    h_shifted = degrees(atan(y, x));
    s_shifted = length(vec2(x, y));
}


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

// shift the color in XY to shift the wedge point
    float h_shifted, s_shifted;
    shift_hs(h_shifted, 
        s_shifted, 
        h, 
        s, 
        sat_x,
        sat_y);

/* Get the difference between the current hue & the hue key */
	float h_diff = abs(hue_distance(h_shifted, hue_key));

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
    if(s_shifted > min_s_out)
    {
// saturation with offset applied
// completely inside the wedge
        if(s_shifted > min_s_in)
            as = 0.0;
// inside the gradient
        if(s_shifted >= min_s_out)
			as = (min_s_in - s_shifted) / (min_s_in - min_s_out);
    }


// alpha contribution from brightness range
// brightness range is defined by 4 in/out variables
// outside wedge < min_v_out < min_v_in < inside wedge < max_v_in < max_v_out < outside wedge
    if(v > min_v_out)
    {
        if(v < min_v_in || max_v_in >= 1.0)
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
    if(spill_tolerance > 0.0)
    {
// get the difference between the shifted input color to the unshifted spill wedge
        if(spill_distance != 0.0)
        {
            shift_hs(h_shifted, 
                s_shifted, 
                h, 
                s, 
                spill_x,
                spill_y);
        }
        else
        {
            h_shifted = h;
            s_shifted = s;
        }

// Difference between the shifted hue & the unshifted hue key
		h_diff = hue_distance(h_shifted, hue_key);

// inside the wedge
        if(abs(h_diff) < spill_tolerance)
        {
            if(!desaturate_only)
            {
// the shifted input color in the unshifted wedge
// gives 2 unshifted border colors & the weighting
                float blend = 0.5 + h_diff / spill_tolerance / 2.0;
// shift the 2 border colors to the output wedge
                float min_h_shifted;
                float min_s_shifted;
                shift_hs(min_h_shifted, 
                    min_s_shifted, 
                    min_h, 
                    s_shifted, 
                    -spill_x,
                    -spill_y);
                float max_h_shifted;
                float max_s_shifted;
                shift_hs(max_h_shifted, 
                    max_s_shifted, 
                    max_h, 
                    s_shifted, 
                    -spill_x,
                    -spill_y);


// blend the shifted border colors using the unshifted weighting
// the only thing which doesn't restore the key color & doesn't make an edge is
// fading the saturation to 0 in the middle
                if(blend > 0.5)
                {
                    h = max_h_shifted;
                    s = max_s_shifted;
                }
                else
                {
                    h = min_h_shifted;
                    s = min_s_shifted;
                }
            } 
            else // !desaturate_only
            {

// fade the saturation to 0 in the middle
                s *= abs(h_diff) / spill_tolerance;
            }


            if(h < 0.0) h += 360.0;

// store new color
            color2.r = h;
            color2.g = s;
            color2.b = v;
		    color2 = hsv_to_rgb(color2);
		    color.rgb = rgb_to_yuv(color2).rgb;
        }
    }



	a += alpha_offset;
    a = min(a, 1.0);
    color.a = a;
/* Convert mask into image */
	gl_FragColor = show_mask(color, color2);
}


