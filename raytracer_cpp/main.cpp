#include "common.h"

#include "writebitmap.h"
#include "sceneobjects.h"
#include "camera.h"

const int ImageWidth = 512;
const int ImageHeight = 512;
const float FieldOfView = 60.0f;
const int MaxDepth = 3;

// A list of objects in the scene
std::vector<std::shared_ptr<SceneObject>> sceneObjects;

// The camera
std::shared_ptr<Camera> pCamera;

void InitScene()
{
    pCamera = std::make_shared<Camera>(vec3(0.0f, 6.0f, 8.0f),      // Where the camera is
        vec3(0.0f, -.8f, -1.0f),                                    // The point it is looking at
        FieldOfView,                                                // The field of view of the 'lens'
        ImageWidth, ImageHeight);                                   // The size in pixels of the view plane

    // Red ball
    Material mat;
    mat.albedo = vec3(.7f, .1f, .1f);
    mat.specular = vec3(.9f, .1f, .1f);
    mat.reflectance = 0.5f;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, vec3(0.0f, 2.0f, 0.f), 2.0f));

    // Purple ball
    mat.albedo = vec3(0.7f, 0.0f, 0.7f);
    mat.specular = vec3(0.9f, 0.9f, 0.8f);
    mat.reflectance = 0.5f;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, vec3(-2.5f, 1.0f, 2.f), 1.0f));

    // Blue ball
    mat.albedo = vec3(0.0f, 0.3f, 1.0f);
    mat.specular = vec3(0.0f, 0.0f, 1.0f);
    mat.reflectance = 0.0f;
    mat.emissive = vec3(0.0f, 0.0f, 0.0f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, vec3(-0.0f, 0.5f, 3.f), 0.5f));

    // Yellow ball on floor
    mat.albedo = vec3(1.0f, 1.0f, 1.0f);
    mat.specular = vec3(0.0f, 0.0f, 0.0f);
    mat.reflectance = .0f;
    mat.emissive = vec3(1.0f, 1.0f, 0.2f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, vec3(2.8f, 0.8f, 2.0f), 0.8f));

    // White light (distant)
    mat.albedo = vec3(0.0f, 0.8f, 0.0f);
    mat.specular = vec3(0.0f, 0.0f, 0.0f);
    mat.reflectance = 0.0f;
    mat.emissive = vec3(1.0f, 1.0f, 1.0f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, vec3(-10.8f, 6.4f, 10.0f), 0.4f));

    // Plane on the ground
    sceneObjects.push_back(std::make_shared<TiledPlane>(vec3(0.0f, 0.0f, 0.0f), normalize(vec3(0.0f, 1.0f, 0.0f))));
}

std::shared_ptr<SceneObject> FindFirstIntersector(const vec3 from, const vec3 dir, float& dst)
{
    float minDistance = FLT_MAX;
    std::shared_ptr<SceneObject> pObject;

    for (const auto& obj : sceneObjects)
    {
        float distance;
        if (obj->Intersects(from, dir, distance))
        {
            if (distance < minDistance)
            {
                pObject = obj;
                minDistance = distance;
            }

        }
    }

    dst = minDistance;
    return pObject;
}

// Trace a ray into the scene, return the accumulated light value
vec3 TraceRay(const vec3& rayorig, const vec3 &raydir, const int depth)
{
    // Here's how to print out a vector for inspection
    //std::cout << "Ray: " << glm::to_string(raydir) << std::endl;


    auto minDistance = 0.0f;
    auto obj = FindFirstIntersector(rayorig, raydir, minDistance);

    auto interPos = rayorig + glm::normalize(raydir) * minDistance;
    auto normal = obj->GetSurfaceNormal(interPos);
    auto albedo = obj->GetMaterial(interPos).albedo;

    auto color = obj->GetMaterial(vec3(0, 0, 0)).emissive;

    //color = vec3(0, 0, 0);
    for (const auto& light : sceneObjects)
    {
        vec3 rayFrom = light->GetRayFrom(interPos);
        rayFrom = glm::normalize(rayFrom);

        float dis;
        auto occluder = FindFirstIntersector(interPos + rayFrom * 0.001f, rayFrom, dis);
        auto occlusionOccurred = occluder != light;

        if (!occlusionOccurred)
        {
            auto intensity = std::max(glm::dot(normal, rayFrom), 0.0f);
            color += albedo * light->GetMaterial(vec3(0, 0, 0)).emissive * intensity;

            auto reflection = glm::reflect(raydir, normal);
            auto specular_intensity = glm::dot(reflection, rayFrom);
            specular_intensity = specular_intensity * specular_intensity;
            color += obj->GetMaterial(vec3(0, 0, 0)).specular * specular_intensity;
        }
    }


    // For now, just convert the incoming ray to a 'color' to display it has it changes 
    return color;
}

// Draw the scene by generating camera rays and tracing rays
void DrawScene(Bitmap* pBitmap)
{
    for (int y = 0; y < ImageHeight; y++)
    {
        for (int x = 0; x < ImageWidth; x++)
        {
            // Make a ray pointing into the center of the pixel.
            auto ray = pCamera->GetWorldRay(vec2((float)x + .5f, (float)y + .5f));

            // Trace the ray and get the color
            auto color = TraceRay(pCamera->position, ray, 0);

            // Color might have maxed out, so clamp.
            // Also convert it to 8 bit color
            color = color * 255.0f;
            color = clamp(color, vec3(0.0f, 0.0f, 0.0f), vec3(255.0f, 255.0f, 255.0f));

            // Draw it on the bitmap
            PutPixel(pBitmap, x, y, Color{ uint8_t(color.x), uint8_t(color.y),uint8_t(color.z) });
        }
    }
}

int main()
{
    Bitmap* pBitmap = CreateBitmap(ImageWidth, ImageHeight);

    Color col{ 127, 127, 127 };
    ClearBitmap(pBitmap, col);

    InitScene();

    DrawScene(pBitmap);

    const char* pImage = "image.bmp";
    WriteBitmap(pBitmap, pImage);

    DestroyBitmap(pBitmap);

#ifdef _MSC_VER
    system("start image.bmp");
#else
    // Requires ImageMagick?
    system("display image.bmp");
#endif
    return 0;
}
