from roboschool.scene_abstract import Scene, cpp_household
from roboschool.multiplayer import SharedMemoryClientEnv
import gym, gym.spaces, gym.utils, gym.utils.seeding
import numpy as np
import os, sys

class PongScene(Scene):
    multiplayer = False
    players_count = 1
    VIDEO_W = 600
    VIDEO_H = 400

    def __init__(self):
        Scene.__init__(self, gravity=9.8, timestep=0.0165/4, frame_skip=4)
        self.score_left = 0
        self.score_right = 0
        self.ball_x = 0

    def actor_introduce(self, robot):
        i = robot.player_n - 1

    def episode_restart(self):
        Scene.episode_restart(self)
        if self.score_right + self.score_left > 0:
            sys.stdout.write("%i:%i " % (self.score_left, self.score_right))
            sys.stdout.flush()
        self.mjcf = self.cpp_world.load_mjcf(os.path.join(os.path.dirname(__file__), "models_robot/roboschool_pong.xml"))
        dump = 0
        for r in self.mjcf:
            if dump: print("ROBOT '%s'" % r.root_part.name)
            for part in r.parts:
                if dump: print("\tPART '%s'" % part.name)
                #if part.name==self.robot_name:
            for j in r.joints:
                if j.name=="p0x": self.p0x = j
                if j.name=="p0y": self.p0y = j
                if j.name=="p1x": self.p1x = j
                if j.name=="p1y": self.p1y = j
                if j.name=="ballx": self.ballx = j
                if j.name=="bally": self.bally = j
        self.ballx.set_motor_torque(0.0)
        self.bally.set_motor_torque(0.0)
        for r in self.mjcf:
            r.query_position()
        fpose = cpp_household.Pose()
        fpose.set_xyz(0,0,-0.04)
        self.field = self.cpp_world.load_thingy(
            os.path.join(os.path.dirname(__file__), "models_outdoor/stadium/pong1.obj"),
            fpose, 1.0, 0, 0xFFFFFF, True)
        self.camera = self.cpp_world.new_camera_free_float(self.VIDEO_W, self.VIDEO_H, "video_camera")
        self.camera_itertia = 0
        self.frame = 0
        self.jstate_for_frame = -1
        self.score_left = 0
        self.score_right = 0
        self.restart_from_center(self.players_count==1 or self.np_random.randint(2)==0)

    def restart_from_center(self, leftwards):
        self.ballx.reset_current_position(0, self.np_random.uniform(low=2.0, high=2.5) * (-1 if leftwards else +1))
        self.bally.reset_current_position(self.np_random.uniform(low=-0.9, high=+0.9), self.np_random.uniform(low=-2, high=+2))
        self.timeout = self.TIMEOUT
        self.timeout_dir = (-1 if leftwards else +1)
        self.bounce_n = 0
        self.trainer_x = self.np_random.uniform(low=-0.9, high=+0.9)
        self.trainer_y = self.np_random.uniform(low=-0.9, high=+0.9)

    def global_step(self):
        self.frame += 1

        if not self.multiplayer:
            # Trainer
            self.p1x.set_servo_target( self.trainer_x, 0.02, 0.02, 4 )
            self.p1y.set_servo_target( self.trainer_y, 0.02, 0.02, 4 )

        Scene.global_step(self)

        self.ball_x, ball_vx = self.ballx.current_position()
        self.ball_y, ball_vy = self.bally.current_position()

        if np.abs(self.ball_y) > 1.0 and self.ball_y*ball_vy > 0:
            self.bally.reset_current_position(self.ball_y, -ball_vy)

        if ball_vx*self.timeout_dir < 0:
            if self.timeout_dir < 0:
                self.score_left  += 0.01*np.abs(ball_vx)   # hint for early learning: hit the ball!
            else:
                self.score_right += 0.01*np.abs(ball_vx)
            self.timeout_dir *= -1
            self.timeout = 150
            self.bounce_n += 1
        else:
            self.timeout -= 1

        if np.abs(self.ball_x) > 1.65 or self.timeout==0:
            if self.timeout==0:
                self.restart_from_center(self.players_count==1 or ball_vx<0)  # send ball in same dir on timeout
            elif ball_vx>0:
                if self.bounce_n > 0:
                    self.score_left  += 1
                self.score_right -= 1
                self.restart_from_center(self.players_count==1 or ball_vx>0)  # winning streak, let it hit more
            else:
                if self.bounce_n > 0:
                    self.score_right += 1.0
                self.score_left -= 1
                self.restart_from_center(self.players_count==1 or ball_vx>0)

    def global_state(self):
        if self.frame==self.jstate_for_frame:
            return self.jstate
        self.jstate_for_frame = self.frame
        j = np.array(
            [j.current_relative_position() for j in [self.p0x, self.p0y, self.p1x, self.p1y, self.ballx, self.bally]]
            ).flatten()
        self.jstate = np.concatenate([j, [(self.timeout - self.TIMEOUT) / self.TIMEOUT]])
        #print(self.jstate)
        return self.jstate

    TIMEOUT = 150

    def HUD(self, a, s):
        self.cpp_world.test_window_history_advance()
        self.cpp_world.test_window_observations(s.tolist())
        self.cpp_world.test_window_actions(a.tolist())
        s = "%04i TIMEOUT%3i %0.2f:%0.2f" % (
            self.frame, self.timeout, self.score_left, self.score_right
            )
        self.cpp_world.test_window_score(s)
        self.camera.test_window_score(s)

    def camera_adjust(self):
        self.camera_itertia *= 0.9
        self.camera_itertia += 0.1 * 0.05*self.ball_x
        self.camera.move_and_look_at(0,-1.0,1.5, self.camera_itertia,-0.1  ,0)

