#include "ShaderManager.h"
#include <string>
#include <iostream>
#include <fstream>

ShaderManager::ShaderManager()
: _enabled( true ), _programObject( 0 )
{
	// empty
}

void ShaderManager::reset()
{
	glDeleteProgram( _programObject );
	_programObject = 0;
	_fp.clear();
	_vp.clear();
	_gp.clear();
	_uniformI.clear();
	_uniformF.clear();
}

void ShaderManager::bindProgram()
{
	if( _enabled )
		glUseProgram( _programObject );
}

void ShaderManager::unbindProgram()
{
	if( _enabled )
		glUseProgram( 0 );
}

void ShaderManager::setFragmentProgram( const std::string& filename )
{
	_fp = filename;
}

const std::string& ShaderManager::fragmentProgram() const
{
	return _fp;
}

void ShaderManager::setVertexProgram( const std::string& filename )
{
	_vp = filename;
}

const std::string& ShaderManager::vertexProgram() const
{
	return _vp;
}

void ShaderManager::addUniformi( const char* symbolName, int value )
{
	_uniformI.push_back( IntUniform( symbolName, value ) );
}

void ShaderManager::addUniformf( const char* symbolName, float value )
{
	_uniformF.push_back( FloatUniform( symbolName, value ) );
}

void ShaderManager::initShaders()
{
	bool ok = reloadShaders();

	if( !ok )
		std::cout << "Warning: Error initializing one or more shaders." << std::endl;
}

bool ShaderManager::reloadShaders()
{
	_programObject = glCreateProgram();
	bool shaderOk = true;

	// Fragment Shader to be used
	if( !_fp.empty() )
		shaderOk &= initShader( _programObject, _fp.c_str(), GL_FRAGMENT_SHADER );

	// Vertex Shader to be used
	if( !_vp.empty() )
		shaderOk &= initShader( _programObject, _vp.c_str(), GL_VERTEX_SHADER );
	
	// Geometry Shader to be used
	if( !_gp.empty() )
	{
		shaderOk &= initShader( _programObject, _gp.c_str(), GL_GEOMETRY_SHADER_EXT );

		////Setup Geometry Shader////
		// one of: GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES, GL_TRIANGLES_ADJACENCY_EXT
		//Set GL_TRIANGLES primitives as INPUT
		glProgramParameteriEXT( _programObject,GL_GEOMETRY_INPUT_TYPE_EXT , GL_TRIANGLES );

		// one of: GL_POINTS, GL_LINE_STRIP, GL_TRIANGLE_STRIP  
		//Set TRIANGLE STRIP as OUTPUT
		glProgramParameteriEXT( _programObject,GL_GEOMETRY_OUTPUT_TYPE_EXT , GL_TRIANGLE_STRIP );

		// TODO: 
		// This parameter is very important and have a great impact on shader performance
		// Its value must be chosen closer as possible to real maximum number of vertices
		//glProgramParameteriEXT( _programObject,GL_GEOMETRY_VERTICES_OUT_EXT, outputVertexCount );
		//std::cout << "GS output set to: " << outputVertexCount << "\n";
	}

	if( !shaderOk )
		return false;

	if( _fp.empty() && _vp.empty() && _gp.empty() )
		return true;

	// Link whole program object
	glLinkProgram( _programObject );

	// Test link success
	GLint ok = false;
	glGetProgramiv( _programObject, GL_LINK_STATUS, &ok );
	if( !ok )
	{
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetProgramInfoLog(_programObject, maxLength, &maxLength, infoLog);
		std::cout<<"Link error: "<<infoLog<<"\n";
		delete []infoLog;
	}

	// Program validation
	glValidateProgram(_programObject);
	ok = false;
	glGetProgramiv(_programObject, GL_VALIDATE_STATUS, &ok);
	if (!ok)
	{
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetProgramInfoLog(_programObject, maxLength, &maxLength, infoLog);
		std::cout<<"Validation error: "<<infoLog<<"\n";
		delete []infoLog;
	}

	// Bind program object for parameters setting
	glUseProgram( _programObject );

	/************************************************************************/
	/* Send shader parameters                                               */
	/************************************************************************/
	for( int i = 0; i < _uniformI.size(); ++i )
	{
		int loc = glGetUniformLocation( _programObject, _uniformI[i].first.c_str() );
		glUniform1i( loc, _uniformI[i].second );
	}
	for( int i = 0; i < _uniformF.size(); ++i )
	{
		glUniform1f( glGetUniformLocation( _programObject, _uniformF[i].first.c_str() ), _uniformF[i].second );
	}

	// Cleanup
	glUseProgram( 0 );
	std::cout << std::endl;
	return ok;
}

void ShaderManager::setEnabled( bool enabled )
{
	_enabled = enabled;
}

bool ShaderManager::initShader( GLhandleARB programObject, const char *filen, GLuint type )
{
	//Source file reading
	std::string buff;
	std::ifstream file;
	std::string filename=filen;
	std::cerr.flush();
	file.open(filename.c_str());
	std::string line;
	while(std::getline(file, line))
		buff += line + "\n";

	const GLcharARB *txt=buff.c_str();

	//Shader object creation
	GLhandleARB shader = glCreateShader(type);

	//Source code assignment
	glShaderSource(shader, 1, &txt, NULL);

	//Compile shader object
	glCompileShader(shader);

	//Check if shader compiled
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
		std::cout<<"Compilation error: "<<infoLog<<"\n";
		delete []infoLog;
		return false;
	}

	// attach shader to program object
	glAttachShader(programObject, shader);

	// delete object, no longer needed
	glDeleteShader(shader);

	//Global error checking
	std::cout<<"InitShader: "<< "\'" << filen << "\'" <<" Errors: "<<gluErrorString(glGetError())<<"\n";
	return true;
}
