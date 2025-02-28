#version 330 core
layout (location = 0) in vec2 aPos; // either -1 or 1
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform bool windowMode;
uniform vec2 windowSize;
uniform vec2 screenPos;
uniform vec2 resolution;

void main() {
    vec2 pos = aPos;
    if(windowMode) {
        // [-1..+1] -> [0..1]
        vec2 uv = (pos + 1.0) * 0.5;

        vec2 screenSpace = screenPos + windowSize * uv;
        vec2 screenUv = screenSpace / resolution;

        // [0..1] -> [-1..+1]
        vec2 screenClip = screenUv * 2.0 - 1.0;

        pos = screenClip;
        pos.y = -pos.y; // convert to opengl Y up
    }

    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
