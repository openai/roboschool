import gym, roboschool
import numpy as np
from roboschool.gym_mujoco_xml_env import RoboschoolMujocoXmlEnv
from roboschool.scene_abstract import SingleRobotEmptyScene
import os, sys

class RoboschoolPegInsertion(RoboschoolMujocoXmlEnv):
    def __init__(self):
        RoboschoolMujocoXmlEnv.__init__(self, 'peg_insertion_arm.xml', 'arm', action_dim=7, obs_dim=110)

    def create_single_player_scene(self):
        return SingleRobotEmptyScene(gravity=9.8, timestep=0.01, frame_skip=1)

    def robot_specific_reset(self):
        for name, joint in self.jdict.items():
            joint.reset_current_position(np.random.uniform(low=-.1, high=.1),0)
            joint.set_motor_torque(0)

    def calc_state(self):
        part_coords = []
        for name, part in self.parts.items():
            # NOTE: received errors trying to do this the straightforward way
            def add_to_parts_list(x):
                part_coords.append([x[0], x[1], x[2]])
            add_to_parts_list(part.pose().xyz())
        joint_pos_s = []
        for name, joint in self.jdict.items():
            joint_pos_s.append(joint.current_position())
        result = np.append(np.array(part_coords), np.array(joint_pos_s))
        return result

    def _step(self, action):
        self.apply_action(action)
        self.scene.global_step()

        state = self.calc_state()

        consts = dict(w_u=1e-6, w_p=1, alpha=0)
        p_x_t   = self.parts['ball'].pose().xyz()
        p_star  = np.array([0, 0.3, -0.47])
        diff = p_x_t-p_star

        loss = .5 * consts['w_u'] * np.linalg.norm(action)**2
        def l12(z):
            return .5*np.linalg.norm(z)**2+np.sqrt(consts['alpha'] + z**2)

        loss += consts['w_p']*l12(diff)
        self.rewards = [-np.sum(loss)]
        done = True if np.linalg.norm(diff) < 0.05 else False

        self.HUD(state, action, done)
        return state, self.rewards[0], done, {}

    def apply_action(self, a):
        assert(np.isfinite(a).all())
        idx = 0
        for name, joint in self.jdict.items():
            action = 100*float(np.clip(a[idx], -1, +1))
            joint.set_motor_torque(action)
            idx += 1

    def camera_adjust(self):
        # self.camera.move_and_look_at(0,1.2,1.2, 0,0,0.5)
        self.camera.move_and_look_at(0, 0, -0.188, 0.0, 0.3, -0.55)
