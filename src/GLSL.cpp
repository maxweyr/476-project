//
//    Many useful helper functions for GLSL shaders - gleaned from various sources including orange book
//    Created by zwood on 2/21/10.
//    Modified by sueda 10/15/15.
//

#include "GLSL.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <cassert>


namespace GLSL
{

const char * errorString(GLenum err)
{
	switch (err) {
	case GL_NO_ERROR:
		return "No error";
	case GL_INVALID_ENUM:
		return "Invalid enum";
	case GL_INVALID_VALUE:
		return "Invalid value";
	case GL_INVALID_OPERATION:
		return "Invalid operation";
	case GL_STACK_OVERFLOW:
		return "Stack overflow";
	case GL_STACK_UNDERFLOW:
		return "Stack underflow";
	case GL_OUT_OF_MEMORY:
		return "Out of memory";
	default:
		return "No error";
	}
}

void printOpenGLErrors(const char *const Function, const char * const File, int const Line)
{
	GLenum Error = glGetError();
	if (Error != GL_NO_ERROR)
	{
		const char *const ErrorString = errorString(Error);
		printf("OpenGL error in file '%s' at line %d calling function '%s': '%s' '%d 0x%X'\n", File, Line, Function, ErrorString, Error, Error);
	}
}

void checkError(const char *str)
{
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		if (str)
		{
			printf("%s: ", str);
		}
		printf("GL_ERROR = %s.\n", errorString(glErr));
		assert(false);
	}
}

void printShaderInfoLog(GLuint shader)
{
	GLint infologLength = 0;
	CHECKED_GL_CALL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength));

	if (infologLength > 0)
	{
		GLchar * infoLog = new GLchar[infologLength];
		if (infoLog == NULL)
		{
			puts("ERROR: Could not allocate InfoLog buffer");
			exit(1);
		}

		GLint charsWritten = 0;
		CHECKED_GL_CALL(glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog));
		printf("Shader InfoLog:\n%s\n\n", infoLog);
		delete[] infoLog;
	}
}

void printProgramInfoLog(GLuint program)
{
	GLchar *infoLog;

	GLint infologLength = 0;
	CHECKED_GL_CALL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength));

	if (infologLength > 0)
	{
		infoLog = new GLchar[infologLength];
		if (infoLog == NULL)
		{
			puts("ERROR: Could not allocate InfoLog buffer");
			exit(1);
		}

		GLint charsWritten = 0;
		CHECKED_GL_CALL(glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog));
		printf("Program InfoLog:\n%s\n\n", infoLog);
		delete[] infoLog;
	}
}

void checkVersion()
{
	int major, minor;
	major = minor = 0;
	const char *verstr = (const char *) glGetString(GL_VERSION);

	if ((verstr == NULL) || (sscanf(verstr, "%d.%d", &major, &minor) != 2))
	{
		printf("Invalid GL_VERSION format %d.%d\n", major, minor);
	}
	if (major < 2)
	{
		printf("This shader example will not work due to the installed Opengl version, which is %d.%d.\n", major, minor);
		exit(0);
	}
}

GLint getAttribLocation(const GLuint program, const char varname[], bool verbose)
{
	GLint r = glGetAttribLocation(program, varname);
	if (r < 0 && verbose)
	{
		std::cerr << "WARN: " << varname << " cannot be bound (it either doesn't exist or has been optimized away). safe_glAttrib calls will silently ignore it.\n" << std::endl;
	}
	return r;
}


GLint getUniformLocation(const GLuint program, const char varname[], bool verbose)
{
	GLint r = glGetUniformLocation(program, varname);
	if (r < 0 && verbose)
	{
		std::cerr << "WARN: " << varname << " cannot be bound (it either doesn't exist or has been optimized away). safe_glUniform calls will silently ignore it.\n" << std::endl;
	}
	return r;
}


void enableVertexAttribArray(const GLint handle)
{
	if (handle >= 0)
	{
		glEnableVertexAttribArray(handle);
	}
}

void disableVertexAttribArray(const GLint handle)
{
	if (handle >= 0)
	{
		glDisableVertexAttribArray(handle);
	}
}

void vertexAttribPointer(const GLint handle, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer)
{
	if (handle >= 0)
	{
		glVertexAttribPointer(handle, size, type, normalized, stride, pointer);
	}
}

}
