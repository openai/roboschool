import os, sys, subprocess
import gym, roboschool
import roboschool.multiplayer
import numpy as np
import random

servers_to_start = 4   # set to CPU count (real cores, hyperthreading vCPUs/2)

if len(sys.argv) in [1,2]:
    # Without parameters, this script runs itself servers_to_start times, with game id and adversary policy script from agent_zoo.
    if len(sys.argv)==2:
        servers_to_start = int(sys.argv[1])
    skilled_adversaries = []
    zoo_path = os.path.join(os.path.dirname(__file__), "../agent_zoo")
    for f in os.listdir(zoo_path):
        if not f.startswith("RoboschoolPong"): continue
        skilled_adversaries.append(os.path.join(zoo_path, f))
    random.shuffle(skilled_adversaries)
    skilled_adversaries_count = len(skilled_adversaries)

    proclist = []
    for i in range(servers_to_start):
        run_myself = [sys.executable, sys.argv[0], "pong_training%02i" % i, skilled_adversaries[i % skilled_adversaries_count]]
        print("EXEC %s" % " ".join(run_myself))
        proclist.append( subprocess.Popen(run_myself) )

    # Subprocesses work here, until one of players disconnects.
    #
    # After servers fire up, start here your training.
    # Play with all started adversaries simultaneously using one shared policy, apply joined gradient to your weights.
    # (manually in different console, or write startup code here.)

    for p in proclist:
        p.wait()

elif len(sys.argv)==3:
    game_server_guid = sys.argv[1]
    adversary_script = sys.argv[2]

    game       = roboschool.gym_pong.PongSceneMultiplayer()
    gameserver = roboschool.multiplayer.SharedMemoryServer(game, game_server_guid, want_test_window=False)
    # Shared memory files and pipes are ready here

    run_adv = [sys.executable, adversary_script, game_server_guid, "0"]   # plays for player 0, connect your agent as player 1
    print("EXEC %s" % " ".join(run_adv))
    subprocess.Popen(run_adv)
    # Adversary script must support two command line parameters: game_server_guid and player_n.
    # Inside, it must call:
    #    env.unwrapped.multiplayer(env, game_server_guid, int(player_n))

    gameserver.serve_forever()

else:
    raise ValueError("run this script with zero parameters or 3 parameters")
