#include "sdl_runner_application.hpp"

#if defined(__ANDROID__)
#include <SDL3/SDL_main.h>
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
#else
int main() {
#endif
    xno::runner::platform::SdlRunnerApplication application;
    return application.run();
}
