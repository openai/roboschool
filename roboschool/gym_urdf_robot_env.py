import gym, gym.spaces, gym.utils, gym.utils.seeding
from roboschool.scene_abstract import cpp_household
import numpy as np
import os

class RoboschoolUrdfEnv(gym.Env):
    """
    Base class for URDF robot actor in a Scene.
    """

    metadata = {
        'render.modes': ['human', 'rgb_array'],
        'video.frames_per_second': 60
        }

    VIDEO_W = 600  # for video showing the robot, not for camera ON the robot
    VIDEO_H = 400

    def __init__(self, model_urdf, robot_name, action_dim, obs_dim, fixed_base, self_collision):
        self.scene = None

        high = np.ones([action_dim])
        self.action_space = gym.spaces.Box(-high, high)
        high = np.inf*np.ones([obs_dim])
        self.observation_space = gym.spaces.Box(-high, high)
        self.seed()

        self.model_urdf = model_urdf
        self.fixed_base = fixed_base
        self.self_collision = self_collision
        self.robot_name = robot_name

    def seed(self, seed=None):
        self.np_random, seed = gym.utils.seeding.np_random(seed)
        return [seed]

    def reset(self):
        if self.scene is None:
            self.scene = self.create_single_player_scene()
        if not self.scene.multiplayer:
            self.scene.episode_restart()

        pose = cpp_household.Pose()
        #import time
        #t1 = time.time()
        self.urdf = self.scene.cpp_world.load_urdf(
            os.path.join(os.path.dirname(__file__), "models_robot", self.model_urdf),
            pose,
            self.fixed_base,
            self.self_collision)
        #t2 = time.time()
        #print("URDF load %0.2fms" % ((t2-t1)*1000))

        self.ordered_joints = []
        self.jdict = {}
        self.parts = {}
        self.frame = 0
        self.done = 0
        self.reward = 0
        dump = 0
        r = self.urdf
        self.cpp_robot = r
        if dump: print("ROBOT '%s'" % r.root_part.name)
        if r.root_part.name==self.robot_name:
            self.robot_body = r.root_part
        for part in r.parts:
            if dump: print("\tPART '%s'" % part.name)
            self.parts[part.name] = part
            if part.name==self.robot_name:
                self.robot_body = part
        for j in r.joints:
            if dump: print("\tALL JOINTS '%s' limits = %+0.2f..%+0.2f effort=%0.3f speed=%0.3f" % ((j.name,) + j.limits()) )
            if j.name[:6]=="ignore":
                j.set_motor_torque(0)
                continue
            j.power_coef, j.max_velocity = j.limits()[2:4]
            self.ordered_joints.append(j)
            self.jdict[j.name] = j
        #print("ordered_joints", len(self.ordered_joints))
        self.robot_specific_reset()
        self.cpp_robot.query_position()
        s = self.calc_state()    # optimization: calc_state() can calculate something in self.* for calc_potential() to use
        self.potential = self.calc_potential()
        self.camera = self.scene.cpp_world.new_camera_free_float(self.VIDEO_W, self.VIDEO_H, "video_camera")
        return s

    def render(self, mode='human', close=False):
        if close:
            return
        if mode=="human":
            self.scene.human_render_detected = True
            return self.scene.cpp_world.test_window()
        elif mode=="rgb_array":
            self.camera_adjust()
            rgb, _, _, _, _ = self.camera.render(False, False, False) # render_depth, render_labeling, print_timing)
            rendered_rgb = np.fromstring(rgb, dtype=np.uint8).reshape( (self.VIDEO_H,self.VIDEO_W,3) )
            return rendered_rgb
        else:
            assert(0)

    def calc_potential(self):
        return 0

    def HUD(self, s, a, done):
        active = self.scene.actor_is_active(self)
        if active and self.done<=2:
            self.scene.cpp_world.test_window_history_advance()
            self.scene.cpp_world.test_window_observations(s.tolist())
            self.scene.cpp_world.test_window_actions(a.tolist())
            self.scene.cpp_world.test_window_rewards(self.rewards)
        if self.done<=1: # Only post score on first time done flag is seen, keep this score if user continues to use env
            s = "%04i %07.1f %s" % (self.frame, self.reward, "DONE" if self.done else "")
            if active:
                self.scene.cpp_world.test_window_score(s)
            #self.camera.test_window_score(s)  # will appear on video ("rgb_array"), but not on cameras istalled on the robot (because that would be different camera)

