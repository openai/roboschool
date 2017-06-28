import os, sys, subprocess
import numpy as np
import gym
import roboschool

env = gym.make('RoboschoolPegInsertion-v0')
while 1:
    frame = 0
    score = 0
    restart_delay = 0
    obs = env.reset()
    
    while 1:
        a = env.action_space.sample()
        # a = np.array([0]*7)
        obs, r, done, _ = env.step(a)
        score += r
        frame += 1
        still_open = env.render("rgb_array")
        # import cv2
        # cv2.imwrite('image.jpg', still_open)

        if not done: continue
        if restart_delay==0:
            print("score=%0.2f in %i frames" % (score, frame))
            restart_delay = 60*2  # 2 sec at 60 fps
        else:
            restart_delay -= 1
            if restart_delay==0: break
