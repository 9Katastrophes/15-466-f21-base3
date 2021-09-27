#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player data
	Scene::Transform *player = nullptr;
	Scene::Transform *player_head = nullptr;
	float player_radius = 0.5f;

	//suspect data
	std::vector<Scene::Transform *> suspects;
	std::vector<std::shared_ptr<Sound::PlayingSample>> alibis;
	float suspect_radius = 0.5f;
	float suspect_speak_radius = 1.0f;
	size_t murderer_id = 2; //Blue (suspect 2) is the murderer

	//evidence data
	std::vector<Scene::Transform *> evidences;
	std::vector<std::shared_ptr<Sound::PlayingSample>> recordings;
	float recording_play_radius = 0.5f;
	size_t murder_recording_id = 4; //last recording is of the crime

	//walls for collisions
	std::vector<Scene::Transform *> walls;
	
	//camera data
	Scene::Camera *camera = nullptr;

};
