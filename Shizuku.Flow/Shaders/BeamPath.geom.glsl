#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2[] modelPos;

uniform bool Filter = true;

uniform vec2 Target = vec2(0,0);

void main()
{
	if (!Filter) {
		gl_Position = gl_in[0].gl_Position;
		EmitVertex();

		gl_Position = gl_in[1].gl_Position;
		EmitVertex();

		gl_Position = gl_in[2].gl_Position;
		EmitVertex();
	}
	else
	{
		if (length(modelPos[1] - Target) < 0.05)
		{
			gl_Position = gl_in[0].gl_Position;
			EmitVertex();

			gl_Position = gl_in[1].gl_Position;
			EmitVertex();

			gl_Position = gl_in[2].gl_Position;
			EmitVertex();
		}
	}
}