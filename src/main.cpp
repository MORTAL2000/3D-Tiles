#include <iostream>
#include <string>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "gl_core_4_4.h"
#include "util.h"
#include "multi_renderbatch.h"

int main(int, char**){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	SDL_Window *win = SDL_CreateWindow("3D Tiles", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);

	if (ogl_LoadFunctions() == ogl_LOAD_FAILED){
		std::cerr << "ogl load failed\n";
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(win);
		SDL_Quit();
		return 1;
	}
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClearDepth(1.f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n"
		<< "OpenGL Vendor: " << glGetString(GL_VENDOR) << "\n"
		<< "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(util::gldebug_callback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#endif

	STD140Buffer<glm::mat4> viewing{2, GL_UNIFORM_BUFFER, GL_STATIC_DRAW};
	viewing.map(GL_WRITE_ONLY);
	viewing.write<0>(0) = glm::lookAt(glm::vec3{0.f, 4.f, 8.f}, glm::vec3{0.f, 0.f, 0.f},
		glm::vec3{0.f, 1.f, 0.f});
	viewing.write<0>(1) = glm::perspective(util::deg_to_rad(75.f), 640.f / 480.f, 1.f, 100.f);
	viewing.unmap();

	const std::string shader_path = util::get_resource_path("shaders");
	GLuint shader = util::load_program({std::make_tuple(GL_VERTEX_SHADER, shader_path + "vmdei_test.glsl"),
		std::make_tuple(GL_FRAGMENT_SHADER, shader_path + "fmdei_test.glsl")});
	glUseProgram(shader);
	GLuint viewing_block = glGetUniformBlockIndex(shader, "Viewing");
	glUniformBlockBinding(shader, viewing_block, 0);
	viewing.bind_base(0);

	const std::string model_path = util::get_resource_path("models");
	PackedBuffer<glm::vec3, glm::vec3, glm::vec3> vbo{0, GL_ARRAY_BUFFER, GL_STATIC_DRAW, true};
	PackedBuffer<GLushort> ebo{0, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, true};
	std::vector<size_t> num_verts(3), num_elems(3);
	if (!util::load_obj(model_path + "dented_tile.obj", vbo, ebo, num_elems[0], &num_verts[0])){
		std::cout << "Failed to load dented tile\n";
		return 1;
	}
	if (!util::load_obj(model_path + "spike_tile.obj", vbo, ebo, num_elems[1], &num_verts[1], num_verts[0], num_elems[0])){
		std::cout << "Failed to load spike tile\n";
		return 1;
	}
	if (!util::load_obj(model_path + "big_tile.obj", vbo, ebo, num_elems[2], &num_verts[2],
		num_verts[0] + num_verts[1], num_elems[0] + num_elems[1]))
	{
		std::cout << "Failed to load big tile\n";
		return 1;
	}

	MultiRenderBatch<glm::vec3, glm::mat4> tile_batches{{4, 4, 2}, num_elems, {0, num_elems[0], num_elems[0] + num_elems[1]},
		std::move(vbo), std::move(ebo)};
	tile_batches.set_attrib_indices({2, 3});
	tile_batches.push_instance(0, std::make_tuple(glm::vec3{1.f, 0.f, 0.f}, glm::translate(glm::vec3{-3.f, 0.f, 1.f})));
	tile_batches.push_instance(0, std::make_tuple(glm::vec3{1.f, 0.f, 1.f}, glm::translate(glm::vec3{1.f, 0.f, -3.f})));
	tile_batches.push_instance(0, std::make_tuple(glm::vec3{1.f, 0.f, 1.f}, glm::translate(glm::vec3{1.f, 0.f, 3.f})
		* glm::rotate(util::deg_to_rad(90), glm::vec3{0, 1, 0})));
	tile_batches.push_instance(1, std::make_tuple(glm::vec3{0.f, 0.f, 1.f}, glm::translate(glm::vec3{3.f, 0.f, 1.f})));
	tile_batches.push_instance(1, std::make_tuple(glm::vec3{1.f, 1.f, 0.f}, glm::translate(glm::vec3{-1.f, 0.f, -3.f})));
	tile_batches.push_instance(1, std::make_tuple(glm::vec3{0.f, 0.f, 1.f}, glm::translate(glm::vec3{-3.f, 0.f, -1.f})));
	tile_batches.push_instance(2, std::make_tuple(glm::vec3{1.f, 0.5f, 0.5f}, glm::translate(glm::vec3{0.f, 0.f, 0.f})));

	SDL_Event e;
	bool quit = false, view_change = false;
	int view_pos = 0;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT){
				quit = true;
			}
			else if (e.type == SDL_KEYDOWN){
				switch (e.key.keysym.sym){
					case SDLK_d:
						view_pos = (view_pos + 1) % 4;
						view_change = true;
						break;
					case SDLK_a:
						view_pos = view_pos == 0 ? 3 : (view_pos - 1) % 4;
						view_change = true;
						break;
					case SDLK_ESCAPE:
						quit = true;
						break;
					default:
						break;
				}
			}
		}
		if (view_change){
			view_change = false;
			glm::vec3 eye_pos;
			switch (view_pos){
				case 1:
					eye_pos = glm::vec3{8, 4, 0};
					break;
				case 2:
					eye_pos = glm::vec3{0, 4, -8};
					break;
				case 3:
					eye_pos = glm::vec3{-8, 4, 0};
					break;
				default:
					eye_pos = glm::vec3{0, 4, 8};
					break;
			}
			viewing.map(GL_WRITE_ONLY);
			viewing.write<0>(0) = glm::lookAt(eye_pos, glm::vec3{0.f, 0.f, 0.f},
				glm::vec3{0.f, 1.f, 0.f});
			viewing.unmap();
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		tile_batches.render();

		SDL_GL_SwapWindow(win);
	}
	glDeleteProgram(shader);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}

