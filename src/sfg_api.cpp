#include "sfg_api.hpp"
#include "app/game_app.hpp"
#include "io/log.hpp"
#

extern "C"
{
	SFG::game_app* create_app(uint32 width, uint32 height)
	{
		SFG::game_app* app = new SFG::game_app();
		app->init({width, height});
		// app->tick();
		return app;
	}

	void destroy_app(SFG::game_app* app)
	{
		// app->uninit();
		delete app;
	}

}