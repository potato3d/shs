#version 110

void main( void )
{
	gl_Position = ftransform();
	gl_FrontColor = gl_Color;
}
