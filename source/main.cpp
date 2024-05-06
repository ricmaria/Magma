#include <engine.h>

int main(int argc, char* argv[])
{
	MagmaEngine engine;

	engine.init();	
	
	engine.run();	

	engine.cleanup();	

	return 0;
}