class PongSceneMultiplayer(PongScene):
    multiplayer = True
    players_count = 2


# -- Environment itself here --

class RoboschoolPong(gym.Env, SharedMemoryClientEnv):
    '''
    Continuous control version of Atari Pong.
    Agent controls x and y velocity of the left paddle and gets points for opponent missing the ball. 
    Observations are positions and velocities of both paddles, and position and velocity of the ball. 
    '''
    metadata = {
        'render.modes': ['human', 'rgb_array'],
        'video.frames_per_second' : 60
    }

    VIDEO_W = 600
    VIDEO_H = 400

    def __init__(self):
        self.scene = None
        action_dim = 2
        obs_dim = 13
        high = np.ones([action_dim])
        self.action_space = gym.spaces.Box(-high, high)
        high = np.inf*np.ones([obs_dim])
        self.observation_space = gym.spaces.Box(-high, high)
        self.seed()

    def create_single_player_scene(self):
        self.player_n = 0
        s = PongScene()
        s.np_random = self.np_random
        return s

    def seed(self, seed=None):
        self.np_random, seed = gym.utils.seeding.np_random(seed)
        return [seed]

    def reset(self):
        if self.scene is None:
            self.scene = self.create_single_player_scene()
        if not self.scene.multiplayer:
            self.scene.episode_restart()
        s = self.calc_state()
        self.score_reported = 0
        return s

    def calc_state(self):
        j = self.scene.global_state()
        if self.player_n==1:
            #                    [  0,1,  2,3,   4, 5, 6,7,  8,9,10,11,12]
            #                    [p0x,v,p0y,v, p1x,v,p1y,v, bx,v,by,v, T]
            signflip =  np.array([ -1,-1, 1,1,  -1,-1, 1,1, -1,-1,1,1, 1])
            reorder  =  np.array([  4, 5, 6,7,   0, 1, 2,3,  8,9,10,11,12])
            j = (j*signflip)[reorder]
        return j

    def apply_action(self, a):
        assert( np.isfinite(a).all() )
        a = np.clip(a, -1, +1)
        if self.player_n==0:
            self.scene.p0x.set_target_speed(  3*float(a[0]), 0.05, 7 )
            self.scene.p0y.set_target_speed(  3*float(a[1]), 0.05, 7 )
        else:
            self.scene.p1x.set_target_speed( -3*float(a[0]), 0.05, 7 )
            self.scene.p1y.set_target_speed(  3*float(a[1]), 0.05, 7 )

    def step(self, a):
        if not self.scene.multiplayer:
            self.apply_action(a)
            self.scene.global_step()

        state = self.calc_state()
        if self.player_n==0:
            self.scene.HUD(a, state)

        if self.player_n==0:
            new_score = self.scene.score_left
        else:
            new_score = self.scene.score_right
        self.rewards = [new_score - self.score_reported]
        self.score_reported = new_score

        return state, sum(self.rewards), False, {}

    def render(self, mode='human', close=False):
        if close:
            return
        if mode=="human":
            return self.scene.cpp_world.test_window()
        elif mode=="rgb_array":
            self.scene.camera_adjust()
            rgb, _, _, _, _ = self.scene.camera.render(False, False, False) # render_depth, render_labeling, print_timing)
            rendered_rgb = np.fromstring(rgb, dtype=np.uint8).reshape( (self.VIDEO_H,self.VIDEO_W,3) )
            return rendered_rgb
        else:
            assert(0)
