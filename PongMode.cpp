#include "PongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PongMode::PongMode()
{

	//set up trail as if ball has been here for 'forever':

	for (int i = 0; i < ball_num; i++) {
		balls.push_back(glm::vec2(i + 0.0f, i + 0.0f));
		
		std::deque< glm::vec3 > tmp;
		tmp.clear();
		tmp.emplace_back(balls[i], trail_length);
		tmp.emplace_back(balls[i], 0.0f);
		ball_trails.push_back(tmp);

		if (i % 4 == 0) {
			ball_velocities.push_back(glm::vec2(0.0f - 1, 0.0f - 1));
		} else if (i % 3 == 0) {
			ball_velocities.push_back(glm::vec2(0.0f + 1, 0.0f - 1));
		} else if (i % 2 == 0) {
			ball_velocities.push_back(glm::vec2(0.0f + 1, 0.0f + 1.5));
		} else {
			ball_velocities.push_back(glm::vec2(0.0f - 1, 0.0f + 1));
		}
	}

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3,									 //size
			GL_FLOAT,							 //type
			GL_FALSE,							 //normalized
			sizeof(Vertex),						 //stride
			(GLbyte *)0 + 0						 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4,								  //size
			GL_UNSIGNED_BYTE,				  //type
			GL_TRUE,						  //normalized
			sizeof(Vertex),					  //stride
			(GLbyte *)0 + 4 * 3				  //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2,									 //size
			GL_FLOAT,							 //type
			GL_FALSE,							 //normalized
			sizeof(Vertex),						 //stride
			(GLbyte *)0 + 4 * 3 + 4 * 1			 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1, 1);
		std::vector<glm::u8vec4> data(size.x * size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

PongMode::~PongMode()
{

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool PongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_MOUSEMOTION)
	{
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y * -2.0f + 1.0f);
		left_paddle.y = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).y;
	}else if (evt.type == SDL_KEYDOWN) {
		if(evt.key.keysym.sym == SDLK_1) {
			ball_num = 1;
		} else if(evt.key.keysym.sym == SDLK_2) {
			ball_num = 2;
		} else if(evt.key.keysym.sym == SDLK_3) {
			ball_num = 3;
		} else if(evt.key.keysym.sym == SDLK_4) {
			ball_num = 4;
		} else if(evt.key.keysym.sym == SDLK_5) {
			ball_num = 5;
		} else if(evt.key.keysym.sym == SDLK_6) {
			ball_num = 6;
		} else if(evt.key.keysym.sym == SDLK_7) {
			ball_num = 7;
		} else if(evt.key.keysym.sym == SDLK_8) {
			ball_num = 8;
		} else if(evt.key.keysym.sym == SDLK_9) {
			ball_num = 9;
		}
		std::cout<<ball_num<<std::endl;
	}

	return false;
}

