#include <SFML/Window.hpp>
#include "Camera.hpp"
#include <GL/glew.h>
#include "ShaderProgram.hpp"
#include "Inotify.hpp"
#include "InputElementDescription.hpp"
#include "VertexBuffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "FPSCounter.hpp"
#include "UI.hpp"
#include <sstream>
#include "GameState.hpp"
#include "models.hpp"
#include "TimeStepper.hpp"

class Player
{
public:
	Player()
	: movingforward(false)
	, movingbackward(false)
	{
	}
	bool movingforward;
	bool movingbackward;
};
	
int main()
{
	sf::ContextSettings settings;
	settings.majorVersion = 3;
	settings.minorVersion = 2;
	const size_t width = 800;
	const size_t height = 600;
	sf::Window window(sf::VideoMode(width, height), "OpenGL", sf::Style::Default, sf::ContextSettings(32));
	window.setVerticalSyncEnabled(false);
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK) throw new GeneralException("glewInit failed");

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glViewport(0, 0, width, height);
	glm::mat4 projection = glm::perspective(60.0f, width / (float)height, 1.0f, 1000.0f);
	glm::mat4 world = glm::mat4(1.0f);

	const InputElementDescription description[] { { "in_position", 3, sizeof(glm::vec3), GL_FLOAT, false },
                                                      { "in_normal",   3, sizeof(glm::vec3), GL_FLOAT, false },
                                                      { "in_color",    3, sizeof(glm::vec3), GL_FLOAT, false },
                                                      { "", 0, 0, 0, false } };

	ShaderProgram program("null.vs", "null.fs", description);
	RocketUI rocketui(width, height);
	FPSCounter fpscounter;
	bool running = true;
	bool input = true;
	std::string currentline;
	VertexBuffer& worldblocks = blocks();
	FreeLookCamera camera(0.0f, 0.0f, glm::vec3(-1.0f, 1.0f, 4.0f), width, height);
	Player player;
	TimeStepper timestepper(60);
	while (running)
	{
		if(input) camera.Update();
		std::ostringstream convert;
		convert << fpscounter.Update();
		rocketui.setFPS(convert.str());
		Inotify::Poll();
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				running = false;
			}
			else if (event.type == sf::Event::Resized)
			{
				glViewport(0, 0, event.size.width, event.size.height);
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
				{
					running = false;
				}
				else if(event.key.code == sf::Keyboard::Quote)
				{
					running = false;
				}
				else if(event.key.code == sf::Keyboard::I)
				{
					input = !input;
				}
				else if(event.key.code == sf::Keyboard::Up)
				{
					player.movingforward = true;
				}
				else if(event.key.code == sf::Keyboard::Down)
				{
					player.movingbackward = true;
				}
			}
			else if (event.type == sf::Event::KeyReleased)
			{
				if(event.key.code == sf::Keyboard::Up)
				{
					player.movingforward = false;
				}
				else if(event.key.code == sf::Keyboard::Down)
				{
					player.movingbackward = false;
				}
			}
		}
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glFrontFace(GL_CW);
		glDisable(GL_BLEND);
		program.Set("projection", &projection[0][0]);
		program.Set("view", &camera.GetViewMatrix()[0][0]);
		program.Set("world", &world[0][0]);
		program.Use();
		worldblocks.Draw();
		static glm::vec3 playerpos(0.25f, 20.0f, 0.25f);
		size_t steps = timestepper.steps();
		for(size_t i = 0; i < steps; i++)
		{
			if(player.movingforward) playerpos.x += 0.05f;
			if(player.movingbackward) playerpos.x -= 0.05f;
		}
		glm::mat4 playerworld = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)), playerpos);
		program.Set("world", &playerworld[0][0]);
		cube().Draw();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glFrontFace(GL_CCW);
		rocketui.UpdateAndRender();
		window.display();
	}
	return 0;
}

