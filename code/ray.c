F32 pixel_scale = 10.0f;

U32 sim_width = W_WIDTH / pixel_scale;
U32 sim_height = W_HEIGHT / pixel_scale;
U32 pixel_count = sim_width * sim_height;

float *texture_data = 0;
U32 texture_data_size = sizeof(float) * pixel_count * 4;

F32 RandF32()
{
    return (F32)rand() / (F32)RAND_MAX;
}

static void InitTextureData()
{
    texture_data = (float*)malloc(texture_data_size);
    
    for (int i = 0; i < pixel_count; i++)
    {
        Vec4F32 *pixel_data = (Vec4F32*)texture_data;
        
        pixel_data[i] = V4F32(RandF32(), RandF32(), RandF32(), 1.0f);
    }
}







// NOTE(randy): image import example
#if 0
int x,y,n;
unsigned char *img = stbi_load("code/yeet.bmp", &x, &y, &n, 0);

for (int i = 0; i < PIXEL_COUNT * 4; i++)
{
    texture_data[i] = img[i] / 255.0f;
}
#endif