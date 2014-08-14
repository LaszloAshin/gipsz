@set echo off
set verbosity MLDBG
version
unbindall
bind tilde "@toggle console" push
bind w "@move forward"
bind s "@move back"
bind a "@move left"
bind d "@move right"
bind space "@move up"
bind c "@move down"
bind left "@turn left"
bind right "@turn right"
bind mouse_left "@attack" push
bind mouse_right "@zoom"
bind f1 "@toggle gravity" push
bind f2 "@toggle clip" push
bind f3 "@toggle r_drawwalls" push
bind f4 "@leavemap" push
bind f5 "@toggle r_clear" push
bind f10 @quit push
set echo on
