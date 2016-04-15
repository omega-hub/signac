#version 410
in double x;
in double y;

uniform double xmin;
uniform double xmax;
uniform double ymin;
uniform double ymax;

void main(void)
{
	gl_Position = vec4((x - xmin)/(xmax - xmin) * 2.0 - 1.0, 1.0 - (y - ymin)/(ymax - ymin) * 2.0, 0, 1);
}
