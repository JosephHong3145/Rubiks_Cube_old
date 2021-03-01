#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aUV;

out vec3 FragPos;
out vec3 Normal;
out vec3 vertexColor;
out vec2 vertexUV;
out vec4 fragment_position_light_space;
//out vec3 LightPos;


uniform mat4 worldMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 light_proj_view_matrix;

void main()
{
    FragPos = vec3(worldMatrix * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(worldMatrix))) * aNormal;
    vertexColor = aColor;
    vertexUV = aUV;
    fragment_position_light_space = light_proj_view_matrix * vec4(FragPos, 1.0);

    mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;
    gl_Position = modelViewProjection * vec4(aPos, 1.0);
    //LightPos = vec3(view * vec4(lightPos, 1.0)); // Transform world-space light position to view-space light position
}

