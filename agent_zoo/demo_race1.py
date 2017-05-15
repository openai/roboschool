import os, sys, subprocess
import numpy as np
import gym
import roboschool

if len(sys.argv)==1:
    import roboschool.multiplayer
    stadium = roboschool.scene_stadium.MultiplayerStadiumScene(gravity=9.8, timestep=0.0165/4, frame_skip=4)
    gameserver = roboschool.multiplayer.SharedMemoryServer(stadium, "race", want_test_window=True)
    # We start subprocesses between constructor and serve_forever(), because constructor creates necessary pipes to connect to
    for n in range(stadium.players_count):
        subprocess.Popen([sys.executable, sys.argv[0], "race", "%i"%n])
    gameserver.serve_forever()

else:
    from RoboschoolWalker2d_v0_2017may        import SmallReactivePolicy as PolWalker
    from RoboschoolHopper_v0_2017may          import SmallReactivePolicy as PolHopper
    from RoboschoolHalfCheetah_v0_2017may     import SmallReactivePolicy as PolHalfCheetah
    from RoboschoolHumanoid_v0_2017may        import SmallReactivePolicy as PolHumanoid1
    from RoboschoolHumanoidFlagrun_v0_2017may import SmallReactivePolicy as PolHumanoid2
    # HumanoidFlagrun is compatible with normal Humanoid in observations and actions. The walk is not as good, and
    # ability to turn is unnecessary in this race, but it can take part anyway.

    possible_participants = [
        ("RoboschoolWalker2d-v0", PolWalker),
        ("RoboschoolHopper-v0",   PolHopper),
        ("RoboschoolHalfCheetah-v0", PolHalfCheetah),
        ("RoboschoolHumanoid-v0", PolHumanoid1),
        ("RoboschoolHumanoid-v0", PolHumanoid2),
        ]
    env_id, PolicyClass = possible_participants[ np.random.randint(len(possible_participants)) ]
    env = gym.make(env_id)
    env.unwrapped.multiplayer(env, game_server_guid=sys.argv[1], player_n=int(sys.argv[2]))

    pi = PolicyClass(env.observation_space, env.action_space)

    while 1:
        obs = env.reset()
        while 1:
            a = pi.act(obs)
            obs, rew, done, info = env.step(a)
            if done: break
