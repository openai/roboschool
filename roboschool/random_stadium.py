import numpy as np
import tqdm

import scipy
import scipy.spatial
import PIL                    # pip3 install Pillow
import PIL.Image as Image
import PIL.ImageDraw as ImageDraw
import PIL.ImageFont as ImageFont
import scipy
import scipy.misc
import scipy.misc.pilutil
import pyximport              # pip3 install cython
pyximport.install()
import random_stadium_kernel

class Obj:
	def __init__(self, fn):
		self.ind_v  = 0
		self.ind_vt = 0
		self.ind_vn = 0
		self.fn = fn
		self.out = open(fn + ".tmp", "w")
		self.out.write("mtllib stadium.mtl\n")
	def __del__(self):
		self.out.close()
		import shutil
		shutil.move(self.fn + ".tmp", self.fn)
	def push_v(self, v):
		self.out.write("v %f %f %f\n" % (v[0],v[1],v[2]))
		self.ind_v += 1
		return self.ind_v
	def push_vt(self, vt):
		self.out.write("vt %f %f\n" % (vt[0],vt[1]))
		self.ind_vt += 1
		return self.ind_vt
	def push_vn(self, vn):
		vn /= np.linalg.norm(vn)
		self.out.write("vn %f %f %f\n" % (vn[0],vn[1],vn[2]))
		self.ind_vn += 1
		return self.ind_vn

class Triangulation:
    pass

def find_color_change(x1,y1,color1, x2,y2,color2):
    EPS = 0.001
    while 1:
        x = 0.5*(x1+x2)
        y = 0.5*(y1+y2)
        c = color(x, y)
        if c==color1:
            x1,y1 = x,y
        else:
            x2,y2 = x,y
        if abs(x1-x2) + abs(y1-y2) < EPS: break
    return [x,y]

def color_change_grid_search(tr, x1, x2, y1, y2, W, H):
    tr.p = []
    tr.material_palette = ["stadium_white", "stadium_dirt", "stadium_grass"]

    kx = float(x2-x1) / W
    ky = float(y2-y1) / H

    for y in tqdm.tqdm(range(H)):
        ry = ky*y + y1
        rx_prev,c_prev = None,-1
        for x in np.arange(0,W,0.1):
            rx = kx*x + x1
            c = color(rx,ry)
            if c != c_prev and rx_prev != None:
                tr.p.append( find_color_change(rx_prev,ry,c_prev, rx,ry,c) )
            c_prev = c
            rx_prev = rx

    for x in tqdm.tqdm(range(W)):
        rx = kx*x + x1
        ry_prev,c_prev = None,-1
        for y in np.arange(0,H,0.1):
            ry = ky*y + y1
            c = color(rx,ry)
            if c != c_prev and ry_prev != None:
                tr.p.append( find_color_change(rx,ry_prev,c_prev, rx,ry,c) )
            c_prev = c
            ry_prev = ry

stadium_extents = (-50,+50, -40,+40)
pong_extents    = (-4.0,+4.0, -3,+3)
tennis_extents  = (-20,+20, -20,+20)

def test_triangulation(fn, extents):
    tr = Triangulation()

    color_change_grid_search(tr, *extents, 250,250*4//5)
    grid_result = tr.p.copy()

    retry = 10
    nothing_added = False
    while 1:
        tr.p_array = np.array( tr.p )
        print("points:", tr.p_array.shape)
        tr.dela = scipy.spatial.Delaunay(tr.p_array)
        if retry==0: break
        if nothing_added: break
        retry -= 1
        nothing_added = True
        for r in tr.dela.simplices:
            centerx = 1.0/3*( tr.p[r[0]][0] + tr.p[r[1]][0] + tr.p[r[2]][0] )
            centery = 1.0/3*( tr.p[r[0]][1] + tr.p[r[1]][1] + tr.p[r[2]][1] )
            centercolor = color(centerx, centery)
            for p in [0,1,2]:
                worst = 0.02
                fix_x, fix_y = None, None
                for i in [5,0,1,2,3,4,6,7,8,9]:  # start from middle
                    middle = i==5
                    p1share = (i+0.5)/10   # 0.05 0.15 .. 0.95
                    px = p1share*tr.p[r[p]][0] + (1-p1share)*tr.p[r[p-1]][0]
                    py = p1share*tr.p[r[p]][1] + (1-p1share)*tr.p[r[p-1]][1]
                    pcolor = color(px, py)
                    if pcolor==centercolor: continue
                    nx, ny = find_color_change(px,py,pcolor, centerx,centery,centercolor)
                    dist = np.sqrt( np.square(px-nx) + np.square(py-ny) )
                    if worst < dist:
                        worst = dist
                        fix_x, fix_y = nx, ny
                    else:
                        if middle: break  # optimization
                if fix_x != None:
                    tr.p.append( [fix_x, fix_y] )
                    nothing_added = False

    obj = Obj(fn)
    obj.v_idx = []
    for v in tr.p:
        obj.v_idx.append( obj.push_v( [v[0],v[1],0] ) )
        obj.push_vn( [0,0,1] )
        obj.push_vt( [v[0]*0.2+0.25, v[1]*0.2+0.25] )
    for material_index,material in enumerate(tr.material_palette):
        obj.out.write("\n\nusemtl %s\n" % material)
        obj.out.write("o %s\n" % material)
        for r in tr.dela.simplices:
            rx = 1.0/3*( tr.p[r[0]][0] + tr.p[r[1]][0] + tr.p[r[2]][0] )
            ry = 1.0/3*( tr.p[r[0]][1] + tr.p[r[1]][1] + tr.p[r[2]][1] )
            c = color(rx, ry)
            if c!=material_index: continue
            obj.out.write("f %i/%i/%i %i/%i/%i %i/%i/%i\n" % tuple( [obj.v_idx[r[0]]]*3 + [obj.v_idx[r[1]]]*3 + [obj.v_idx[r[2]]]*3 ))
    obj.out.close()

if __name__=='__main__':
    pic = random_stadium_kernel.test_raster_image(*pong_extents, 3*320,3*320//5*4)
    image = scipy.misc.toimage(pic, cmin=-1.0, cmax=1.0)
    image.save("roboschool/models_outdoor/stadium/test.png")

    #color = random_stadium_kernel.stadium_color
    #test_triangulation("roboschool/models_outdoor/stadium/stadium1.obj", stadium_extents)
    #color = random_stadium_kernel.pong_color
    #test_triangulation("roboschool/models_outdoor/stadium/pong1.obj", pong_extents)
    color = random_stadium_kernel.tennis_color
    test_triangulation("roboschool/models_outdoor/stadium/tennis1.obj", tennis_extents)
