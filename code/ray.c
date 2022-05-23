#define W_WIDTH 800
#define W_HEIGHT 600
#define PIXEL_COUNT W_WIDTH * W_HEIGHT

float *texture_data = 0;
float aspect_ratio = (F32)W_WIDTH / (F32)W_HEIGHT;

typedef struct Ray
{
    Vec3F32 origin;
    Vec3F32 dir;
} Ray;

static Vec3F32 RayAt(Ray r, F32 t)
{
    return Add3F32(r.origin, Scale3F32(r.dir, t));
}

static Vec4F32 RayColor(Ray *r)
{
    Vec3F32 unit_dir = Normalize3F32(r->dir);
    F32 t = (unit_dir.y + 1.0f) * 0.5f;
    
    Vec3F32 col = Add3F32(Scale3F32(V3F32(1.0f, 1.0f, 1.0f), 1.0f - t),
                          Scale3F32(V3F32(0.5f, 0.7f, 1.0f), t));
    
    return V4F32(col.r, col.g, col.b, 1.0f);
}

static void InitRay()
{
    texture_data = (float*)malloc(sizeof(float) * PIXEL_COUNT * 4);
    
    // camera
    F32 view_height = 2.0f;
    F32 view_width = view_height * aspect_ratio;
    F32 focal_length = 1.0f; // length to the far plane?
    
    Vec3F32 origin = {0.0f, 0.0f, 0.0f};
    Vec3F32 horizontal_point = {view_width, 0.0f, 0.0f};
    Vec3F32 vertical_point = {0.0f, view_height, 0.0f};
    Vec3F32 lower_left_corner = Sub3F32(origin,
                                        V3F32(view_width / 2.0f,
                                              view_height / 2.0f,
                                              focal_length));
    // NOTE(randy): Might need to break out the lower left corner declartion properly (it was just too many vector operations lol)
    
    FILE *f = 0;
    f = fopen("test_img.ppm", "w");
    
    fprintf(f, "P3\n%i %i\n255\n", W_WIDTH, W_HEIGHT);
    
    for (int j = W_HEIGHT - 1; j>= 0; --j)
    {
        for (int i = 0; i < W_WIDTH; ++i)
        {
            F32 u = (F32)i / ((F32)W_WIDTH - 1.0f);
            F32 v = (F32)j / ((F32)W_HEIGHT - 1.0f);
            
            Ray ray = {0};
            ray.origin = origin;
            ray.dir = Sub3F32(Add3F32(lower_left_corner,
                                      V3F32(u * view_width,
                                            v * view_height,
                                            0.0f)),
                              origin);
            
            Vec4F32 col = RayColor(&ray);
            
            Vec4F32 *texture_data_vec = (Vec4F32*)texture_data;
            
            int pixel_index = j * W_WIDTH + i;
            
            //col = V4F32(0.5f, 0.7f, 1.0f, 0.0f);
            texture_data_vec[pixel_index] = col;
            
            int r = 255.0f * col.r;
            int g = 255.0f * col.g;
            int b = 255.0f * col.b;
            
            fprintf(f, "%i %i %i\n", r, g, b);
        }
    }
    
    fclose(f);
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