void PongMode::update(float elapsed)
{

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- paddle update -----

	{ //right player ai:
		// ai_offset_update -= elapsed;
		// if (ai_offset_update < elapsed)
		// {
		// 	//update again in [0.5,1.0) seconds:
		// 	ai_offset_update = (mt() / float(mt.max())) * 0.5f + 0.5f;
		// 	ai_offset = (mt() / float(mt.max())) * 2.5f - 1.25f;
		// }
		// if (right_paddle.y < ball.y + ai_offset)
		// {
		// 	right_paddle.y = std::min(ball.y + ai_offset, right_paddle.y + 2.0f * elapsed);
		// }
		// else
		// {
		// 	right_paddle.y = std::max(ball.y + ai_offset, right_paddle.y - 2.0f * elapsed);
		// }
	}

	//clamp paddles to court:
	right_paddle.y = std::max(right_paddle.y, -court_radius.y + paddle_radius.y);
	right_paddle.y = std::min(right_paddle.y, court_radius.y - paddle_radius.y);

	left_paddle.y = std::max(left_paddle.y, -court_radius.y + paddle_radius.y);
	left_paddle.y = std::min(left_paddle.y, court_radius.y - paddle_radius.y);

	//----- ball update -----

	//speed of ball doubles every four points:
	float speed_multiplier = 2.0f * std::pow(2.0f, (20 - left_score) / 4.0f);

	//velocity cap, though (otherwise ball can pass through paddles):
	speed_multiplier = std::min(speed_multiplier, 10.0f);


	for (int i = 0; i < ball_num; i++) {
		balls[i] += elapsed * speed_multiplier * ball_velocities[i];
	}

	// ball += elapsed * speed_multiplier * ball_velocity;
	// second ball
	// ball_2 += elapsed * speed_multiplier * ball_2_velocity;

	//---- collision handling ----

	//paddles:
	auto paddle_vs_ball = [this](glm::vec2 const &paddle, glm::vec2 &ball_in, glm::vec2 &ball_velocity_in, int flag) {
		//compute area of overlap:
		glm::vec2 min = glm::max(paddle - paddle_radius, ball_in - ball_radius);
		glm::vec2 max = glm::min(paddle + paddle_radius, ball_in + ball_radius);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y)
			return;

		if (left_score > 0){
			if(flag == 0) {
				left_score -= 1;
				std::cout<<"left-\n";
			} else {
				left_score += 1;
				std::cout<<"left+\n";
			}
		}

		if (max.x - min.x > max.y - min.y)
		{
			//wider overlap in x => bounce in y direction:
			if (ball_in.y > paddle.y)
			{
				ball_in.y = paddle.y + paddle_radius.y + ball_radius.y;
				ball_velocity_in.y = std::abs(ball_velocity_in.y);
			}
			else
			{
				ball_in.y = paddle.y - paddle_radius.y - ball_radius.y;
				ball_velocity_in.y = -std::abs(ball_velocity_in.y);
			}
		}
		else
		{
			//wider overlap in y => bounce in x direction:
			if (ball_in.x > paddle.x)
			{
				ball_in.x = paddle.x + paddle_radius.x + ball_radius.x;
				ball_velocity_in.x = std::abs(ball_velocity_in.x);
			}
			else
			{
				ball_in.x = paddle.x - paddle_radius.x - ball_radius.x;
				ball_velocity_in.x = -std::abs(ball_velocity_in.x);
			}
			//warp y velocity based on offset from paddle center:
			float vel = (ball_in.y - paddle.y) / (paddle_radius.y + ball_radius.y);
			ball_velocity_in.y = glm::mix(ball_velocity_in.y, vel, 0.75f);
		}
	};

	for (int i = 0; i < ball_num; i++) {
		paddle_vs_ball(left_paddle, balls[i], ball_velocities[i], i % 2);
	}

	// paddle_vs_ball(left_paddle, balls[0], ball_velocity);
	// paddle_vs_ball(left_paddle, ball_2, ball_2_velocity);
	// paddle_vs_ball(right_paddle);

	auto court_wall = [this](glm::vec2 &ball_in, glm::vec2 &ball_velocity_in) {
		//court walls:
		if (ball_in.y > court_radius.y - ball_radius.y)
		{
			ball_in.y = court_radius.y - ball_radius.y;
			if (ball_velocity_in.y > 0.0f)
			{
				ball_velocity_in.y = -ball_velocity_in.y;
			}
		}
		if (ball_in.y < -court_radius.y + ball_radius.y)
		{
			ball_in.y = -court_radius.y + ball_radius.y;
			if (ball_velocity_in.y < 0.0f)
			{
				ball_velocity_in.y = -ball_velocity_in.y;
			}
		}

		if (ball_in.x > court_radius.x - ball_radius.x)
		{
			ball_in.x = court_radius.x - ball_radius.x;
			if (ball_velocity_in.x > 0.0f)
			{
				ball_velocity_in.x = -ball_velocity_in.x;
				// left_score += 1;
			}
		}
		if (ball_in.x < -court_radius.x + ball_radius.x)
		{
			ball_in.x = -court_radius.x + ball_radius.x;
			if (ball_velocity_in.x < 0.0f)
			{
				ball_velocity_in.x = -ball_velocity_in.x;
				// right_score += 1;
			}
		}
	};
	
	for (int i = 0; i < ball_num; i++) {
		court_wall(balls[i], ball_velocities[i]);
	}
	// court_wall(balls[0], ball_velocity);
	// court_wall(ball_2, ball_2_velocity);
	//----- rainbow trails -----
	
	auto rainbow_trail = [this](glm::vec2 &ball_in, std::deque< glm::vec3 > &ball_trail_in, float elapsed) {
		//age up all locations in ball trail:
		for (auto &t : ball_trail_in)
		{
			t.z += elapsed;
		}
		//store fresh location at back of ball trail:
		ball_trail_in.emplace_back(ball_in, 0.0f);

		//trim any too-old locations from back of trail:
		//NOTE: since trail drawing interpolates between points, only removes back element if second-to-back element is too old:
		while (ball_trail_in.size() >= 2 && ball_trail_in[1].z > trail_length)
		{
			ball_trail_in.pop_front();
		}
	};
	
	for (int i = 0; i < ball_num; i++) {
		rainbow_trail(balls[i], ball_trails[i], elapsed);
	}
	// rainbow_trail(balls[0], ball_trail, elapsed);
	// rainbow_trail(ball_2, ball_2_trail, elapsed);
}

