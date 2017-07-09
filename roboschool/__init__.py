from gym.envs.registration import register
#from gym.scoreboard.registration import add_task, add_group

register(
    id='RoboschoolInvertedPendulum-v0',
    entry_point='roboschool:RoboschoolInvertedPendulum',
    max_episode_steps=1000,
    reward_threshold=950.0,
    )
register(
    id='RoboschoolInvertedPendulumSwingup-v0',
    entry_point='roboschool:RoboschoolInvertedPendulumSwingup',
    max_episode_steps=1000,
    reward_threshold=800.0,
    )
register(
    id='RoboschoolInvertedDoublePendulum-v0',
    entry_point='roboschool:RoboschoolInvertedDoublePendulum',
    max_episode_steps=1000,
    reward_threshold=9100.0,
    )

register(
    id='RoboschoolReacher-v0',
    entry_point='roboschool:RoboschoolReacher',
    max_episode_steps=150,
    reward_threshold=18.0,
    )

register(
    id='RoboschoolHopper-v0',
    entry_point='roboschool:RoboschoolHopper',
    max_episode_steps=1000,
    reward_threshold=2500.0
    )
register(
    id='RoboschoolWalker2d-v0',
    entry_point='roboschool:RoboschoolWalker2d',
    max_episode_steps=1000,
    reward_threshold=2500.0
    )
register(
    id='RoboschoolHalfCheetah-v0',
    entry_point='roboschool:RoboschoolHalfCheetah',
    max_episode_steps=1000,
    reward_threshold=3000.0
    )

register(
    id='RoboschoolAnt-v0',
    entry_point='roboschool:RoboschoolAnt',
    max_episode_steps=1000,
    reward_threshold=2500.0
    )

register(
    id='RoboschoolHumanoid-v0',
    entry_point='roboschool:RoboschoolHumanoid',
    max_episode_steps=1000,
    reward_threshold=3500.0
    )
register(
    id='RoboschoolHumanoidFlagrun-v0',
    entry_point='roboschool:RoboschoolHumanoidFlagrun',
    max_episode_steps=1000,
    reward_threshold=2000.0
    )
register(
    id='RoboschoolHumanoidFlagrunHarder-v0',
    entry_point='roboschool:RoboschoolHumanoidFlagrunHarder',
    max_episode_steps=1000
    )


# Atlas

register(
    id='RoboschoolAtlasForwardWalk-v0',
    entry_point='roboschool:RoboschoolAtlasForwardWalk',
    max_episode_steps=1000
    )


# Multiplayer

register(
    id='RoboschoolPong-v0',
    entry_point='roboschool:RoboschoolPong',
    max_episode_steps=1000
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
