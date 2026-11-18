#version 330 core
in vec2 uv;
uniform samplerCube panorama;
uniform float angle;
uniform float aspect;
out vec4 FragColor;
void main() {
    vec3 ray = normalize(vec3((uv.x * 2.0 - 1.0) * aspect, (1.0 - uv.y * 2.0), 1.3));
    float pitch = -0.12;
    ray.yz = mat2(cos(pitch), -sin(pitch), sin(pitch), cos(pitch)) * ray.yz;
    ray.xz = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * ray.xz;
    vec3 image = texture(panorama, ray).rgb;
    float vignette = 1.0 - .26 * length((uv - .5) * vec2(1.0, .8));
    FragColor = vec4(image * vignette * .82, 1);
}
