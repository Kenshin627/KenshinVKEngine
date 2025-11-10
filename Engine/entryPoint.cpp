#include "src/engine/kEngine.h"
#include "logger.h"

int main()
{
	Log::Init();
	KEngine engine(1280, 720);
	engine.init();
	engine.run();
	engine.cleanUp();
}