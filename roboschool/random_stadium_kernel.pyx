import numpy as np
import tqdm
import math

# ------------ library -------------

cdef float norm(x,y):
    return math.sqrt(x*x + y*y)

cdef float abs(float x):
    return x if x>0 else -x

cdef float relu(float x):
    return 0.5*(x + abs(x))

cdef float rect(float x, float y, float w, float h, float linewidth):
    return abs( max(abs(x)-w, abs(y)-h) ) - linewidth

cdef float filled_rect(float x, float y, float w, float h):
    return max(abs(x)-w, abs(y)-h)

cdef float circle(float x, float y, float rad, float linewidth):
    return abs(norm(x, y) - rad) - linewidth


# --------------- football stadium -------------

DEF GREEN_SQUARE = 26
DEF RUN_RAD = 21
DEF RUN_STARTPAD  = 29
DEF RUN_STARTLINE = 27
DEF ELONGATION = 15
DEF RUNSTRIP = 3.0/2

DEF WHITE_STRIP = 0.05
DEF WHITE_RUN_SPACING = 1.0

cdef float stadium_green_field(float x, float y):
    return filled_rect(x, y, GREEN_SQUARE+ELONGATION, GREEN_SQUARE)
    #return relu(abs(x)-GREEN_SQUARE-ELONGATION) + relu(abs(y)-GREEN_SQUARE) - 1

cdef float stadium_running_area(float x, float y):
    return min(
        abs( norm(relu(abs(x)-ELONGATION), y) - RUN_RAD) - RUNSTRIP,
        filled_rect(x, y+RUN_RAD, RUN_STARTPAD, RUNSTRIP)
        )

cdef float white_bubble(float x, float y, float n):
    return min(
        abs( norm(relu(abs(x)-ELONGATION), y) - RUN_RAD - n*WHITE_RUN_SPACING) - WHITE_STRIP,
        filled_rect(x, y + RUN_RAD + n*WHITE_RUN_SPACING, RUN_STARTPAD, WHITE_STRIP)
        )

cdef float stadium_running_area_whitelines(float x, float y):
    return min(
        white_bubble(x, y, -1.5),
        white_bubble(x, y, -0.5),
        white_bubble(x, y, +0.5),
        white_bubble(x, y, +1.5),
        rect(x, y + RUN_RAD, RUN_STARTPAD, WHITE_RUN_SPACING*1.5, WHITE_STRIP),
        filled_rect(x - RUN_STARTLINE, y + RUN_RAD, WHITE_STRIP*2, RUNSTRIP),  # start line
        filled_rect(x + RUN_STARTLINE, y + RUN_RAD, WHITE_STRIP*2, RUNSTRIP),  # finish line
        )

# FIFA football field, smaller by factor of K
# https://upload.wikimedia.org/wikipedia/en/9/96/Football_pitch_metric_and_imperial.svg
DEF K = 0.25
DEF GOAL_HALF_WID = 7.32/2
DEF FOOBALL_FIELD_HALFLEN = 105*K
DEF FOOBALL_FIELD_HALFWID = 50*K   # 45..90 meters
DEF PENALTY_ZONE_LEN = (16.5)*K
DEF PENALTY_ZONE_WID = (GOAL_HALF_WID+5.5+11)*K
DEF CENTER_CIRCLE = (9.15)*K

cdef float stadium_white(float x, float y):
    return min(
        stadium_running_area_whitelines(x, y),
        rect(x, y, FOOBALL_FIELD_HALFLEN, FOOBALL_FIELD_HALFWID, WHITE_STRIP),
        rect(x-FOOBALL_FIELD_HALFLEN+PENALTY_ZONE_LEN/2, y, PENALTY_ZONE_LEN/2, PENALTY_ZONE_WID, WHITE_STRIP),
        rect(x+FOOBALL_FIELD_HALFLEN-PENALTY_ZONE_LEN/2, y, PENALTY_ZONE_LEN/2, PENALTY_ZONE_WID, WHITE_STRIP),
        rect(x, y, 0, FOOBALL_FIELD_HALFWID, WHITE_STRIP),
        circle(x, y, CENTER_CIRCLE, WHITE_STRIP)
        )

def stadium_color(x, y):
    "return -1 0 white 1 yellow 2 green"
    g = stadium_white(x, y)
    if g < 0: return 0
    r = stadium_running_area(x, y)
    if r < 0: return 1
    g = stadium_green_field(x, y)
    if g < 0: return 2
    return -1



# -------------- pong ---------------

DEF PONG_WHITESTRIP = 0.016
DEF PONG_HALFLEN    = 1.6
DEF PONG_PLAYER     = 1.3
DEF PONG_PLAYER_HOR = 0.3
DEF PONG_HALFWIDTH  = 1.0

