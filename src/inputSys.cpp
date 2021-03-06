#include "inputSys.h"
#include "objects.h"
#include "program.h"

// CONTROLLER

bool InputSys::Controller::Open(int id) {
	gamepad = SDL_IsGameController(id) ? SDL_GameControllerOpen(id) : nullptr;
	joystick = gamepad ? SDL_GameControllerGetJoystick(gamepad) : SDL_JoystickOpen(id);
	if (SDL_JoystickIsHaptic(joystick) == SDL_TRUE) {
		haptic = SDL_HapticOpenFromJoystick(joystick);
		SDL_HapticRumbleInit(haptic);
	} else
		haptic = nullptr;
	return joystick;
}

void InputSys::Controller::Close() {
	if (haptic)
		SDL_HapticClose(haptic);
	if (gamepad)
		SDL_GameControllerClose(gamepad);
	else if (joystick)
		SDL_JoystickClose(joystick);
}

// INPUT SYS

InputSys::InputSys(WindowSys* window) :
	program(new Program(window))
{
	SetCapture(nullptr);

	for (int i = 0; i < SDL_NumJoysticks(); ++i)
		if (Controller dev; dev.Open(i))
			controllers.emplace(SDL_JoystickInstanceID(dev.joystick), dev);
	curDevice = !controllers.empty() ? controllers.begin()->first : -1;
}

InputSys::~InputSys() {
	for (auto& [id, ct] : controllers)
		ct.Close();
	delete program;
}

void InputSys::MouseMotionEvent(const SDL_MouseMotionEvent& motion, const vector<Object*>& objects) {
	if (scrollHold) {
		if (HorSlider* slider = dynamic_cast<HorSlider*>(scrollHold))
			slider->DragSlider(motion.x);
		else if (ScrollArea* area = dynamic_cast<ScrollArea*>(scrollHold))
			area->DragSlider(motion.y);
	} else for (Object* obj : objects)
		if (InRect({motion.x, motion.y}, obj->Rect())) {
			mouseOver = obj;
			break;
		}
}

void InputSys::MouseButtonDownEvent(const SDL_MouseButtonEvent& button) {
	if (captured)
		captured->Confirm();
	if (button.button != SDL_BUTTON_LEFT || !mouseOver)
		return;

	if (Button* but = dynamic_cast<Button*>(mouseOver))
		but->OnClick();
	else if (LineEditor* edt = dynamic_cast<LineEditor*>(mouseOver))
		edt->OnClick();
	else if (HorSlider* slider = dynamic_cast<HorSlider*>(mouseOver))
		CheckSliderClick(slider, button.x);
	else if (ScrollArea* area = dynamic_cast<ScrollArea*>(mouseOver))
		CheckSliderClick(area, {button.x, button.y});
}

void InputSys::CheckSliderClick(HorSlider* obj, int mx) {
	scrollHold = obj;
	if (mx < obj->SliderX() || mx > obj->SliderX() + HorSlider::sliderW)
		obj->DragSlider(mx - HorSlider::sliderW / 2);
	obj->diffSliderMouseX = mx - obj->SliderX();
}

void InputSys::CheckSliderClick(ScrollArea* obj, SDL_Point mPos) {
	if (InRect(mPos, obj->Bar())) {
		scrollHold = obj;
		if (mPos.y < obj->SliderY() || mPos.y > obj->SliderY() + obj->SliderH())	// if mouse outside of slider but inside bar
			obj->DragSlider(mPos.y - obj->SliderH() /2);
		obj->diffSliderMouseY = mPos.y - obj->SliderY();	// get difference between mouse y and slider y
	}
}

void InputSys::MouseButtonUpEvent(const SDL_MouseButtonEvent& button, const vector<Object*>& objects) {
	if (button.button == SDL_BUTTON_LEFT && scrollHold) {
		if (HorSlider* slider = dynamic_cast<HorSlider*>(scrollHold))
			slider->diffSliderMouseX = 0;
		else if (ScrollArea* area = dynamic_cast<ScrollArea*>(scrollHold))
			area->diffSliderMouseY = 0;

		scrollHold = nullptr;
		SimulateMouseMove(objects);
	}
}

