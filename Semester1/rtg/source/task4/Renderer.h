


#ifndef INCLUDED_RENDERER
#define INCLUDED_RENDERER

#pragma once

#include <GL/gl.h>
#include <framework/BasicRenderer.h>
#include "math\math.h"
#include "math\vector.h"
#include "math\matrix.h"


class Renderer : public BasicRenderer
{
private:
	int viewport_width;
	int viewport_height;

public:
	Renderer(const Renderer&) = delete;
	Renderer& operator =(const Renderer&) = delete;

	Renderer(GL::platform::Window& window);

	void resize(int width, int height);
	void render();
	float deg2rad(float degrees);

	float pi = math::constants<float>().pi();
	int addDegree = 0;
};

#endif  // INCLUDED_RENDERER
