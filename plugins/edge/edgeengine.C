#include "clip.h"
#include "edgeengine.h"
#include <math.h>


EdgePackage::EdgePackage()
 : LoadPackage()
{
}

EdgeUnit::EdgeUnit(EdgeEngine *server) : LoadClient(server)
{
	this->server = server;
}

EdgeUnit::~EdgeUnit() 
{
}


float EdgeUnit::edge_detect(float *data, float max, int do_max)
{
	const float v_kernel[9] = { 0,  0,  0,
                               0,  2, -2,
                               0,  2, -2 };
	const float h_kernel[9] = { 0,  0,  0,
                               0, -2, -2,
                               0,  2,  2 };
	int i;
	float v_grad, h_grad;
	float amount = server->amount;

	for (i = 0, v_grad = 0, h_grad = 0; i < 9; i++)
    {
    	v_grad += v_kernel[i] * data[i];
    	h_grad += h_kernel[i] * data[i];
    }

	float result = sqrt (v_grad * v_grad * amount +
            	 h_grad * h_grad * amount);
// TODO: max is -0x80 - 0x7f for UV channels
	if(do_max)
		CLAMP(result, 0, max);
	return result;
}

#define EDGE_MACRO(type, max, components, is_yuv) \
{ \
	type **input_rows = (type**)server->src->get_rows(); \
	type **output_rows = (type**)server->dst->get_rows(); \
	int comps = MIN(components, 3); \
	for(int y = pkg->y1; y < pkg->y2; y++) \
	{ \
		for(int x = 0; x < w; x++) \
		{ \
/* kernel is in bounds */ \
			if(y > 0 && x > 0 && y < h - 2 && x < w - 2) \
			{ \
				for(int chan = 0; chan < comps; chan++) \
				{ \
/* load kernel */ \
					for(int kernel_y = 0; kernel_y < 3; kernel_y++) \
					{ \
						for(int kernel_x = 0; kernel_x < 3; kernel_x++) \
						{ \
							kernel[3 * kernel_y + kernel_x] = \
								(type)input_rows[y - 1 + kernel_y][(x - 1 + kernel_x) * components + chan]; \
 \
 							if(is_yuv && chan > 0) \
							{ \
								kernel[3 * kernel_y + kernel_x] -= 0x80; \
							} \
 \
						} \
					} \
/* do the business */ \
					output_rows[y][x * components + chan] = edge_detect(kernel, max, sizeof(type) < 4); \
 					if(is_yuv && chan > 0) \
					{ \
						output_rows[y][x * components + chan] += 0x80; \
					} \
 \
				} \
 \
				if(components == 4) output_rows[y][x * components + 3] = \
					input_rows[y][x * components + 3]; \
			} \
			else \
			{ \
				for(int chan = 0; chan < comps; chan++) \
				{ \
/* load kernel */ \
					for(int kernel_y = 0; kernel_y < 3; kernel_y++) \
					{ \
						for(int kernel_x = 0; kernel_x < 3; kernel_x++) \
						{ \
							int in_y = y - 1 + kernel_y; \
							int in_x = x - 1 + kernel_x; \
							CLAMP(in_y, 0, h - 1); \
							CLAMP(in_x, 0, w - 1); \
							kernel[3 * kernel_y + kernel_x] = \
								(type)input_rows[in_y][in_x * components + chan]; \
 							if(is_yuv && chan > 0) \
							{ \
								kernel[3 * kernel_y + kernel_x] -= 0x80; \
							} \
						} \
					} \
/* do the business */ \
					output_rows[y][x * components + chan] = edge_detect(kernel, max, sizeof(type) < 4); \
 					if(is_yuv && chan > 0) \
					{ \
						output_rows[y][x * components + chan] += 0x80; \
					} \
				} \
				if(components == 4) output_rows[y][x * components + 3] = \
					input_rows[y][x * components + 3]; \
			} \
		} \
	} \
}


void EdgeUnit::process_package(LoadPackage *package)
{
	EdgePackage *pkg = (EdgePackage*)package;
	int w = server->src->get_w();
	int h = server->src->get_h();
	float kernel[9];
	
	switch(server->src->get_color_model())
	{
		case BC_RGB_FLOAT:
			EDGE_MACRO(float, 1, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			EDGE_MACRO(float, 1, 4, 0);
			break;
		case BC_RGB888:
			EDGE_MACRO(unsigned char, 0xff, 3, 0);
			break;
		case BC_YUV888:
			EDGE_MACRO(unsigned char, 0xff, 3, 1);
			break;
		case BC_RGBA8888:
			EDGE_MACRO(unsigned char, 0xff, 4, 0);
			break;
		case BC_YUVA8888:
			EDGE_MACRO(unsigned char, 0xff, 4, 1);
			break;
	}
}


EdgeEngine::EdgeEngine(int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
    tmp = 0;
    amount = 1;
}

EdgeEngine::~EdgeEngine()
{
    if(tmp)
    {
        delete tmp;
    }
}


void EdgeEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		EdgePackage *pkg = (EdgePackage*)get_package(i);
		pkg->y1 = src->get_h() * i / LoadServer::get_total_packages();
		pkg->y2 = src->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

void EdgeEngine::process(VFrame *dst, VFrame *src, int amount)
{
	this->dst = dst;
	this->src = src;
    this->amount = amount;
    if(dst == src)
    {
        if(!tmp)
        {
            tmp = new VFrame;
            tmp->set_use_shm(0);
            tmp->reallocate(
		        0,   // Data if shared
		        -1,             // shmid if IPC  -1 if not
		        0,         // plane offsets if shared YUV
		        0,
		        0,
		        src->get_w(), 
		        src->get_h(), 
		        src->get_color_model(), 
		        -1);        // -1 if unused
        }
        tmp->copy_from(src);
        this->src = tmp;
    }
	process_packages();
}


LoadClient* EdgeEngine::new_client()
{
	return new EdgeUnit(this);
}

LoadPackage* EdgeEngine::new_package()
{
	return new EdgePackage;
}


