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

	//helper functions
	bool collide(glm::vec3 new_position);
	void attempt_arrest();
	void play_alibi(size_t i);
	void play_recording(size_t i);

	//----- game state -----

	//game state tracking:
	enum GameState : uint8_t {
		IN_PROGRESS,
		WIN,
		LOSE
	};
	GameState gamestate = IN_PROGRESS;

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
	std::vector<Sound::Sample> alibi_samples;
	std::vector<std::shared_ptr<Sound::PlayingSample>> alibis;
	float suspect_radius = 0.5f;
	float suspect_speak_radius = 1.3f;
	size_t murderer_id = 2; //Blue (suspect 2) is the murderer

	//evidence data
	std::vector<Scene::Transform *> evidences;
	std::vector<Sound::Sample> recording_samples;
	std::vector<std::shared_ptr<Sound::PlayingSample>> recordings;
	float evidence_radius = 0.5f;
	float recording_play_radius = 1.3f;

	//walls for collisions
	std::vector<Scene::Transform *> walls;

	//background music
	std::shared_ptr<Sound::PlayingSample> background_music;
	
	//camera data
	Scene::Camera *camera = nullptr;

};
