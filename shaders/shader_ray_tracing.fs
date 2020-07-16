#version 330
out vec4 FragColor;

in vec2 TexCoords;


// You can change the code whatever you want


const int MAX_DEPTH = 1; // maximum bounce
const int INFINITY = 10000000;
const bool usingFresnel = true;
uniform samplerCube environmentMap;


struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Material {
    // phong shading coefficients
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;

    // reflect / refract
    vec3 R0; // Schlick approximation
    float ior; // index of refration

    // for refractive material
    vec3 extinction_constant;
    vec3 shadow_attenuation_constant;

    

    // 0 : phong
    // 1 : refractive dielectric
    // add more
    int material_type;
};

const int material_type_phong = 0;
const int material_type_refractive = 1;

// Just consider point light
struct Light{
    vec3 position;
    vec3 color;
    bool castShadow;
};
uniform vec3 ambientLightColor;

// hit information
struct HitRecord{
    float t;        // distance to hit point
    vec3 p;         // hit point
    vec3 normal;    // hit point normal
    Material mat;   // hit point material
};

// Geometry
struct Sphere {
    vec3 center;
    float radius;
    Material mat;
};

struct Plane {
    vec3 normal;
    vec3 p0;
    Material mat;
};

struct Box {
    vec3 box_min;
    vec3 box_max;
    Material mat;
};

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
    // we didn't add material to triangle because it requires so many uniform memory when we use mesh...
};


const int mat_phong = 0;
const int mat_refractive = 0;


uniform Material material_ground;
uniform Material material_box;
uniform Material material_gold;
uniform Material material_dielectric_glass;
uniform Material material_mirror;
uniform Material material_lambert;
uniform Material material_mesh;

uniform vec3 current_pos;
uniform float mask_mode;
uniform float start;


Sphere spheres[] = Sphere[](
  Sphere(current_pos, 0.5, material_gold)
);

Box boxes[] = Box[](//assume that all boxes are uniform cube
  //Box(vec3(0,0,0), vec3(0.5,1,0.5), dielectric),
  Box(vec3(5,-5,-5), vec3(15,5,5), material_box),
  Box(vec3(-5,-5,5), vec3(5,5,15), material_box),
  Box(vec3(-15,-5,-5), vec3(-5,5,5), material_box),
  Box(vec3(-5,-5,-15), vec3(5,5,-5), material_box),

  Box(vec3(-15,-5,-15), vec3(-5,5,-5), material_box),
  Box(vec3(5,-5,-15), vec3(15,5,-5), material_box),
  Box(vec3(-15,-5,5), vec3(-5,5,15), material_box),
  Box(vec3(5,-5,5), vec3(15,5,15), material_box)



);

Plane groundPlane = Plane(vec3(0,1,0), vec3(0), material_ground);
Triangle mirrorTriangle = Triangle( vec3(-3,0,0), vec3(0,0,-4), vec3(-1, 4, -2));
Light lights[] = Light[](
    Light(vec3(0,5,0), vec3(1,1,1), true)
//     ,Light(vec3(-3,5,3), vec3(0.5,0,0), false),
//     Light(vec3(-3,5,-3), vec3(0,0.5,0), false),
//     Light(vec3(3,5,-3), vec3(0,0,0.5), false)


);

// use this for mesh
/*
layout (std140) uniform mesh_vertices_ubo
{
    vec3 mesh_vertices[500];
};

layout (std140) uniform mesh_tri_indices_ubo
{
    vec3 mesh_tri_indices[500];
};

uniform int meshTriangleNumber;*/

