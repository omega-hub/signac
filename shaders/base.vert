in float x;
in float y;

uniform float xmin;
uniform float xmax;
uniform float ymin;
uniform float ymax;

void main(void)
{
	gl_Position = vec4((x - xmin)/(xmax - xmin) * 2.0 - 1.0, 1.0 - (y - ymin)/(ymax - ymin) * 2.0, 0, 1);
}
