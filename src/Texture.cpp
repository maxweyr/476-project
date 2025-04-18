
#include "Texture.h"
#include "GLSL.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;


void Texture::init()
{
	// Load texture
	int w, h, ncomps;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename.c_str(), &w, &h, &ncomps, 0);
	if (! data)
	{
		cerr << filename << " not found" << endl;
	}
	if (ncomps != 3)
	{
		cerr << filename << " must have 3 components (RGB)" << endl;
	}
	if ((w & (w - 1)) != 0 || (h & (h - 1)) != 0)
	{
		cerr << filename << " must be a power of 2" << endl;
	}
	width = w;
	height = h;

	// Generate a texture buffer object
	CHECKED_GL_CALL(glGenTextures(1, &tid));
	// Bind the current texture to be the newly generated texture object
	CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, tid));

	// Load the actual texture data
	// Base level is 0, number of channels is 3, and border is 0.
	CHECKED_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
	// Generate image pyramid
	CHECKED_GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));

	// Set texture wrap modes for the S and T directions
	CHECKED_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	CHECKED_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	// Set filtering mode for magnification and minimification
	CHECKED_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	CHECKED_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

	// Unbind
	CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
	// Free image, since the data is now on the GPU
	stbi_image_free(data);
}

void Texture::setWrapModes(GLint wrapS, GLint wrapT)
{
	// Must be called after init()
	CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, tid));
	CHECKED_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS));
	CHECKED_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT));
}

void Texture::bind(GLint handle)
{
	CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
	CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, tid));
	CHECKED_GL_CALL(glUniform1i(handle, unit));
}

void Texture::unbind()
{
	CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
	CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
}
