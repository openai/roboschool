import os, gym, roboschool
import numpy as np
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import tensorflow as tf
config = tf.ConfigProto(
    inter_op_parallelism_threads=1,
    intra_op_parallelism_threads=1,
    device_count = { "GPU": 0 } )
sess = tf.InteractiveSession(config=config)

from RoboschoolWalker2d_v1_2017jul        import ZooPolicyTensorflow as PolWalker
from RoboschoolHopper_v1_2017jul          import ZooPolicyTensorflow as PolHopper
from RoboschoolHalfCheetah_v1_2017jul     import ZooPolicyTensorflow as PolHalfCheetah
from RoboschoolHumanoid_v1_2017jul        import ZooPolicyTensorflow as PolHumanoid1
from RoboschoolHumanoidFlagrun_v1_2017jul import ZooPolicyTensorflow as PolHumanoid2
# HumanoidFlagrun is compatible with normal Humanoid in observations and actions.

possible_participants = [
    ("RoboschoolHopper-v1",   PolHopper),
    ("RoboschoolWalker2d-v1", PolWalker),
    ("RoboschoolHalfCheetah-v1", PolHalfCheetah),
    ("RoboschoolHumanoid-v1", PolHumanoid1),
    ("RoboschoolHumanoid-v1", PolHumanoid2),
    ]

stadium = roboschool.scene_stadium.MultiplayerStadiumScene(gravity=9.8, timestep=0.0165/4, frame_skip=4)

# This example shows inner workings of multiplayer scene, how you can run
# several robots in one process.

participants = []
for lane in range(3):
    env_id, PolicyClass = possible_participants[ np.random.randint(len(possible_participants)) ]
    env = gym.make(env_id)
    env.unwrapped.scene = stadium   # if you set scene before first reset(), it will be used.
    env.unwrapped.player_n = lane   # mutliplayer scenes will also use player_n
    pi = PolicyClass("mymodel%i" % lane, env.observation_space, env.action_space)
    participants.append( (env, pi) )

episode_n = 0
video = False
while 1:
    stadium.episode_restart()
    episode_n += 1

    multi_state = [env.reset() for env, _ in participants]
    frame = 0
    restart_delay = 0
    if video: video_recorder = gym.monitoring.video_recorder.VideoRecorder(env=participants[0][0], base_path=("/tmp/demo_race_episode%i" % episode_n), enabled=True)
    while 1:
        still_open = stadium.test_window()
        multi_action = [pi.act(s, None) for s, (env, pi) in zip(multi_state, participants)]

        for a, (env, pi) in zip(multi_action, participants):
            env.unwrapped.apply_action(a)  # action sent in apply_action() must be the same that sent into step(), 
        # some wrappers will not work

        stadium.global_step()

        state_reward_done_info = [env.step(a) for a, (env, pi) in zip(multi_action, participants)]
        multi_state = [x[0] for x in state_reward_done_info]
        multi_done  = [x[2] for x in state_reward_done_info]

        if video: video_recorder.capture_frame()

        if sum(multi_done)==len(multi_done):
            break

        frame += 1
        stadium.cpp_world.test_window_score("%04i" % frame)
        if not still_open: break
        if frame==1000: break
    if video: video_recorder.close()
    if not still_open: break

