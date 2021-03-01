#version 330 core

const float PI = 3.1415926535897932384626433832795;
uniform sampler2D textureSampler;

in vec3 Normal;  
in vec3 vertexColor;
in vec3 FragPos;
in vec4 fragment_position_light_space;
in vec2 vertexUV;
  
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 light_direction;

const float shading_ambient_strength    = 0.1 * 2;
const float shading_diffuse_strength    = 0.6 * 2;
const float shading_specular_strength   = 0.3 * 2;

uniform float light_cutoff_outer;
uniform float light_cutoff_inner;
uniform float light_near_plane;
uniform float light_far_plane;

uniform vec3 objectColor;
uniform vec3 viewPos; 

uniform sampler2D shadow_map;

in vec4 gl_FragCoord;

out vec4 FragColor;

vec3 ambient_color(vec3 lightColor_arg) {
    return shading_ambient_strength * lightColor_arg;
}

vec3 diffuse_color(vec3 lightColor_arg, vec3 lightPos_arg) {
    vec3 light_direction = normalize(lightPos_arg - FragPos);
    return shading_diffuse_strength * lightColor_arg * max(dot(normalize(Normal), light_direction), 0.0f);
}

vec3 specular_color(vec3 lightColor_arg, vec3 lightPos_arg) {
    vec3 light_direction = normalize(lightPos_arg - FragPos);
    vec3 view_direction = normalize(viewPos - FragPos);
    vec3 reflect_light_direction = reflect(-light_direction, normalize(Normal));
    return shading_specular_strength * lightColor_arg * pow(max(dot(reflect_light_direction, view_direction), 0.0f),32);
}

float shadow_scalar() {
    // this function returns 1.0 when the surface receives light, and 0.0 when it is in a shadow
    // perform perspective divide
    vec3 ndc = fragment_position_light_space.xyz / fragment_position_light_space.w;
    // transform to [0,1] range
    ndc = ndc * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragment_position_light_space as coords)
    float closest_depth = texture(shadow_map, ndc.xy).r;
    // get depth of current fragment from light's perspective
    float current_depth = ndc.z;
    // check whether current frag pos is in shadow
    float bias = 0;  // bias applied in depth map: see shadow_vertex.glsl
    return ((current_depth - bias) < closest_depth) ? 1.0 : 0.0;
}

float spotlight_scalar() {
    float theta = dot(normalize(FragPos - lightPos), light_direction);
    
    if(theta > light_cutoff_inner) {
        return 1.0;
    } else if(theta > light_cutoff_outer) {
        return (1.0 - cos(PI * (theta - light_cutoff_outer) / (light_cutoff_inner - light_cutoff_outer))) / 2.0;
    } else {
        return 0.0;
    }
}

void main()
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    float scalar = shadow_scalar() * spotlight_scalar();
    ambient = ambient_color(lightColor);
    diffuse = scalar * diffuse_color(lightColor, lightPos);
    specular = scalar * specular_color(lightColor, lightPos);
        
    vec3 result = (ambient + diffuse + specular) * objectColor;
    vec4 textureColor = texture( textureSampler, vertexUV );
    FragColor = vec4(result, 1.0) * textureColor;
} 