void PongMode::draw(glm::uvec2 const &drawable_size)
{
//some nice colors from the course web page:
#define HEX_TO_U8VEC4(HX) (glm::u8vec4((HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX)&0xff))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0xf3ffc6ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0x000000ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xa5df40ff);
	const glm::u8vec4 light_color = HEX_TO_U8VEC4(0xabff0000);
	const std::vector<glm::u8vec4> rainbow_colors = {
		HEX_TO_U8VEC4(0xe2ff70ff), HEX_TO_U8VEC4(0xcbff70ff), HEX_TO_U8VEC4(0xaeff5dff),
		HEX_TO_U8VEC4(0x88ff52ff), HEX_TO_U8VEC4(0x6cff47ff), HEX_TO_U8VEC4(0x3aff37ff),
		HEX_TO_U8VEC4(0x2eff94ff), HEX_TO_U8VEC4(0x2effa5ff), HEX_TO_U8VEC4(0x17ffc1ff),
		HEX_TO_U8VEC4(0x00f4e7ff), HEX_TO_U8VEC4(0x00cbe4ff), HEX_TO_U8VEC4(0x00b0d8ff),
		HEX_TO_U8VEC4(0x00a5d1ff), HEX_TO_U8VEC4(0x0098cfd8), HEX_TO_U8VEC4(0x0098cf54),
		HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54),
		HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54),
		HEX_TO_U8VEC4(0x0098cf54)};
#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector<Vertex> vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//split rectangle into two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f, -shadow_offset);

	draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(left_paddle + s, paddle_radius, shadow_color);
	// draw_rectangle(right_paddle+s, paddle_radius, shadow_color);
	// draw_rectangle(balls[0] + s, ball_radius, shadow_color);
	// draw_rectangle(ball_2 + s, ball_radius, shadow_color);

	for (int i = 0; i < ball_num; i++) {
		draw_rectangle(balls[i] + s, ball_radius, shadow_color);
	}

	//ball's trail:
	
	for (int i = 0; i < ball_num; i++) {
		if (ball_trails[i].size() >= 2)
		{
			//start ti at second element so there is always something before it to interpolate from:
			std::deque<glm::vec3>::iterator ti = ball_trails[i].begin() + 1;
			//draw trail from oldest-to-newest:
			for (uint32_t i = uint32_t(rainbow_colors.size()) - 1; i < rainbow_colors.size(); --i)
			{
				//time at which to draw the trail element:
				float t = (i + 1) / float(rainbow_colors.size()) * trail_length;
				//advance ti until 'just before' t:
				while (ti != ball_trails[i].end() && ti->z > t)
					++ti;
				//if we ran out of tail, stop drawing:
				if (ti == ball_trails[i].end())
					break;
				//interpolate between previous and current trail point to the correct time:
				glm::vec3 a = *(ti - 1);
				glm::vec3 b = *(ti);
				glm::vec2 at = (t - a.z) / (b.z - a.z) * (glm::vec2(b) - glm::vec2(a)) + glm::vec2(a);
				//draw:
				draw_rectangle(at, ball_radius, rainbow_colors[i]);
			}
		}
	}

	//solid objects:

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	//paddles:
	draw_rectangle(left_paddle, paddle_radius, fg_color);
	// draw_rectangle(right_paddle, paddle_radius, fg_color);

	//ball:
	for (int i = 0; i < ball_num; i++) {
		if(i % 2 == 0) {
			draw_rectangle(balls[i], ball_radius, fg_color);
		} else {
			draw_rectangle(balls[i], ball_radius, light_color);
		}
	}
	// draw_rectangle(balls[0], ball_radius, fg_color);
	// draw_rectangle(ball_2, ball_radius, fg_color);

	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < left_score; ++i)
	{
		draw_rectangle(glm::vec2(-court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}
	for (uint32_t i = 0; i < right_score; ++i)
	{
		draw_rectangle(glm::vec2(court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y)		   //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f));
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so this matrix is actually transposed from how it appears.

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y));

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);														   //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.
}
