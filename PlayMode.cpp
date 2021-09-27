#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>

GLuint musicmurdermystery_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > musicmurdermystery_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("musicmurdermystery.pnct"));
	musicmurdermystery_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > musicmurdermystery_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("musicmurdermystery.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = musicmurdermystery_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = musicmurdermystery_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});

PlayMode::PlayMode() : scene(*musicmurdermystery_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Player") player = &transform;
		else if (transform.name == "PlayerHead") player_head = &transform;
		else if (transform.name.substr(0, 7) == "Suspect") suspects.push_back(&transform);
		else if (transform.name.substr(0, 8) == "Evidence") evidences.push_back(&transform);
		else if (transform.name.substr(0, 4) == "Wall") walls.push_back(&transform);
	}

	if (player == nullptr) throw std::runtime_error("Player not found.");
	if (player_head == nullptr) throw std::runtime_error("Player head not found.");
	if (suspects.size() != 4) {
		std::string err_string = std::to_string(suspects.size()) + " suspects found. Expected 4.";
		throw std::runtime_error(err_string);
	}
	if (evidences.size() != 5) {
		std::string err_string = std::to_string(evidences.size()) + " evidences found. Expected 5.";
		throw std::runtime_error(err_string);
	}
	if (walls.size() == 0) throw std::runtime_error("Walls missing.");
	std::cout << walls.size();

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	glm::vec3 right = frame[0];
	glm::vec3 at = frame[3];
	Sound::listener.set_position_right(at, right, 1.0f / 60.0f);

	//set background music
	background_music = Sound::loop_3D(*dusty_floor_sample, 1.0f, camera->transform->position, 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			attempt_arrest();
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//move sound to follow leg tip position:
	// leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	//move player:
	if (gamestate == IN_PROGRESS) {
		//combine inputs into a move:
		constexpr float PlayerSpeed = 10.0f;
		glm::vec2 move = glm::vec2(0.0f);
		float rotation = 0.0f;
		if (left.pressed && !right.pressed) {
			rotation = 3.0f * 3.14f / 2.0f;
			move.x = -1.0f;
		}
		if (!left.pressed && right.pressed) {
			rotation = 3.14f / 2.0f;
			move.x = 1.0f;
		}
		if (down.pressed && !up.pressed) {
			rotation = 0.0f;
			move.y = -1.0f;
		}
		if (!down.pressed && up.pressed) {
			rotation = 3.14f;
			move.y = 1.0f;
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = player->make_local_to_parent();
		glm::vec3 right = -frame[0];
		glm::vec3 up = frame[1];

		glm::vec3 angles(0.0f, 0.0f, rotation);
		glm::quat new_rotation(angles);

		glm::vec3 new_position = player->position + move.y * right + move.x * up;

		if (!collide(new_position)){
			player->position += move.y * right + move.x * up;
		}
		player_head->rotation = glm::normalize(new_rotation);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		if (gamestate == IN_PROGRESS) {
			lines.draw_text("WASD moves; Listen to alibis and evidence. Make an arrest with SPACE.",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
		else if (gamestate == WIN) {
			lines.draw_text("You arrested the culprit!",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
		else if (gamestate == LOSE) {
			lines.draw_text("You arrested the wrong person. :(",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
	}
	GL_ERRORS();
}

bool PlayMode::collide(glm::vec3 new_position) {
	for (size_t i=0;i<suspects.size();i++){
		float distance = glm::distance(new_position, suspects[i]->position);
		if (distance <= player_radius + suspect_radius)
			return true;
	}
	for (size_t i=0;i<evidences.size();i++) {
		float distance = glm::distance(new_position, evidences[i]->position);
		if (distance <= player_radius + recording_play_radius)
			return true;
	}
	for (size_t i=0;i<walls.size();i++) {
		glm::vec3 wall_scale = walls[i]->scale;
		glm::vec3 wall_pos = walls[i]->position;

		if (wall_pos.x - wall_scale.x <= new_position.x + player_radius && new_position.x - player_radius <= wall_pos.x + wall_scale.x
			&& wall_pos.y - wall_scale.y <= new_position.y + player_radius && new_position.y - player_radius <= wall_pos.y + wall_scale.y)
			return true;
	}
	return false;
}

void PlayMode::attempt_arrest() {
	float arrest_radius = suspect_speak_radius;

	for (size_t i=0;i<suspects.size();i++){
		float distance = glm::distance(player->position, suspects[i]->position);
		if (distance <= arrest_radius) {
			//player can arrest the suspect
			if (murderer_id == i) { //suspect is the murderer!
				gamestate = WIN;
			}
			else { //arrested the wrong person :(
				gamestate = LOSE;
			}

			break;
		}
	}
}

// glm::vec3 PlayMode::get_leg_tip_position() {
// 	//the vertex position here was read from the model in blender:
// 	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
// }
