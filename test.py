import roboschool
import gym

env = gym.make('RoboschoolAnt-v1')
env.reset()
while True:
    ac = env.action_space.sample()
    env.step(ac)
    env.render()

    
