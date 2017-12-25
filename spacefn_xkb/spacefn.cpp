// Copyright (c) 2013, Pro XKB
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     - Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     - Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Original:
// g++ spacefn.cc -I/usr/X11R7/include -L/usr/X11R7/lib -rpath=/usr/X11R7/lib -lXtst -lX11 -o spacefn

// What worked:
// g++ spacefn.cpp -I/usr/X11R7/include -L/usr/X11R7/lib -Wl,--rpath -Wl,/usr/X11R7/lib -lXtst -lX11 -o spacefn

// Original: Then xmodmap -e "keycode 186 = space" -e "keycode 65 =" ; ./spacefn
// Then xmodmap -e "keycode 202 = space" -e "keycode 65 =" ; ./spacefn

#include <iostream>
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <poll.h>

// A few user-adjustable values follow below

enum { TRACKED_KEYCODE = 65,    // Keycode of a physical key.  The key should
                                // be unmapped, and autorepeat turned off.

       // 202 = FK24 on my layout
       // Original: SYMBOL_KEYCODE = 186,
       SYMBOL_KEYCODE = 202,    // Keycode not associated to a physical key.
                                // Map the symbols normally associated to
                                // TRACKED_KEYCODE key to this keycode.
                                // Autorepeat should be on.
       MODIFIER_MASK = Mod3Mask // Modifier mask
};

// If the physical key is held for shorter than this time (in ms) and if no
// other key was pressed between press and release of the physical key, the
// symbol keycode is programmatically hit (and immediatly released).
// Original: SYMBOL_TIMEOUT = 200,
const Time SYMBOL_TIMEOUT = 140;

// If the above holds, and after the first press of the physical key is pressed
// again within a time span shorter than the following time (in ms), the symbol
// key is hit programmatically again, and held until the physical key is
// released.
// Original: SYMBOL_HOLD_TIMEOUT = 250
const Time SYMBOL_HOLD_TIMEOUT = 200;


// If the physical key has been held for for the following time (in ms), the
// Modifier is set and remains set until the physical key is released.
const int MODIFIER_DELAY = 5;

// End of adjustable values

// 2017/12/01 12:03:35, AA: From https://stackoverflow.com/questions/28746744/passing-lambda-as-function-pointer
// OT => Object Type
// RT => Return Type
// A ... => Arguments
template<typename OT, typename RT, typename ... A>
struct lambda_expression {
    OT _object;
    RT(OT::*_function)(A...)const;

    lambda_expression(const OT & object)
        : _object(object), _function(&decltype(_object)::operator()) {}

    RT operator() (A ... args) const {
        return (_object.*_function)(args...);
    }
};


enum { N = 2 };
KeyCode pressed[N] = { 0, 0 } , released[N] = { 0, 0 };
Time presstime[N] = { 0, 0 }, releasetime[N] = { 0, 0 };

bool mod_set = false, symbol_down = false;
int delay = -1;

auto create_callback(Time symbol_timeout, Time symbol_hold_timeout, int modifier_delay, int modifier_mask, int tracked_keycode, int symbol_keycode){

    auto to_return = [=](XPointer priv, XRecordInterceptData *id){
        if(id->category == XRecordFromServer) {
            xEvent* xe = (xEvent*)id->data;
            int num = id->data_len/8;
            Time now = id->server_time;

            while(num){
                const int type        = xe->u.u.type;
                const KeyCode keycode = xe->u.u.detail;
                Display* dpy = (Display*)priv;

                if(type == KeyPress){
                    if(keycode == tracked_keycode){
                        if(pressed[N-2] == tracked_keycode &&
                                pressed[N-1] == symbol_keycode &&
                                now-presstime[N-2] < symbol_hold_timeout){
                            XTestFakeKeyEvent(dpy, symbol_keycode, True,  CurrentTime);
                            symbol_down = true;
                        }else{
                            delay = modifier_delay;
                        }
                    }

                    for(int i = 1; i < N; ++i){
                        pressed[i-1] = pressed[i];
                        presstime[i-1] = presstime[i];
                    }
                    pressed[N-1] = keycode;
                    presstime[N-1] = now;
                }else if(type == KeyRelease){
                    if (keycode == tracked_keycode){
                        if(symbol_down){
                            XTestFakeKeyEvent(dpy, symbol_keycode, False, CurrentTime);
                            symbol_down = false;
                        }
                        if(mod_set){
                            XkbLockModifiers(dpy, XkbUseCoreKbd, modifier_mask, 0);
                            mod_set = false;
                        }
                        if(keycode == pressed[N-1]){
                            if(now-presstime[N-1] < symbol_timeout){
                                XTestFakeKeyEvent(dpy, symbol_keycode, True,  CurrentTime);
                                XTestFakeKeyEvent(dpy, symbol_keycode, False, CurrentTime);
                            }
                        }
                    }

                    for(int i = 1; i < N; ++i){
                        released[i-1] = released[i];
                        releasetime[i-1] = releasetime[i];
                    }
                    released[N-1] = keycode;
                    releasetime[N-1] = now;
                }

                xe++; num--;
            }
        }
        XRecordFreeData(id);
    };

    return lambda_expression<decltype(to_return), XPointer, XRecordInterceptData *, void *>(to_return);
}


void error(const char* msg){
  std::cerr << "Error: " << msg << std::endl;
  exit(1);
}

int main(int argn, char** argv){
  Time symbol_timeout = 140;
  Time symbol_hold_timeout = 200;
  int modifier_delay = 5;
  int modifier_mask = Mod3Mask;
  int tracked_keycode = 65;
  int symbol_keycode = 202;

  const auto callback = create_callback(symbol_timeout, symbol_hold_timeout, modifier_delay, modifier_mask, tracked_keycode, symbol_keycode);

  XRecordRange* rr = XRecordAllocRange();
  if(!rr) error("Cannot allocate XRecordRange.");
  memset(rr, 0, sizeof(XRecordRange));
  rr->device_events.first = KeyPress;
  rr->device_events.last  = KeyRelease;

  const char* dn = argn < 2 ? 0 : argv[1];
  Display* cdpy = XOpenDisplay(dn);
  Display* ddpy = XOpenDisplay(dn);
  if(!cdpy) error("Cannot open display.");

  int r1, r2;
  if(!XRecordQueryVersion(cdpy, &r1, &r2))
    error("XRecord extension not active.");

  XRecordClientSpec rs = XRecordAllClients;
  XRecordContext rc = XRecordCreateContext(cdpy, 0, &rs, 1, &rr, 1);
  if(!rc) error("Cannot create XRecordContext.");

  XSynchronize(ddpy, True);
  if(!XRecordEnableContextAsync(cdpy, rc, callback, (XPointer)ddpy))
    error("Cannot enable recording.");

  pollfd pf = { ConnectionNumber(cdpy), POLLIN | POLLRDNORM | POLLRDBAND, 0 };
  while(true){
    if(poll(&pf, 1, delay) > 0){
      delay = -1;
      XRecordProcessReplies(cdpy);
    }else{
      delay = -1;
      XkbLockModifiers(ddpy, XkbUseCoreKbd, modifier_mask, modifier_mask);
      mod_set = true;
    }
  }
  return 0;
}