void InputSys::MouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(mouseOver))
		box->ScrollList(int(float(wheel.y) * -scrollFactor));
}

void InputSys::KeypressEvent(const SDL_KeyboardEvent& key) {
	if (captured)
		captured->OnKeypress(key.keysym.scancode);
	else if (!key.repeat)
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_LEFT:
			program->EventOpenPrevJoystick();
			break;
		case SDL_SCANCODE_RGUI:
			program->EventOpenNextJoystick();
			break;
		case SDL_SCANCODE_1: case SDL_SCANCODE_2: case SDL_SCANCODE_3: case SDL_SCANCODE_4: case SDL_SCANCODE_5: case SDL_SCANCODE_6: case SDL_SCANCODE_7: case SDL_SCANCODE_8: case SDL_SCANCODE_9: case SDL_SCANCODE_0:
			program->EventOpenJoystick(key.keysym.scancode - SDL_SCANCODE_1);
			break;
		case SDL_SCANCODE_KP_1: case SDL_SCANCODE_KP_2: case SDL_SCANCODE_KP_3: case SDL_SCANCODE_KP_4: case SDL_SCANCODE_KP_5: case SDL_SCANCODE_KP_6: case SDL_SCANCODE_KP_7: case SDL_SCANCODE_KP_8: case SDL_SCANCODE_KP_9: case SDL_SCANCODE_KP_0:
			program->EventOpenJoystick(key.keysym.scancode - SDL_SCANCODE_KP_1);
			break;
		case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
			program->EventEnter();
			break;
		case SDL_SCANCODE_ESCAPE:
			program->EventEscape();
		}
}

void InputSys::TextEvent(const SDL_TextInputEvent& text) {
	captured->OnText(text.text);
}

void InputSys::Tick(uint32_t dMsec) {
	if (curRumbling) {
		if (uint32_t val = curRumbling - dMsec; val && val <= curRumbling)
			curRumbling = val;
		else {
			curRumbling = 0;
			program->EventTestStopped();
		}
	}
}

void InputSys::NextController() {
	umap<SDL_JoystickID, Controller>::iterator it = controllers.find(curDevice);
	curDevice = it != std::prev(controllers.end()) ? std::next(it)->first : controllers.begin()->first;
}

void InputSys::PrevController() {
	umap<SDL_JoystickID, Controller>::iterator it = controllers.find(curDevice);
	curDevice = it != controllers.begin() ? std::prev(it)->first : std::prev(controllers.end())->first;
}

void InputSys::FindController(size_t i) {
	if (i < controllers.size()) {
		umap<SDL_JoystickID, Controller>::iterator it = controllers.begin();
		for (; i--; ++it);
		curDevice = it->first;
	}
}

bool InputSys::StartRumble(float strength, uint32_t length) {
	if (SDL_HapticRumblePlay(controllers[curDevice].haptic, strength, length) != -1) {
		curRumbling = length;
		return true;
	}
	return false;
}

void InputSys::StopRumble() {
	curRumbling = 0;
	SDL_HapticRumbleStop(controllers.at(curDevice).haptic);
	program->EventTestStopped();
}

vector<uint8_t> InputSys::GetJoystickButtons() const {
	vector<uint8_t> buttons;
	for (int i = 0; i < SDL_JoystickNumButtons(controllers.at(curDevice).joystick); ++i)
		if (SDL_JoystickGetButton(controllers.at(curDevice).joystick, i))
			buttons.push_back(i);
	return buttons;
}

vector<pair<uint8_t, uint8_t>> InputSys::GetJoystickHats() const {
	vector<pair<uint8_t, uint8_t>> hats(SDL_JoystickNumHats(controllers.at(curDevice).joystick));
	for (int i = 0; i < int(hats.size()); ++i)
		hats[i] = pair(i, SDL_JoystickGetHat(controllers.at(curDevice).joystick, i));
	return hats;
}

