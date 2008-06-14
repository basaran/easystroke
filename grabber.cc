#include "grabber.h"
#include "main.h"
#include <X11/extensions/XTest.h>
#include <X11/Xutil.h>

Grabber::Grabber() {
	current.grab = false;
	current.suspend = false;
	current.all = false;
	get_button();
	wm_state = XInternAtom(dpy, "WM_STATE", False);

	xinput = init_xi();

	init(ROOT, 0);
}

bool Grabber::init_xi() {
	if (!experimental)
		return false;
	int nMajor, nFEV, nFER;
	if (!XQueryExtension(dpy,INAME,&nMajor,&nFEV,&nFER))
		return false;

	int i, n;
	XDeviceInfo *devs, *dev = 0;
	devs = XListInputDevices(dpy, &n);
	if (!devs)
		return false;

	for (i=0; i<n; ++i)
		if (!strcasecmp(devs[i].name,"stylus") && devs[i].num_classes)
			dev = devs + i;

	if (!dev)
		return false;

	xi_dev = XOpenDevice(dpy,dev->id);

	if (!xi_dev)
		return false;

	int dummy = 0;
	DeviceButtonPress(xi_dev, button_down, button_events[0]);
	DeviceButtonRelease(xi_dev, button_up, button_events[1]);
	DeviceMotionNotify(xi_dev, motion_notify, button_events[2]);
	DeviceButtonMotion(xi_dev, &dummy, button_events[3]);
	button_events_n = 0;

	return true;
}

// Fuck Xlib
bool Grabber::has_wm_state(Window w) {
	Atom actual_type_return;
	int actual_format_return;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	if (Success != XGetWindowProperty(dpy, w, wm_state, 0, 2, False,
				AnyPropertyType, &actual_type_return,
				&actual_format_return, &nitems_return,
				&bytes_after_return, &prop_return))
		return false;
	XFree(prop_return);
	return nitems_return;
}

// Calls create on top-level windows, i.e. windows that have th wm_state property.
void Grabber::init(Window w, int depth) {
	depth++;
	// I have no clue why this is needed, but just querying the whole tree
	// prevents EnterNotifies from being generated.  Weird.
	// update 2/12/08:  Disappeared.  Leaving the code here in case this
	// comes back
	// 2/15/08: put it back in for release.  You never know.
	if (depth > 2)
		return;
	unsigned int n;
	Window dummyw1, dummyw2, *ch;
	XQueryTree(dpy, w, &dummyw1, &dummyw2, &ch, &n);
	for (unsigned int i = 0; i != n; i++) {
		if (!has_wm_state(ch[i]))
			init(ch[i], depth);
		else
			create(ch[i]);
	}
	XFree(ch);
}

void Grabber::set(State s) {
	Goal old_goal = goal(current);
	Goal new_goal = goal(s);
	current = s;
	if (old_goal == new_goal)
		return;
	if (old_goal == BUTTON) {
		printf("Ungrabbing Device\n");
		if (xinput)
			XUngrabDeviceButton(dpy, xi_dev, button, state, NULL, ROOT);
		XUngrabButton(dpy, button, state, ROOT);
	}
	if (old_goal == ALL)
		XUngrabButton(dpy, AnyButton, AnyModifier, ROOT);
	if (new_goal == BUTTON) {
		while (!XGrabButton(dpy, button, state, ROOT, False, ButtonMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None)) {
			printf("Grab failed, retrying...\n");
			usleep(10000);
		}
		// All we need to do here is make sure that xi events aren't passed along to the application,
		// which - surpisingly enough - even an "empty" grab accomplishes
		while (xinput && XGrabDeviceButton(dpy, xi_dev, button, state, NULL, ROOT, False, 0, 0, GrabModeAsync, GrabModeAsync)) {
			printf("Grab failed, retrying...\n");
			usleep(10000);
		}
	}
	if (new_goal == ALL) {
		while (!XGrabButton(dpy, AnyButton, AnyModifier, ROOT, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None)) {
			printf("Grab failed, retrying...\n");
			usleep(10000);
		}
	}
}

void Grabber::get_button() {
	Ref<ButtonInfo> ref(prefs().button);
	button = ref->button;
	state = ref->state;
}

void Grabber::fake_button() {
	suspend();
	XTestFakeButtonEvent(dpy, button, True, CurrentTime);
	XTestFakeButtonEvent(dpy, button, False, CurrentTime);
	restore();
}

// Wow, this is such a crazy hack, I'm really surprised it works
void Grabber::ignore(int b) {
	// Make the X Server think the state of the button is up
	XTestFakeButtonEvent(dpy, b, False, CurrentTime);
	// ungrab
	State s = current;
	s.suspend = true;
	s.all = false;
	set(s);
	// The button is not grabbed now, so this fake event gets passed to the app
	before();
	XTestFakeButtonEvent(dpy, b, True, CurrentTime);
	after();
	// Resulting motion events are not reported to us, even if we re-grab right away
	restore();
}

void Grabber::create(Window w) {
	XClassHint ch;
	if (!XGetClassHint(dpy, w, &ch))
		return;
	wins[w] = ch.res_name;
	XFree(ch.res_name);
	XFree(ch.res_class);
	XSetWindowAttributes attr;
	attr.event_mask = EnterWindowMask;
	XChangeWindowAttributes(dpy, w, CWEventMask, &attr);
}