// Math functions
/* returns a varying number between 0 and 1 */
float rand(vec2 co) {
  return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float max3 (vec3 v) {
  return max (max (v.x, v.y), v.z);
}

float min3 (vec3 v) {
  return min (min (v.x, v.y), v.z);
}

uniform vec3 cameraPosition;
uniform mat3 cameraToWorldRotMatrix;
uniform float fovY; //set to 45
uniform float H;
uniform float W;

Ray getRay(vec2 uv){
    // TODO:
    float imageAspectRatio = W/H;
    float Px = (2*uv.x - 1) * tan(radians(fovY / 2 )) * imageAspectRatio;
    float Py = (2*uv.y - 1) * tan(radians(fovY / 2));
    vec3 rayDirection = cameraToWorldRotMatrix * vec3(Px, Py, -1);
    rayDirection = normalize(rayDirection);
    return Ray(cameraPosition, rayDirection);

}


const float bias = 0.0001; // to prevent point too close to surface.

bool sphereIntersect(Sphere sp, Ray r, inout HitRecord hit){
    // TODO:
  vec3 oc = r.origin - sp.center;
  float a = dot(r.direction, r.direction);
  float b = dot(oc, r.direction);
  float c = dot(oc, oc) - sp.radius * sp.radius;
  float discriminant = b*b - a*c;
  if (discriminant > 0) {
    float temp = (-b - sqrt(b*b-a*c)) /a;
    if (temp < INFINITY && temp > bias) {
      hit.t = temp;
      hit.p = r.origin + temp * r.direction;
      hit.normal = (hit.p - sp.center) / sp.radius;
      hit.mat = sp.mat;
      return true;
    }
    temp = (-b + sqrt(b*b-a*c)) /a;
    if (temp < INFINITY && temp > bias) {
      hit.t = temp;
      hit.p = r.origin + temp * r.direction;
      hit.normal = (hit.p - sp.center) / sp.radius;
      hit.normal = normalize(hit.normal);
      hit.mat = sp.mat;
      return true;
    }
  }
  return false;
}

bool planeIntersect(Plane p, Ray r, inout HitRecord hit){
    // TODO:
    float p0 = -dot(p.p0,p.normal);
     float t = -(dot(p.normal, r.origin) + p0) / dot(p.normal, r.direction);
     if (t < bias || t > INFINITY) return false;
     hit.t = t;
     hit.p = r.origin + t * r.direction;
     hit.mat = p.mat;
     hit.normal = p.normal;
     return true;
    
}

bool boxIntersect(Box b, Ray r, inout HitRecord hit){
    // TODO:
    vec3 invdir = vec3(1/r.direction.x, 1/r.direction.y, 1/r.direction.z);
    vec3 nmin, nmax;
    int sign[3];
    sign[0] = int(invdir.x < 0);
    sign[1] = int(invdir.y < 0);
    sign[2] = int(invdir.z < 0);

    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    vec3 bounds[2];
    bounds[0] = b.box_min;
    bounds[1] = b.box_max;

    tmin = (bounds[sign[0]].x - r.origin.x) * invdir.x;
    nmin = (sign[0]==0)? vec3(-1,0,0) : vec3(1,0,0);
    tmax = (bounds[1-sign[0]].x - r.origin.x) * invdir.x;
    nmax = (sign[0]!=0)? vec3(-1,0,0) : vec3(1,0,0);
    tymin = (bounds[sign[1]].y - r.origin.y) * invdir.y;
    tymax = (bounds[1-sign[1]].y - r.origin.y) * invdir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin){
        tmin = tymin;
        nmin = (sign[1]==0)? vec3(0,-1,0) : vec3(0,1,0);
    }
    if (tymax < tmax){
        tmax = tymax;
        nmax = (sign[1]!=0)? vec3(0,-1,0) : vec3(0,1,0);
    }

    tzmin = (bounds[sign[2]].z - r.origin.z) * invdir.z;
    tzmax = (bounds[1-sign[2]].z - r.origin.z) * invdir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin){
        tmin = tzmin;
        nmin = (sign[2]==0)? vec3(0,0,-1) : vec3(0,0,1);
    }

    if (tzmax < tmax){
        tmax = tzmax;
        nmax = (sign[2]!=0)? vec3(0,0,-1) : vec3(0,0,1);
    }


    if( tmin < 0 ){
        hit.t = tmax;
        if (hit.t < 0) return false;
        hit.t = tmax;
        hit.normal = nmax;
    }
    else{
        hit.t = tmin;
        hit.normal = nmin;
    }

    hit.p = r.origin + hit.t * r.direction;
    hit.mat = b.mat;
    return true;
}

