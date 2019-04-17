#version 330

in vec2 UV;

out vec3 color;

uniform sampler2D render_tex_pass;

void main(void) {
	color = texture2D(render_tex_pass, UV.xy).xyz;
	//color = vec3(UV.xy, 0.0);
}