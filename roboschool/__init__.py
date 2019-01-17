from gym.envs.registration import register
#from gym.scoreboard.registration import add_task, add_group

import os
import os.path as osp
import subprocess

os.environ['QT_PLUGIN_PATH'] = osp.join(osp.dirname(osp.abspath(__file__)), '.qt_plugins') + ':' + \
                               os.environ.get('QT_PLUGIN_PATH','')

register(
    id='RoboschoolInvertedPendulum-v1',
    entry_point='roboschool:RoboschoolInvertedPendulum',
    max_episode_steps=1000,
    reward_threshold=950.0,
    tags={ "pg_complexity": 1*1000000 },
    )
register(
    id='RoboschoolInvertedPendulumSwingup-v1',
    entry_point='roboschool:RoboschoolInvertedPendulumSwingup',
    max_episode_steps=1000,
    reward_threshold=800.0,
    tags={ "pg_complexity": 1*1000000 },
    )
register(
    id='RoboschoolInvertedDoublePendulum-v1',
    entry_point='roboschool:RoboschoolInvertedDoublePendulum',
    max_episode_steps=1000,
    reward_threshold=9100.0,
    tags={ "pg_complexity": 1*1000000 },
    )

register(
    id='RoboschoolReacher-v1',
    entry_point='roboschool:RoboschoolReacher',
    max_episode_steps=150,
    reward_threshold=18.0,
    tags={ "pg_complexity": 1*1000000 },
    )

register(
    id='RoboschoolHopper-v1',
    entry_point='roboschool:RoboschoolHopper',
    max_episode_steps=1000,
    reward_threshold=2500.0,
    tags={ "pg_complexity": 8*1000000 },
    )
register(
    id='RoboschoolWalker2d-v1',
    entry_point='roboschool:RoboschoolWalker2d',
    max_episode_steps=1000,
    reward_threshold=2500.0,
    tags={ "pg_complexity": 8*1000000 },
    )
register(
    id='RoboschoolHalfCheetah-v1',
    entry_point='roboschool:RoboschoolHalfCheetah',
    max_episode_steps=1000,
    reward_threshold=3000.0,
    tags={ "pg_complexity": 8*1000000 },
    )

register(
    id='RoboschoolAnt-v1',
    entry_point='roboschool:RoboschoolAnt',
    max_episode_steps=1000,
    reward_threshold=2500.0,
    tags={ "pg_complexity": 8*1000000 },
    )

register(
    id='RoboschoolHumanoid-v1',
    entry_point='roboschool:RoboschoolHumanoid',
    max_episode_steps=1000,
    reward_threshold=3500.0,
    tags={ "pg_complexity": 100*1000000 },
    )
register(
    id='RoboschoolHumanoidFlagrun-v1',
    entry_point='roboschool:RoboschoolHumanoidFlagrun',
    max_episode_steps=1000,
    reward_threshold=2000.0,
    tags={ "pg_complexity": 200*1000000 },
    )
register(
    id='RoboschoolHumanoidFlagrunHarder-v1',
    entry_point='roboschool:RoboschoolHumanoidFlagrunHarder',
    max_episode_steps=1000,
    tags={ "pg_complexity": 300*1000000 },
    )


# Atlas

register(
    id='RoboschoolAtlasForwardWalk-v1',
    entry_point='roboschool:RoboschoolAtlasForwardWalk',
    max_episode_steps=1000,
    tags={ "pg_complexity": 200*1000000 },
    )


# Multiplayer

register(
    id='RoboschoolPong-v1',
    entry_point='roboschool:RoboschoolPong',
    max_episode_steps=1000,
    tags={ "pg_complexity": 20*1000000 },
    )

from roboschool.gym_pendulums import RoboschoolInvertedPendulum
from roboschool.gym_pendulums import RoboschoolInvertedPendulumSwingup
from roboschool.gym_pendulums import RoboschoolInvertedDoublePendulum
from roboschool.gym_reacher import RoboschoolReacher
from roboschool.gym_mujoco_walkers import RoboschoolHopper
from roboschool.gym_mujoco_walkers import RoboschoolWalker2d
from roboschool.gym_mujoco_walkers import RoboschoolHalfCheetah
from roboschool.gym_mujoco_walkers import RoboschoolAnt
from roboschool.gym_mujoco_walkers import RoboschoolHumanoid
from roboschool.gym_humanoid_flagrun import RoboschoolHumanoidFlagrun
from roboschool.gym_humanoid_flagrun import RoboschoolHumanoidFlagrunHarder
from roboschool.gym_atlas import RoboschoolAtlasForwardWalk
from roboschool.gym_pong import RoboschoolPong