bool triangleIntersect(Triangle tri, Ray r, inout HitRecord hit){
    // TODO:

    // compute plane's normal
    vec3 v0v1 = tri.v1 - tri.v0;
    vec3 v0v2 = tri.v2 - tri.v0;
    vec3 N = normalize(cross(v0v1, v0v2));
    float denom = dot(N,N);
    float t, u, v;

    // Step 1: finding Plane

    // check if ray and plane are parallel ?
    float NdotRayDirection = dot(N,r.direction);
    float abs_NdotRayDirection = (NdotRayDirection > 0)? NdotRayDirection : -NdotRayDirection;
    if(abs_NdotRayDirection < bias)
        return false;

    // compute d parameter using equation 2
    float d = dot(N, tri.v0);

    // compute t (equation 3)
    t = (-dot(N, r.origin) + d) / NdotRayDirection;

    if (t < 0 ) return false;

    vec3 P = r.origin + t * r.direction;

    //step 2 : inside-outside test
    vec3 C;

    vec3 edge0 = tri.v1 - tri.v0;
    vec3 vp0 = P - tri.v0;
    C = cross(edge0, vp0);
    if (dot(N,C) < 0 ) return false;

    vec3 edge1 = tri.v2 - tri.v1;
    vec3 vp1 = P - tri.v1;
    C = cross(edge1, vp1);
    u = dot(N,C);
    if ((dot(N,C) < 0)) return false;

    vec3 edge2 = tri.v0 - tri.v2;
    vec3 vp2 = P - tri.v2;
    C = cross(edge2, vp2);
    v = dot(N, C);
    if ((dot(N, C) < 0)) return false;

    u /= denom;
    v /= denom;

    hit.t = t;
    hit.p = P;
    hit.normal = N;
    hit.mat.Ka = vec3(0);
    hit.mat.Kd = vec3(0);
    hit.mat.Ks = vec3(0);
    hit.mat.R0 = vec3(1);
    hit.mat.material_type = 0;
    return true;



}

bool meshIntersect(Triangle tri, Ray r, inout HitRecord hit){
    // TODO:

    // compute plane's normal
    vec3 v0v1 = tri.v1 - tri.v0;
    vec3 v0v2 = tri.v2 - tri.v0;
    vec3 N = normalize(cross(v0v1, v0v2));
    float denom = dot(N,N);
    float t, u, v;

    // Step 1: finding Plane

    // check if ray and plane are parallel ?
    float NdotRayDirection = dot(N,r.direction);
    float abs_NdotRayDirection = (NdotRayDirection > 0)? NdotRayDirection : -NdotRayDirection;
    if(abs_NdotRayDirection < bias)
        return false;

    // compute d parameter using equation 2
    float d = dot(N, tri.v0);

    // compute t (equation 3)
    t = (-dot(N, r.origin) + d) / NdotRayDirection;

    if (t < 0 ) return false;

    vec3 P = r.origin + t * r.direction;

    //step 2 : inside-outside test
    vec3 C;

    vec3 edge0 = tri.v1 - tri.v0;
    vec3 vp0 = P - tri.v0;
    C = cross(edge0, vp0);
    if (dot(N,C) < 0 ) return false;

    vec3 edge1 = tri.v2 - tri.v1;
    vec3 vp1 = P - tri.v1;
    C = cross(edge1, vp1);
    u = dot(N,C);
    if ((dot(N,C) < 0)) return false;

    vec3 edge2 = tri.v0 - tri.v2;
    vec3 vp2 = P - tri.v2;
    C = cross(edge2, vp2);
    v = dot(N, C);
    if ((dot(N, C) < 0)) return false;

    u /= denom;
    v /= denom;

    hit.t = t;
    hit.p = P;
    hit.normal = N;
    hit.mat.Ka = material_mesh.Ka;
    hit.mat.Kd = material_mesh.Kd;
    hit.mat.Ks = material_mesh.Ks;
    hit.mat.R0 = material_mesh.R0;
    hit.mat.material_type = material_mesh.material_type;
    return true;



}

