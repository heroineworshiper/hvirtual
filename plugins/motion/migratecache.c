// concatenate a list of files into a single motion cache file




#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int used;
    int dx;
    int dy;
    float dr;
} row_t;

#define MAX_ROWS 1000000

void main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Usage: migratecache <motion files>\n");
        return;
    }
    
    row_t *rows = calloc(MAX_ROWS, sizeof(row_t));
    int total_rows = 0;
    
    int i;
    for(i = 1; i < argc; i++)
    {
        char *path = argv[i];
        
        if(strlen(path) < 7)
        {
            printf("%s is too short\n", path);
            return;
        }
        
        int is_motion = 0;
        int is_rotation = 0;
        if(path[0] == 'm')
        {
            is_motion = 1;
        }
        else
        if(path[0] == 'r')
        {
            is_rotation = 1;
        }
        else
        {
            printf("%s doesn't begin in m or r\n", path);
            return;
        }
        
// get the frame number
        int64_t frame = atol(path + 1);
        if(frame >= MAX_ROWS || frame < 0)
        {
            printf("frame %ld is out of bounds\n", frame);
            return;
        }
        int dx_result;
        int dy_result;
        float dr_result;
        
        FILE *input = fopen(path, "r");
        if(input)
        {
            if(is_motion)
            {
                int temp = fscanf(input, 
				    "%d %d", 
				    &dx_result,
				    &dy_result);
                rows[frame].dx = dx_result;
                rows[frame].dy = dy_result;
                rows[frame].used = 1;
            }
            else
            {
                int temp = fscanf(input, 
				    "%f", 
				    &dr_result);
                rows[frame].dr = dr_result;
                rows[frame].used = 1;
            }
            fclose(input);
            
            
            
        }
        else
        {
            printf("Couldn't open %s\n", path);
            return;
        }
        
    }
    
    
    printf("# frame, dx, dy, dr\n");
    for(i = 0; i < MAX_ROWS; i++)
    {
        if(rows[i].used)
        {
            printf("%d %d %d %f\n", i, rows[i].dx, rows[i].dy, rows[i].dr);
        }
    }
}
