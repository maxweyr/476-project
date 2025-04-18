
#pragma once

#ifndef LAB471_TEXTURE_H_INCLUDED
#define LAB471_TEXTURE_H_INCLUDED

#include <glad/glad.h>
#include <string>


class Texture
{

public:

	void setFilename(const std::string &f) { filename = f; }
	void init();
	void setUnit(GLint u) { unit = u; }
	GLint getUnit() const { return unit; }
	void bind(GLint handle);
	void unbind();
	void setWrapModes(GLint wrapS, GLint wrapT); // Must be called after init()
	GLint getID() const { return tid; }

private:

	std::string filename;
	int width = 0;
	int height = 0;
	GLuint tid = 0;
	GLint unit = 0;

};

#endif // LAB471_TEXTURE_H_INCLUDED
