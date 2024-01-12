#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 offset;

out vec4 finalColor;

void main() {
    // vec4 texColor = texture(texture0, texCoord + offset);
    // finalColor = texColor;
    finalColor = texture(texture0, fragTexCoord + offset) * fragColor;
}
