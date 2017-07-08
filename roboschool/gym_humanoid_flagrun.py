from roboschool.gym_forward_walkers import RoboschoolHumanoid
from roboschool.scene_abstract import cpp_household
import numpy as np
import os

class RoboschoolHumanoidFlagrun(RoboschoolHumanoid):
    random_yaw = False

    def create_single_player_scene(self):
        s = RoboschoolHumanoid.create_single_player_scene(self)
        s.zero_at_running_strip_start_line = False
        self.electricity_cost /= 2   # For more athletic running instead of walking
        return s

    def robot_specific_reset(self):
        RoboschoolHumanoid.robot_specific_reset(self)
        self.flag_reposition(first=True)

    def flag_reposition(self, first):
        if first and self.np_random.randint(2)==0:
            # To encourage optimal straight run, often just run forward without turning. This also helps with gait to be symmetric.
            self.walk_target_x = self.np_random.uniform(low=self.scene.stadium_halflen/2,   high=+self.scene.stadium_halflen)
            self.walk_target_y = 0
        else:
            self.walk_target_x = self.np_random.uniform(low=-self.scene.stadium_halflen,   high=+self.scene.stadium_halflen)
            self.walk_target_y = self.np_random.uniform(low=-self.scene.stadium_halfwidth, high=+self.scene.stadium_halfwidth)
            more_compact = 0.5  # set to 1.0 whole football field
            self.walk_target_x *= more_compact
            self.walk_target_y *= more_compact

        self.flag = None
        self.flag = self.scene.cpp_world.debug_sphere(self.walk_target_x, self.walk_target_y, 0.2, 0.2, 0xFF8080)
        self.flag_timeout = 150

    def calc_state(self):
        self.flag_timeout -= 1
        state = RoboschoolHumanoid.calc_state(self)
        if self.walk_target_dist < 1 or self.flag_timeout <= 0:
            self.flag_reposition(first=False)
            state = RoboschoolHumanoid.calc_state(self)  # caclulate state again, against new flag pos
            self.potential = self.calc_potential()       # avoid reward jump
        return state

class RoboschoolHumanoidFlagrunHarder(RoboschoolHumanoidFlagrun):
    random_yaw = True
    random_lean = True  # can fall on start

    def robot_specific_reset(self):
        RoboschoolHumanoidFlagrun.robot_specific_reset(self)
        cpose = cpp_household.Pose()
        cpose.set_rpy(0, 0, 0)
        cpose.set_xyz(-1.5, 0, 0.05)
        self.aggressive_cube = self.scene.cpp_world.load_urdf(os.path.join(os.path.dirname(__file__), "models_household/cube.urdf"), cpose, False)
        self.on_ground_frame_counter = 0
        self.crawl_start_potential = None
        self.crawl_ignored_potential = 0.0
        self.initial_z = 0.8
        self.walking_normally = 0
        self.pelvis = self.parts["pelvis"]

    def alive_bonus(self, z, pitch):
        if self.frame%30==0 and self.frame>100 and self.on_ground_frame_counter==0:
            target_xyz  = np.array(self.body_xyz)
            robot_speed = np.array(self.robot_body.speed())
            angle       = self.np_random.uniform(low=-3.14, high=3.14)
            from_dist   = 4.0
            attack_speed   = self.np_random.uniform(low=20.0, high=30.0)  # speed 20..30 (* mass in cube.urdf = impulse)
            time_to_travel = from_dist / attack_speed
            target_xyz += robot_speed*time_to_travel  # predict future position at the moment the cube hits the robot
            cpose = cpp_household.Pose()
            cpose.set_xyz(
                target_xyz[0] + from_dist*np.cos(angle),
                target_xyz[1] + from_dist*np.sin(angle),
                target_xyz[2] + 1.0)
            attack_speed_vector  = target_xyz - np.array(cpose.xyz())
            attack_speed_vector *= attack_speed / np.linalg.norm(attack_speed_vector)
            attack_speed_vector += self.np_random.uniform(low=-1.0, high=+1.0, size=(3,))
            self.aggressive_cube.set_pose_and_speed(cpose, *attack_speed_vector)
        if z < 0.8:
            self.on_ground_frame_counter += 1
            self.electricity_cost = RoboschoolHumanoid.electricity_cost / 5.0   # Don't care about electricity, just stand up!
        elif self.on_ground_frame_counter > 0:
            self.on_ground_frame_counter -= 1
            self.electricity_cost = RoboschoolHumanoid.electricity_cost / 2.0    # Same as in Flagrun
        else:
            self.walking_normally += 1
            self.electricity_cost = RoboschoolHumanoid.electricity_cost / 2.0
        # End episode if the robot can't get up in 170 frames, to save computation and decorrelate observations.
        return self.potential_leak() if self.on_ground_frame_counter<170 else -1

    def potential_leak(self):
        z1 = self.body_xyz[2]          # 0.00 .. 0.8 .. 1.05 normal walk, 1.2 when jumping
        z2 = self.pelvis.pose().xyz()[2]
        z = 0.7*z1 + 0.3*z2            # This is tuned to be neutral to leaning body forward or backward (and staying like this at local maximum)
        #self.debug1 = self.scene.cpp_world.debug_sphere( self.body_xyz[0], self.body_xyz[1], z, 0.150, 0xF0FFF0 )
        z  = np.clip(z, 0, 0.7)
        return 0.5 + 1.5*(z/0.7)       # 1.0 .. 2.0

    def calc_potential(self):
        # We see alive bonus here as a leak from potential field. Value V(s) of a given state equals
        # potential, if it is topped up with gamma*potential every frame. Gamma is assumed 0.99.
        #
        # 2.0 alive bonus if z>0.8, potential is 200, leak gamma=0.99, (1-0.99)*200==2.0
        # 1.0 alive bonus on the ground z==0, potential is 100, leak (1-0.99)*100==1.0
        #
        # Why robot whould stand up: to receive 100 points in potential field difference.
        flag_running_progress = RoboschoolHumanoid.calc_potential(self)

        # This disables crawl.
        if self.body_xyz[2] < 0.8:
            if self.crawl_start_potential is None:
                self.crawl_start_potential = flag_running_progress - self.crawl_ignored_potential
                #print("CRAWL START %+0.1f %+0.1f" % (self.crawl_start_potential, flag_running_progress))
                if False: # self.on_ground_frame_counter==1:
                    # Idea there is to punish robot that falls on the ground to turn.
                    # This helps a bit, but makes robot think it's better to always stay below
                    # qualifying as walking normally (for some seeds). Try this for yourself, interesting.
                    if self.walking_normally>150:
                        #print("NO NO NO YOU WAS RUNNING FINE, DON'T FALL")
                        self.crawl_start_potential -= 100.0
                    self.walking_normally = 0
            self.crawl_ignored_potential = flag_running_progress - self.crawl_start_potential
            flag_running_progress  = self.crawl_start_potential
        else:
            #print("CRAWL STOP %+0.1f %+0.1f" % (self.crawl_ignored_potential, flag_running_progress))
            flag_running_progress -= self.crawl_ignored_potential
            self.crawl_start_potential = None

        return flag_running_progress + self.potential_leak()*100