cdef float pong_white(x, y):
    return min(
        rect(x, y, PONG_HALFLEN, PONG_HALFWIDTH, PONG_WHITESTRIP),
        rect(x - PONG_PLAYER, y, PONG_PLAYER_HOR, PONG_HALFWIDTH, PONG_WHITESTRIP),
        rect(x + PONG_PLAYER, y, PONG_PLAYER_HOR, PONG_HALFWIDTH, PONG_WHITESTRIP),
        circle(x, y, 0.1, PONG_WHITESTRIP)
        )

cdef float pong_yellow(x, y):
    return filled_rect(x, y, PONG_HALFLEN, PONG_HALFWIDTH)

cdef float pong_green(x, y):
    return filled_rect(x, y, 2*PONG_HALFLEN, 2*PONG_HALFWIDTH)

def pong_color(x, y):
    g = pong_white(x, y)
    if g < 0: return 0
    r = pong_yellow(x, y)
    if r < 0: return 1
    g = pong_green(x, y)
    if g < 0: return 2


# tennis
# https://image.shutterstock.com/z/stock-vector-tennis-court-with-dimensions-grass-105644240.jpg

DEF TENNIS_SCALE_DOWN = 0.5
DEF TENNIS_WHITESTRIP = 0.02
DEF TENNIS_HALFLEN    = 23.77/2 * TENNIS_SCALE_DOWN
DEF TENNIS_HALFWIDTH  =  8.23/2 * TENNIS_SCALE_DOWN   # 10.97 "for doubles"
DEF TENNIS_SERVICE_AREA = 6.4   * TENNIS_SCALE_DOWN

cdef float tennis_white(x, y):
    return min(
        rect(x, y, TENNIS_HALFLEN, TENNIS_HALFWIDTH, TENNIS_WHITESTRIP),
        rect(x - TENNIS_SERVICE_AREA/2, y-TENNIS_HALFWIDTH/2, TENNIS_SERVICE_AREA/2, TENNIS_HALFWIDTH/2, TENNIS_WHITESTRIP),
        rect(x - TENNIS_SERVICE_AREA/2, y+TENNIS_HALFWIDTH/2, TENNIS_SERVICE_AREA/2, TENNIS_HALFWIDTH/2, TENNIS_WHITESTRIP),
        rect(x + TENNIS_SERVICE_AREA/2, y-TENNIS_HALFWIDTH/2, TENNIS_SERVICE_AREA/2, TENNIS_HALFWIDTH/2, TENNIS_WHITESTRIP),
        rect(x + TENNIS_SERVICE_AREA/2, y+TENNIS_HALFWIDTH/2, TENNIS_SERVICE_AREA/2, TENNIS_HALFWIDTH/2, TENNIS_WHITESTRIP),
        )

cdef float tennis_yellow(x, y):
    return filled_rect(x, y, TENNIS_HALFLEN, TENNIS_HALFWIDTH)

cdef float tennis_green(x, y):
    return filled_rect(x, y, 2*TENNIS_HALFLEN, 3*TENNIS_HALFWIDTH)

def tennis_color(x, y):
    g = tennis_white(x, y)
    if g < 0: return 0
    r = tennis_yellow(x, y)
    if r < 0: return 1
    g = tennis_green(x, y)
    if g < 0: return 2


# -------------- test ---------------

#white  = stadium_white
#yellow = stadium_running_area
#green  = stadium_green_field
#white  = pong_white
#yellow = pong_yellow
#green  = pong_green
white  = tennis_white
yellow = tennis_yellow
green  = tennis_green

def test_raster_image(int x1, int x2, int y1, int y2, int W, int H):
    cdef int x, y
    cdef float kx = float(x2-x1) / W
    cdef float ky = float(y2-y1) / H
    pic = np.zeros( shape=(3,H,W) )
    pic.fill(-1)
    for y in tqdm.tqdm(range(H)):
        ry = ky*y + y1
        for x in range(W):
            rx = kx*x + x1

            g = white(rx, ry)
            if g < 0:
                pic[:,y,x] = +1
                continue
            else:
                pic[2,y,x] = -0.5+0.25*np.sin(g)

            r = yellow(rx, ry)
            if r < 0:
                pic[0,y,x] = +0.5
                pic[1,y,x] = +0.5
                continue
            else:
                pic[0,y,x] = -0.5+0.25*np.sin(r)

            continue
            g = green(rx, ry)
            if g < 0:
                pic[1,y,x] = +1
            else:
                pic[1,y,x] = -0.5+0.25*np.sin(g)
    return pic