float schlick(float cosine, float eta) {
    // TODO:

    return 1 - eta * eta * (1 - cosine * cosine);
}

vec3 schlick(float cosine, vec3 r0) {
    // TODO:
    return r0 + (1 - r0) * pow(1.0 - cosine, 5.0 );
}


bool trace(Ray r, inout HitRecord hit){
    // TODO: trace single ray.
    HitRecord temp_hit;
    bool hit_anything = false;
    float closest_so_far = INFINITY;

    //Sphere
    if(start > 0.5){
        for(int i = 0; i < spheres.length(); i++){
            if (sphereIntersect(spheres[i], r, temp_hit)){
                if(temp_hit.t < hit.t){
                    hit_anything = true;
                    hit = temp_hit;
                    closest_so_far = temp_hit.t;
                }
            }
        }
    }


    //Box
    for(int i = 0; i < boxes.length(); i++){
        if(boxIntersect(boxes[i], r, temp_hit)){
            if(temp_hit.t < hit.t){
                hit_anything = true;
                hit = temp_hit;
                closest_so_far = temp_hit.t;
            }
        }
    }
//
//     if (triangleIntersect(mirrorTriangle,r,temp_hit)){
//         if(temp_hit.t < hit.t){
//             hit_anything = true;
//             hit = temp_hit;
//             closest_so_far = temp_hit.t;
//         }
//     }

    if (planeIntersect(groundPlane,r,temp_hit)){
        if(temp_hit.t < hit.t){
            hit_anything = true;
            hit = temp_hit;
            closest_so_far = temp_hit.t;
        }

    }



    return hit_anything;
}

