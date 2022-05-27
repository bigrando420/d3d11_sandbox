const F32 pixel_scale = 10.0f;

const U32 sim_width = W_WIDTH / pixel_scale;
const U32 sim_height = W_HEIGHT / pixel_scale;
const U32 pixel_count = sim_width * sim_height;

float *texture_data = 0;
U32 texture_data_size = sizeof(float) * pixel_count * 4;

typedef enum PixelType
{
    PIXEL_TYPE_boundary = -1,
    PIXEL_TYPE_air = 0,
    PIXEL_TYPE_sand,
    PIXEL_TYPE_water,
    PIXEL_TYPE_MAX,
} PixelType;

struct Pixel
{
    PixelType type;
};

Pixel pixels[sim_width][sim_height];

F32 RandF32()
{
    return (F32)rand() / (F32)RAND_MAX;
}

static void FillPixelsRandomly()
{
    for (int y = 0; y < sim_height; y++)
    {
        for (int x = 0; x < sim_width; x++)
        {
            pixels[x][y].type = (PixelType)(rand() % 3);
        }
    }
}

static void UpdateTextureData()
{
    Vec4F32 *pixel_data = (Vec4F32*)texture_data;
    
    for (int y = 0; y < sim_height; y++)
        for (int x = 0; x < sim_width; x++)
    {
        Vec4F32 *pixel_col = &pixel_data[y*sim_width + x];
        
        switch (pixels[x][y].type)
        {
            case PIXEL_TYPE_air:
            {
                *pixel_col = V4F32(180.0f / 255.0f, 203.0f / 255.0f, 240.0f / 255.0f, 1.0f);
            } break;
            
            case PIXEL_TYPE_sand:
            {
                *pixel_col = V4F32(242.0f / 255.0f, 216.0f / 255.0f, 145.0f / 255.0f, 1.0f);
            } break;
            
            case PIXEL_TYPE_water:
            {
                *pixel_col = V4F32(74.0f / 255.0f, 147.0f / 255.0f, 224.0f / 255.0f, 1.0f);
            } break;
        }
    }
}

Pixel boundary_pixel = {PIXEL_TYPE_boundary};

static Pixel *PixelAt(S32 x, S32 y)
{
    if (x < 0 || x >= sim_width ||
        y < 0 || y >= sim_height)
        return &boundary_pixel;
    
    return &pixels[x][y];
}

static void SwapPixels(Pixel *from, Pixel *to)
{
    Pixel t = *to;
    *to = *from;
    *from = t;
}

static void UpdateSim()
{
    for (int y = 0; y < sim_height; y++)
        for (int x = 0; x < sim_width; x++)
    {
        Pixel *pixel = &pixels[x][y];
        switch (pixel->type)
        {
            case PIXEL_TYPE_sand:
            {
                Pixel *down = PixelAt(x, y-1);
                
                if (down->type == PIXEL_TYPE_air)
                    SwapPixels(pixel, down);
                
            } break;
            
            case PIXEL_TYPE_water:
            {
                
            } break;
        }
    }
}

static void Init()
{
    texture_data = (float*)malloc(texture_data_size);
    
    FillPixelsRandomly();
    UpdateTextureData();
}

static void Update()
{
    UpdateSim();
    UpdateTextureData();
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