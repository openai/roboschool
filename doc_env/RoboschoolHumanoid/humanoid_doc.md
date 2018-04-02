# RoboschoolHumanoid

## Action space

The action space is a vector of 17 float values in the range [-1, 1]. Each value corresponds to the joints of the avatar by this order (XML):
- abdomen_y
- abdomen_z
- abdomen_x
- right_hip_x
- right_hip_z
- right_hip_y
- right_knee
- left_hip_x
- left_hip_z
- left_hip_y
- left_knee
- right_shoulder1
- right_shoulder2
- right_elbow
- left_shoulder1
- left_shoulder2
- left_elbow
At each step, these values are applied to all the joints of the body by the code

```
for n,j in enumerate(self.ordered_joints):
    j.set_motor_torque( self.power*j.power_coef*float(np.clip(a[n], -1, +1)) )
```

in the `apply_action` function in the class which extends the `gym.Env` class (`RoboschoolMujocoXmlEnv`) to set the torque value into the respective motor.

## Observation space 

The state space (or observation space) is a vector of 44 float values in the range [-5, 5] (Roboschool clip the vector with numpy before returning it in the `step` function). That vector is a concatenation of three subvectors:
- **more**: It is a vector of 8 values defined as follows:
    - The distance between the last position of the body and the current one.
    - The sinus of the angle to the target
    - The cosinus of the angle to the target
    - The three next values is the matrix multiplication between 
        - $\begin{pmatrix}cos(-yaw) & -sin(-yaw) & 0\\sin(-yaw) & cos(-yaw) & 0 \\0 & 0 & 1\end{pmatrix}$
        - the speed vector of the body.
    - The roll value of the body
    - The pitch value of the body
- **j** : This is the current relative position of the joint described earlier and their current speed. The position is in the even position, and the speed in the odds (34 values).
- **feet_contact**: Boolean values, 0 or 1, for left and right feet, indicating if the respective feet is touching the ground or not.
    
## Reward 

The reward is a sum of 5 computed values:

- **alive**: -1 or +1 wether is on the ground or not.
- **progress**: potential minus the old potential. The potential is defined by
    the speed multiplied by the distance to target point, to the negative.
- **electricity_cost**: The amount of energy needed for the last action.
- **joints_at_limit_cost**: The amount of collision between joints of body
      during the last action.
- **feet_colision_cost**: The amount of feet collision taken during the last action.
