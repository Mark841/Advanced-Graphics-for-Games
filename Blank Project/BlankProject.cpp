#include "../nclgl/window.h"
#include "Renderer.h"

int main()	{
	Window w("Make your own project!", 1280, 720, false);
	if(!w.HasInitialised()) {
		return -1;
	}
	
	Renderer renderer(w);
	if(!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while(w.UpdateWindow()  && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		w.SetTitle(std::to_string(1.0f / w.GetTimer()->GetTimeDeltaSeconds()));
		renderer.RenderScene();
		renderer.SwapBuffers();
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_X)) {
			renderer.followWaypoints = false;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_P)) {
			renderer.ToggleDayNight();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) {
			renderer.SetWaves(1);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_2)) {
			renderer.SetWaves(2);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_3)) {
			renderer.SetWaves(3);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_4)) {
			renderer.SetWaves(4);
		}
	}
	return 0;
}