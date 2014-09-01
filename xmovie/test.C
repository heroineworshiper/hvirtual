#include "guicast.h"
#include "libmpeg3.h"

#define TESTW 720
#define TESTH 576
#define BITMAPW 720
#define BITMAPH 480

int main(int argc, char *argv[])
{
	mpeg3_t *file;
	BC_SubWindow *canvas;
	BC_Bitmap *bitmap = 0;
	Timer timer;
	int i, result, j;
	long total_frames = 0, fps_denominator = 0, total_frames2 = 0;
	double total_fps = 0;

	if(argc < 2) exit(1);

	if(!(file = mpeg3_open(argv[1]))) exit(1);
	mpeg3_set_cpus(file, 1);

	BC_Window window("Test", TESTW, TESTH, TESTW, TESTH);
	window.add_subwindow(canvas = new BC_SubWindow(10, 10, window.get_w() - 20, window.get_h()));
	printf("YUV possible %d\n", window.accel_available(BC_YUV420_601));
	canvas->start_video();
	timer.update();
	j = 0;
	bitmap = canvas->new_bitmap(BITMAPW, BITMAPH, BC_YUV420_601);

	mpeg3_seek_percentage(file, 0.10);
	do
	{
		if(bitmap->depth == BC_YUV420_601)
			mpeg3_read_yuvframe(file,
				(char*)bitmap->get_y_plane(),
				(char*)bitmap->get_u_plane(),
				(char*)bitmap->get_v_plane(),
				0,
				0,
				bitmap->get_w(),
				bitmap->get_h(),
				0);

		canvas->draw_bitmap(bitmap, 0);

		total_frames++;
		if(total_frames > 30)
		{
			printf("%.02f fps\n", 15000 / (double)timer.get_difference());
			total_fps += 15000 / (double)timer.get_difference();
			fps_denominator++;
			timer.update();
			total_frames = 0;
		}
		else
			total_frames++;

		total_frames2++;
	}while(!mpeg3_end_of_video(file, 0) && total_frames2 < 600);

	printf("Avg fps .02f\n", total_fps / fps_denominator);
	canvas->stop_video();
	mpeg3_close(file);
}
