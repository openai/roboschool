import roboschool, gym, gym.spaces, gym.utils, gym.utils.seeding
import numpy as np
import sys

#
# Run this file to test environments using manual control:
#
# python test_manual.py RoboschoolHopper-v0
#

class TestKeyboardControl:
    def __init__(self):
        self.keys = {}
        self.control = np.zeros(9)
        self.human_pause = False
        self.human_done = False
    def key(self, event_type, key, modifiers):
        self.keys[key] = +1 if event_type==6 else 0
        #print ("event_type", event_type, "key", key, "modifiers", modifiers)
        self.control[0] = self.keys.get(0x1000014, 0) - self.keys.get(0x1000012, 0)
        self.control[1] = self.keys.get(0x1000013, 0) - self.keys.get(0x1000015, 0)
        self.control[2] = self.keys.get(ord('A'), 0)  - self.keys.get(ord('Z'), 0)
        self.control[3] = self.keys.get(ord('S'), 0)  - self.keys.get(ord('X'), 0)
        self.control[4] = self.keys.get(ord('D'), 0)  - self.keys.get(ord('C'), 0)
        self.control[5] = self.keys.get(ord('F'), 0)  - self.keys.get(ord('V'), 0)
        self.control[6] = self.keys.get(ord('G'), 0)  - self.keys.get(ord('B'), 0)
        self.control[7] = self.keys.get(ord('H'), 0)  - self.keys.get(ord('N'), 0)
        self.control[8] = self.keys.get(ord('J'), 0)  - self.keys.get(ord('M'), 0)
        if event_type==6 and key==32:         # press Space to pause
            self.human_pause = 1 - self.human_pause
        if event_type==6 and key==0x1000004:  # press Enter to restart
            self.human_done = True

usage = """
This is manual test. Usage:
%s <env_id>

Keyboard shortcuts:
 * F1 toggle slow motion
 * F2 toggle captions
 * F3 toggle HUD: observations, actions, reward
 * ENTER to restart episode (works only in this test)
 * SPACE to pause (works only in this test)
 * Up/down, left/right, a/z, s/x, d/c, f/v, g/b, h/n, j/m to control robot (works only in this test)
"""

def test(env_id):
    print(usage % sys.argv[0])

    env = gym.make(env_id)
    ctrl = TestKeyboardControl()
    env.reset()  # This creates default single player scene
    env.unwrapped.scene.cpp_world.set_key_callback(ctrl.key)
    if "camera" in env.unwrapped.__dict__:
        env.unwrapped.camera.set_key_callback(ctrl.key)

    a = np.zeros(env.action_space.shape)
    copy_n = min(len(a), len(ctrl.control))
    ctrl.human_pause = False

    while 1:
        ctrl.human_done  = False
        sn = env.reset()
        frame = 0
        reward = 0.0
        episode_over = False
        while 1:
            s = sn
            a[:copy_n] = ctrl.control[:copy_n]
            sn, rplus, done, info = env.step(a)
            reward += rplus
            #env.render("rgb_array")
            episode_over |= done
            still_visible = True
            while still_visible:
                still_visible = env.render("human")
                #env.unwrapped.camera.test_window()
                if not ctrl.human_pause: break
            if ctrl.human_done: break
            if not still_visible: break
            frame += 1
        if not still_visible: break

if __name__=="__main__":
    env_id = "RoboschoolHumanoid-v0" if len(sys.argv) <= 1 else sys.argv[1]
    test(env_id)