vec3 castRay(Ray r){
    // TODO: trace ray in iterative way.
    HitRecord hit, temp_hit;
    hit.t = INFINITY;
    vec3 hitColor = vec3(0,0,0);
    //vec3 bgColor= vec3(87,140,217)/255;
    vec3 bgColor = texture(environmentMap, r.origin + INFINITY*r.direction).rgb;
    //if(!trace(r,hit)) return hitColor;//if no objectIntersect
    //hitColor = bgColor;

    vec3 ambient = vec3(0,0,0);

    vec3 diffuse = vec3(0,0,0);
    vec3 specular = vec3(0,0,0);
    vec3 lightDir;
    float diff, spec;
    vec3 k_s;
    vec3 viewDir = r.direction;
    vec3 reflectDir;
    bool hitFlag = false;
    vec3 total_attenuation = vec3(1.0);
    float reflect_prob;
    vec3 extinction = vec3(1.0);


    for (int bounce = 0; bounce<MAX_DEPTH; bounce++){
        temp_hit.t = INFINITY;
        hitFlag = trace(r,temp_hit);
        if(!hitFlag && bounce == 0) return bgColor;//if no objectIntersect at initial bounce
        else if (!hitFlag && bounce > 0) {
            hitColor = extinction * total_attenuation * texture(environmentMap, r.origin + INFINITY*r.direction).rgb;;
            break;
        }
        else{
         hit = temp_hit;

         vec3 reflectivity = (usingFresnel)? schlick(dot(hit.normal,r.direction), hit.mat.R0): hit.mat.R0;
         if(hit.mat.material_type > 0) {
            //return vec3(1,0,0);
            //reflect_prob = schlick(dot(hit.normal,r.direction), hit.mat.ior);
            //return vec3(reflect_prob,0,0);
            //refract_prob = 1 - reflect_prob;
            vec3 Nref = hit.normal;
            float NdotI = dot(r.direction, Nref);
            float etai = 1.0;
            float etat = hit.mat.ior;
            float eta;
            if(NdotI < 0){
                NdotI = -NdotI;
                eta = etai/etat;
            }
            else{
                Nref = -Nref ;
                eta = etat/etai;
            }

            float fresnel = schlick(NdotI,eta);
            if (fresnel < 0 ){//When light reflects
                r.origin = r.origin + (hit.t - bias)*r.direction;
                r.direction = normalize(reflect(r.direction, hit.normal));
                continue;
            }
            else{//When light refracts
                r.origin = r.origin + (hit.t + bias)*r.direction;
                r.direction = normalize(eta * r.direction + (eta * NdotI - sqrt(fresnel)) * Nref);
                extinction = vec3(1.0) + hit.mat.extinction_constant;
                continue;
            }
            //hitColor += refract_prob * castRay(Ray(hit.p,normalize(refracted_dir)),depth-bounce-1);
         }



         for (int i = 0; i<lights.length(); i++){ //TODO: modify
             lightDir = -normalize(lights[i].position - hit.p);
             Ray light_r;
             HitRecord light_hit;
             light_hit.t = INFINITY;
             light_r.origin = lights[i].position;
             light_r.direction = lightDir;
             vec3 shadow;
             shadow = trace(light_r,light_hit) ? vec3(1.0) : vec3(0.0);

             //vec3 F = schlick(max(dot(H, V), 0), hit.mat.R0);


             if(dot(shadow,vec3(1.0))>0){
                if (length(light_hit.p-hit.p)<bias){
                    shadow = vec3(0.0);
                }
                else {
                    if(light_hit.mat.material_type == 1)
                        shadow *= vec3(1.0) - light_hit.mat.shadow_attenuation_constant;
                    else shadow = vec3(1.0);
                }
                shadow *= 0.4;
             }

             if(!lights[i].castShadow) shadow = vec3(0.0);

             diff = max(dot(hit.normal, -lightDir), 0.0);
             if(hit.mat.Kd!=vec3(0,0,0)) diffuse += extinction * total_attenuation * (1-shadow) * diff * hit.mat.Kd * lights[i].color; // diffuse light

             reflectDir = reflect(-lightDir, hit.normal);
             spec = pow(max(dot(viewDir, reflectDir), 0.0), hit.mat.shininess);
             if(hit.mat.Ks!=vec3(0,0,0)) specular += total_attenuation * (1-shadow) * spec* hit.mat.Ks * lights[i].color;
         }
        if(hit.mat.Ka!=vec3(0,0,0)) ambient += total_attenuation * hit.mat.Ka * ambientLightColor;// ambient light


        r.origin = r.origin + (hit.t - bias) * r.direction;
        //hit.normal = (dot(hit.normal, r.direction) < 0)? hit.normal : -hit.normal;
        r.direction = normalize(reflect(r.direction, hit.normal));
        reflectivity = (usingFresnel)? schlick(dot(hit.normal,r.direction), hit.mat.R0): hit.mat.R0;
        total_attenuation *= reflectivity;

         }
        if(mask_mode > 0.5f) break;




    }

    hitColor += (ambient + diffuse + specular);
    //hitColor = hit.mat.Kd;


    return hitColor;
}

void main()
{
    // TODO:
    const int nsamples = 64;
    vec3 color = vec3(0);
//     for (int i = 0 ; i < nsamples ; i++){
//         float u = (TexCoords.x * W + (rand(color.xy + vec2(i,i)))) / W;
//         float v = (TexCoords.y * H + (rand(color.xz + vec2(i,i)))) / H;
//         Ray r = getRay(vec2(u,v));
//         //r = getRay(TexCoords);
//         color += castRay(r);
//     }
//     color /= nsamples;
    Ray r = getRay(vec2(TexCoords.x, TexCoords.y));
    color = castRay(r);


    FragColor = vec4(color, 1.0);
}
