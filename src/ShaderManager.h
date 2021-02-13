#ifndef _SHADERMANAGER_H_
#define _SHADERMANAGER_H_

#include <gl/glew.h>
#include <vector>
#include <string>

class ShaderManager
{
public:
	ShaderManager();

	void reset();

	void bindProgram();
	void unbindProgram();

	void setFragmentProgram( const std::string& filename );
	const std::string& fragmentProgram() const;

	void setVertexProgram( const std::string& filename );
	const std::string& vertexProgram() const;

	void addUniformi( const char* symbolName, int value );
	void addUniformf( const char* symbolName, float value );

	void initShaders();
	bool reloadShaders();

	void setEnabled( bool enabled );

private:
	typedef std::pair<std::string, int> IntUniform;
	typedef std::pair<std::string, float> FloatUniform;

private:
	bool initShader( GLhandleARB _programObject, const char *filen, GLuint type );

private:
	bool _enabled;
	unsigned int _programObject;
	std::string _fp;
	std::string _vp;
	std::string _gp;
	std::vector<IntUniform>   _uniformI;
	std::vector<FloatUniform> _uniformF;
};

#endif // _SHADERMANAGER_H_
