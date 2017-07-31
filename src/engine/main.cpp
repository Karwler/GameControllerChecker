#include "world.h"
#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_WIN32) && !defined(_DEBUG)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#else
int main(int argc, char** argv) {
#endif
	int retval = 0;
	try {
		World::engine()->Run();
	} catch (Exception exc) {
		exc.Display();
		retval = exc.retval;
	} catch (std::logic_error exc) {
		cerr << "ERROR: " << exc.what() << endl;
		retval = -3;
	} catch (std::runtime_error exc) {
		cerr << "ERROR: " << exc.what() << endl;
		retval = -2;
	} catch (...) {
		cerr << "unknown error" << endl;
		retval = -1;
	}
	World::engine()->Cleanup();
	return retval;
}