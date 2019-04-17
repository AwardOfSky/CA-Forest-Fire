#version 330

#define PROB_IGNITE 0.000005
#define PROB_BORN 0.001
#define FIRE_RED_FADE 0.025
#define FIRE_YEL_FADE 0.075
#define BURNT 0		// black = (0, 0, 0)
#define ALIVE 1		// green = (0, 1, 0)
#define BURNING 2	// red = (1, 0, 0)
#define SET_BURNT vec3(1.0 - FIRE_RED_FADE, 1.0 - FIRE_YEL_FADE, 0.0)
#define SET_ALIVE vec3(0.0, 1.0, 0.0)
#define SET_BURNING vec3(1.0, 0.0, 0.0)

in vec2 UV;		// interpolated tex coordinates from vertex shader
out vec3 color;	// output color

uniform sampler2D render_tex_pass;
uniform uint frame_count;
uniform vec2 resolution;

int get_state(vec3 col) {
	if (col.x == 1.0) {
		return BURNING;
	} else if (col.y == 1.0) {
		return ALIVE;
	} else {
		return BURNT;
	}
}

int get_burning(vec3 col) {
	return int(get_state(col) == BURNING);
}

// Inigo Quilez for the resque! @https://www.shadertoy.com/view/4tXyWN
float hash(uvec2 x) {
	x += uint(resolution.x)*uint(resolution.y)*frame_count;
	uvec2 q = 1103515245U * ((x >> 1U) ^ (x.yx));
	uint  n = 1103515245U * ((q.x) ^ (q.y >> 3U));
	return float(n) * (1.0 / float(0xffffffffU));
}

void main(void) {
	vec2 pos = gl_FragCoord.xy;
	if (pos.x >= 1.0 && pos.x <= (resolution.x - 1.0) && pos.y >= 1.0 && pos.y <= (resolution.y - 1.0)) {

		// get random uniform value
		float rand = hash(uvec2(pos));

		// sum of neigboors (all directions)
		int b00 = get_burning(textureOffset(render_tex_pass, UV, ivec2(-1, 1)).xyz);
		int b01 = get_burning(textureOffset(render_tex_pass, UV, ivec2(0, 1)).xyz);
		int b02 = get_burning(textureOffset(render_tex_pass, UV, ivec2(1, 1)).xyz);
		int b10 = get_burning(textureOffset(render_tex_pass, UV, ivec2(-1, 0)).xyz);
		int b12 = get_burning(textureOffset(render_tex_pass, UV, ivec2(1, 0)).xyz);
		int b20 = get_burning(textureOffset(render_tex_pass, UV, ivec2(-1, -1)).xyz);
		int b21 = get_burning(textureOffset(render_tex_pass, UV, ivec2(0, -1)).xyz);
		int b22 = get_burning(textureOffset(render_tex_pass, UV, ivec2(1, -1)).xyz);
		int neig = b00 + b01 + b02 + b10 + b12 + b20 + b21 + b22;

		// get current state
		vec3 col = texture2D(render_tex_pass, UV).xyz;
		int state = get_state(col);

		// set fragment color according to state
		color = col;
		if (state == BURNT) {
			if (rand < PROB_BORN) {
				color = SET_ALIVE;
			} else {
				col.x -= FIRE_RED_FADE; // fire effect
				col.y -= FIRE_YEL_FADE; // this will get clamped if below 0.0
				color = col;
			}
		} else if (state == BURNING) {
			color = SET_BURNT;
		} else if ((state == ALIVE) && ((rand < PROB_IGNITE) || (neig >= 1))) {
			color = SET_BURNING;
		}
	} else {
		color = texture2D(render_tex_pass, UV.xy).xyz;
	}
}