vector<pair<uint8_t, int16_t>> InputSys::GetJoystickAxes() const {
	vector<pair<uint8_t, int16_t>> axes(SDL_JoystickNumAxes(controllers.at(curDevice).joystick));
	for (int i = 0; i < int(axes.size()); ++i)
		axes[i] = pair(i, SDL_JoystickGetAxis(controllers.at(curDevice).joystick, i));
	return axes;
}

vector<SDL_GameControllerButton> InputSys::GetGamepadButtons() const {
	vector<SDL_GameControllerButton> buttons;
	for (uint8_t i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
		if (SDL_GameControllerGetButton(controllers.at(curDevice).gamepad, SDL_GameControllerButton(i)))
			buttons.push_back(SDL_GameControllerButton(i));
	return buttons;
}

vector<pair<SDL_GameControllerAxis, int16_t>> InputSys::GetGamepadAxes() const {
	vector<pair<SDL_GameControllerAxis, int16_t>> axes(SDL_CONTROLLER_AXIS_MAX);
	for (uint8_t i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
		axes[i] = pair(SDL_GameControllerAxis(i), SDL_GameControllerGetAxis(controllers.at(curDevice).gamepad, SDL_GameControllerAxis(i)));
	return axes;
}

const char* InputSys::JoystickName() const {
	return SDL_JoystickName(controllers.at(curDevice).joystick);
}

const char* InputSys::GamepadName() const {
	return SDL_GameControllerName(controllers.at(curDevice).gamepad);
}

int InputSys::NumJButtons() const {
	return SDL_JoystickNumButtons(controllers.at(curDevice).joystick);
}

int InputSys::NumJHats() const {
	return SDL_JoystickNumHats(controllers.at(curDevice).joystick);
}

int InputSys::NumJAxes() const {
	return SDL_JoystickNumAxes(controllers.at(curDevice).joystick);
}

bool InputSys::IsGamepad() const {
	return controllers.at(curDevice).gamepad;
}

bool InputSys::IsHaptic() const {
	return controllers.at(curDevice).haptic;
}

SDL_JoystickID InputSys::CurDevice() const {
	return curDevice;
}

size_t InputSys::CurIndex() const {
	size_t cnt = 0;
	for (umap<SDL_JoystickID, Controller>::const_iterator it = controllers.begin(); it != controllers.end() && it->first != curDevice; ++it, ++cnt);
	return cnt;
}

size_t InputSys::NumControllers() const {
	return controllers.size();
}

void InputSys::AddController(int id) {
	if (Controller dev; dev.Open(id))
		controllers.emplace(SDL_JoystickInstanceID(dev.joystick), dev);
	if (curDevice == -1)
		curDevice = controllers.begin()->first;
	program->EventControllersChanged(false);
}

void InputSys::DelController(SDL_JoystickID id) {
	if (controllers.erase(id); id == curDevice)
		curDevice = !controllers.empty() ? controllers.begin()->first : -1;
	program->EventControllersChanged(id == curDevice);
}

void InputSys::PostMenuSwitch(const vector<Object*>& objects) {
	scrollHold = nullptr;
	SetCapture(nullptr);
	SimulateMouseMove(objects);
}

void InputSys::SimulateMouseMove(const vector<Object*>& objects) {
	SDL_MouseMotionEvent me{};
	me.state = SDL_GetMouseState(&me.x, &me.y);
	MouseMotionEvent(me, objects);
}

LineEditor* InputSys::Captured() {
	return captured;
}

void InputSys::SetCapture(LineEditor* cbox) {
	if (captured = cbox; captured)
		SDL_StartTextInput();
	else
		SDL_StopTextInput();
}

bool InputSys::IsSelected(const Object* obj) const {
	return obj == mouseOver;
}

Program* InputSys::Prog() {
	return program;
}
