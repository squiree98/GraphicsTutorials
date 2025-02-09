#include "../NCLGL/window.h"
#include "Renderer.h"

int main()	{
	Window w("Coursework :-)", 1920, 1080, true);

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
		// press back to change camera view
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_BACK))
			renderer.ChangeFreeMovement();
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_TAB))
			renderer.ChangeScene();
		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		renderer.SwapBuffers();
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}
	return 0;
}