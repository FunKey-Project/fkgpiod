# fkgpiod
fkgpiod version 0.0.1
FunKey S GPIO daemon

Copyright (C) 2020-2021, Vincent Buso <vincent.buso@funkey-project.com>,
Copyright (C) 2021, Michel Stempin  <michel.stempin@funkey-project.com>,
All rights reserved.
Released under the GNU Lesser General Public License version 2.1 or later

```
Usage: fkgpiod [options] [config_file]
Options:
 -d, -D, --daemonize                                Launch as a background daemon
 -h, -H, --help                                     Print option help
 -k, -K, --kill                                     Kill background daemon
 -v, --version                                      Print version information
```

## Available script commands (commands are not case sensitive):

```
DUMP                                                Dump the button mapping
KEYDOWN <key_code>                                  Send a key down event with the given keycode
KEYPRESS <key_code>                                 Send key press event with the given keycode
KEYUP <key_code>                                    Send a key up event with the given keycode
LOAD <configuration_file>                           Load a configuration file
MAP <button_combination> TO KEY <key_code>          Map a button combination to a keycode
MAP <button_combination> TO COMMAND <shell_command> Map a button combination to a Shell command
RESET                                               Reset the button mapping
SLEEP <delays_ms>                                   Sleep for the given delay in ms
TYPE <character_string>                             Type in a character string
UNMAP <button_combination>                          Unmap a button combination
```

where:
 - <button_combination> is a list of UP, DOWN, LEFT, RIGHT, A, B, L, R, X, Y, MENU, START or FN
   separated by "+" signs
 - <shell_command> is any valid Shell command with its arguments
 - <configuration_file> is the full path to a configurtion file
 - <delay_ms> is a delay in ms
 - <character_string> is a character string
 - <key_code> is among:
   - KEY_0 to KEY_9, KEY_A to KEY_Z
   - KEY_F1 to KEY_F24, KEY_KP0 to KEY_KP9, KEY_PROG1 to KEY_PROG4
   - BTN_0 to BTN_9, BTN_A to BTN_C, BTN_X to BTN_Z, BTN_BASE2 to BTN_BASE6
   - BTN_BACK, BTN_BASE, BTN_DEAD, BTN_EXTRA, BTN_FORWARD, BTN_GAMEPAD, BTN_JOYSTICK, BTN_LEFT,
     BTN_MIDDLE, BTN_MISC, BTN_MODE, BTN_MOUSE, BTN_PINKIE, BTN_RIGHT, BTN_SELECT, BTN_SIDE,
     BTN_START, BTN_TASK, BTN_THUMB, BTN_THUMB2, BTN_THUMBL, BTN_THUMBR, BTN_TL, BTN_TL2, 
     BTN_TOP, BTN_TOP2, BTN_TR, BTN_TR2, BTN_TRIGGER,
   - KEY_102ND, KEY_AGAIN, KEY_ALTERASE, KEY_APOSTROPHE, KEY_BACK, KEY_BACKSLASH, KEY_BACKSPACE,
     KEY_BASSBOOST, KEY_BATTERY, KEY_BLUETOOTH, KEY_BOOKMARKS, KEY_BRIGHTNESSDOWN,
     KEY_BRIGHTNESSUP, KEY_BRIGHTNESS_CYCLE, KEY_BRIGHTNESS_ZERO, KEY_CALC, KEY_CAMERA,
     KEY_CANCEL, KEY_CAPSLOCK, KEY_CHAT, KEY_CLOSE, KEY_CLOSECD, KEY_COFFEE, KEY_COMMA,
     KEY_COMPOSE, KEY_COMPUTER, KEY_CONFIG, KEY_CONNECT, KEY_COPY, KEY_CUT, KEY_CYCLEWINDOWS,
     KEY_DASHBOARD, KEY_DELETE, KEY_DELETEFILE, KEY_DIRECTION, KEY_DISPLAY_OFF, KEY_DOCUMENTS,
     KEY_DOT, KEY_DOWN, KEY_EDIT, KEY_EJECTCD, KEY_EJECTCLOSECD, KEY_EMAIL, KEY_END, KEY_ENTER,
     KEY_EQUAL, KEY_ESC, KEY_EXIT, KEY_FASTFORWARD, KEY_FILE, KEY_FINANCE, KEY_FIND,
     KEY_FORWARD, KEY_FORWARDMAIL, KEY_FRONT, KEY_GRAVE, KEY_HANGEUL, KEY_HANGUEL, KEY_HANJA,
     KEY_HELP, KEY_HENKAN, KEY_HIRAGANA, KEY_HOME, KEY_HOMEPAGE, KEY_HP, KEY_INSERT, KEY_ISO,
     KEY_KATAKANA, KEY_KATAKANAHIRAGANA, KEY_KBDILLUMDOWN, KEY_KBDILLUMTOGGLE, KEY_KBDILLUMUP,
     KEY_KPASTERISK,KEY_KPCOMMA, KEY_KPDOT, KEY_KPENTER, KEY_KPEQUAL, KEY_KPJPCOMMA,
     KEY_KPLEFTPAREN, KEY_KPMINUS, KEY_KPPLUS, KEY_KPPLUSMINUS, KEY_KPRIGHTPAREN, KEY_KPSLASH,
     KEY_LEFT, KEY_LEFTALT, KEY_LEFTBRACE, KEY_LEFTCTRL, KEY_LEFTMETA, KEY_LEFTSHIFT,
     KEY_LINEFEED, KEY_MACRO, KEY_MAIL, KEY_MEDIA, KEY_MENU, KEY_MICMUTE, KEY_MINUS, KEY_MOVE,
     KEY_MSDOS, KEY_MUHENKAN, KEY_MUTE, KEY_NEW, KEY_NEXTSONG, KEY_NUMLOCK, KEY_OPEN,
     KEY_PAGEDOWN, KEY_PAGEUP, KEY_PASTE, KEY_PAUSE, KEY_PAUSECD, KEY_PHONE, KEY_PLAY,
     KEY_PLAYCD, KEY_PLAYPAUSE, KEY_POWER, EY_PREVIOUSSONG, KEY_PRINT, KEY_PROPS, KEY_QUESTION,
     KEY_RECORD, KEY_REDO, KEY_REFRESH, KEY_REPLY, KEY_REWIND, KEY_RFKILL, KEY_RIGHT,
     KEY_RIGHTALT, KEY_RIGHTBRACE, KEY_RIGHTCTRL, KEY_RIGHTMETA, KEY_RIGHTSHIFT, KEY_RO,
     KEY_SAVE, KEY_SCALE, KEY_SCREENLOCK, KEY_SCROLLDOWN, KEY_SCROLLLOCK, KEY_SCROLLUP,
     KEY_SEARCH, KEY_SEMICOLON, KEY_SEND, KEY_SENDFILE, KEY_SETUP, KEY_SHOP, KEY_SLASH,
     KEY_SLEEP, KEY_SOUND, KEY_SPACE, KEY_SPORT, KEY_STOP, KEY_STOPCD, KEY_SUSPEND,
     KEY_SWITCHVIDEOMODE, KEY_SYSRQ, KEY_TAB, KEY_UNDO, KEY_UNKNOWN, KEY_UP, KEY_UWB,
     KEY_VIDEO_NEXT, KEY_VIDEO_PREV, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_WAKEUP, KEY_WIMAX,
     KEY_WLAN, KEY_WWW, KEY_XFER, KEY_YEN, KEY_ZENKAKUHANKAKU
