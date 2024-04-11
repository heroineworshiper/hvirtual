#define CURVE_MAX 1.0
#define CURVE_MIN 0.0
#define MAX_POINTS 16


uniform sampler2D tex;
uniform vec2 points_r[MAX_POINTS];
uniform int total_r;
uniform vec2 points_g[MAX_POINTS];
uniform int total_g;
uniform vec2 points_b[MAX_POINTS];
uniform int total_b;
uniform vec2 points_v[MAX_POINTS];
uniform int total_v;

int min_int(int x, int y) 
{
    return x < y ? x : y;
}

int max_int(int x, int y) 
{
    return x > y ? x : y;
}

float calculate_bezier(int p1, 
    int p2, 
    int p3, 
    int p4, 
    vec2 points[MAX_POINTS],
    int total_points,
    float x)
{
    float x0, x3, y0, y1, y2, y3;
    float dx, dy;
    float slope;

    
/* the outer control points for the bezier curve. */
    x0 = points[p2].x;
    y0 = points[p2].y;
    x3 = points[p3].x;
    y3 = points[p3].y;

/*
 * the x values of the inner control points are fixed at
 * x1 = 2/3*x0 + 1/3*x3   and  x2 = 1/3*x0 + 2/3*x3
 * this ensures that the x values increase linearily with the
 * parameter t and enables us to skip the calculation of the x
 * values altogehter - just calculate y(t) evenly spaced.
 */

    dx = x3 - x0;
    dy = y3 - y0;

    if(dx <= 0.0) return x;

    if (p1 == p2 && p3 == p4)
    {
/* No information about the neighbors,
 * calculate y1 and y2 to get a straight line
 */
        y1 = y0 + dy / 3.0;
        y2 = y0 + dy * 2.0 / 3.0;
    }
    else 
    if (p1 == p2 && p3 != p4)
    {
      /* only the right neighbor is available. Make the tangent at the
       * right endpoint parallel to the line between the left endpoint
       * and the right neighbor. Then point the tangent at the left towards
       * the control handle of the right tangent, to ensure that the curve
       * does not have an inflection point.
       */
        slope = (points[p4].y - y0) / (points[p4].x - x0);

        y2 = y3 - slope * dx / 3.0;
        y1 = y0 + (y2 - y0) / 2.0;
    }
    else 
    if (p1 != p2 && p3 == p4)
    {
        /* see previous case */
        slope = (y3 - points[p1].y) / (x3 - points[p1].x);

        y1 = y0 + slope * dx / 3.0;
        y2 = y3 + (y1 - y3) / 2.0;
    }
    else /* (p1 != p2 && p3 != p4) */
    {
        /* Both neighbors are available. Make the tangents at the endpoints
         * parallel to the line between the opposite endpoint and the adjacent
         * neighbor.
         */
        slope = (y3 - points[p1].y) / (x3 - points[p1].x);

        y1 = y0 + slope * dx / 3.0;

        slope = (points[p4].y - y0) / (points[p4].x - x0);

        y2 = y3 - slope * dx / 3.0;
    }

    /*
     * finally calculate the y(t) values for the given bezier values. We can
     * use homogenously distributed values for t, since x(t) increases linearily.
     */
    float y, t;

    t = (x - x0) / dx;
    y =     y0 * (1.0-t) * (1.0-t) * (1.0-t) +
        3.0 * y1 * (1.0-t) * (1.0-t) * t     +
        3.0 * y2 * (1.0-t) * t     * t     +
            y3 * t     * t     * t;

    return y;
}






float calculate_level(float input, 
    vec2 points[MAX_POINTS],
    int total_points)
{
    if(total_points == 0)
    {
        return input;
    }

// find points bordering input
    int i;
    for(i = total_points - 1; i >= 0; i--)
    {
        if(points[i].x <= input)
        {
            break;
        }
    }

// Boundaries
    if(i >= total_points - 1)
        return points[total_points - 1].y;
    if(i < 0)
        return points[0].y;

// from gimp_curve_calculate
    int p1 = max_int(i - 1, 0);
    int p2 = i;
    int p3 = i + 1;
    int p4 = min_int(i + 2, total_points - 1);

    float output = calculate_bezier(p1, p2, p3, p4, points, total_points, input);
    output = clamp(output, CURVE_MIN, CURVE_MAX);


    return output;
}


void main()
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
    color = yuv_to_rgb(color);

    color.r = calculate_level(color.r, points_r, total_r);
// no recursion in GLSL
    color.r = calculate_level(color.r, points_v, total_v);

    color.g = calculate_level(color.g, points_g, total_g);
    color.g = calculate_level(color.g, points_v, total_v);

    color.b = calculate_level(color.b, points_b, total_b);
    color.b = calculate_level(color.b, points_v, total_v);


    gl_FragColor = rgb_to_yuv(color);
}



