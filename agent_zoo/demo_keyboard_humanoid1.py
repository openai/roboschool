import gym, roboschool, sys, os
import numpy as np
import pyglet, pyglet.window as pw, pyglet.window.key as pwk
from pyglet import gl
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import tensorflow as tf

#
# This opens a test window (not chase camera), allows to control humanoid using keyboard.
#

def demo_run():
    env = gym.make("RoboschoolHumanoidFlagrun-v1")

    config = tf.ConfigProto(
        inter_op_parallelism_threads=1,
        intra_op_parallelism_threads=1,
        device_count = { "GPU": 0 } )
    sess = tf.InteractiveSession(config=config)

    from RoboschoolHumanoidFlagrunHarder_v1_2017jul import ZooPolicyTensorflow
    pi = ZooPolicyTensorflow("humanoid1", env.observation_space, env.action_space)

    class TestKeyboardControl:
        def __init__(self):
            self.keys = {}
            self.control = np.zeros(2)
        def key(self, event_type, key, modifiers):
            self.keys[key] = +1 if event_type==6 else 0
            #print ("event_type", event_type, "key", key, "modifiers", modifiers)
            self.control[0] = self.keys.get(0x1000014, 0) - self.keys.get(0x1000012, 0)
            self.control[1] = self.keys.get(0x1000013, 0) - self.keys.get(0x1000015, 0)

    obs = env.reset()
    eu = env.unwrapped

    still_open = env.render("human") # This creates window to set callbacks on
    ctrl = TestKeyboardControl()
    eu.scene.cpp_world.set_key_callback(ctrl.key)

    while 1:
        a = pi.act(obs, env)

        if (ctrl.control != 0).any():
            eu.walk_target_x = eu.body_xyz[0] + 2.0*ctrl.control[0]
            eu.walk_target_y = eu.body_xyz[1] + 2.0*ctrl.control[1]
            eu.flag = eu.scene.cpp_world.debug_sphere(eu.walk_target_x, eu.walk_target_y, 0.2, 0.2, 0xFF8080)
            eu.flag_timeout = 100500

        obs, r, done, _ = env.step(a)
        still_open = env.render("human")
        if still_open==False:
            return

if __name__=="__main__":
    demo_run()
