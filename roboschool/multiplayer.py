from roboschool.scene_abstract import SingleRobotEmptyScene
import numpy as np
import os
import gym

MULTIPLAYER_FILES_DIR = "/tmp"

class SharedMemoryClientEnv:
    """
    This class is integrated into Environment on client, by using multiple inheritance.

    Environment that support multiplayer inherite from this class. If you call gym.make("RoboschoolHopper-v0")
    you well get all functions there in your environment.

    The most important is multiplayer() below, others are more technical.
    """

    def shmem_client_init(self, game_server_guid, player_n):
        """
        This class maps files into memory:

        /tmp/multiplayer_game_server_guid_player00_obs
        /tmp/multiplayer_game_server_guid_player01_obs
        ...

        These arrays allows server and client to have access to the same data, without a need to copy it.

        This class also creates named pipes, that is used to inform other side when things happen (something
        you can't do with shared memory):

        /tmp/multiplayer_game_server_guid_player00_obsready

        """
        assert isinstance(self.observation_space, gym.spaces.Box)
        assert isinstance(self.action_space, gym.spaces.Box)
        assert isinstance(game_server_guid, str)
        assert isinstance(player_n, int)
        assert("sh_obs" not in self.__dict__)
        self.game_server_guid = game_server_guid
        self.player_n = player_n
        self.prefix = MULTIPLAYER_FILES_DIR + "/multiplayer_%s_player%02i" % (game_server_guid, player_n)
        self.sh_pipe_actready_filename = self.prefix + "_actready"
        self.sh_pipe_actready = os.open(self.sh_pipe_actready_filename, os.O_WRONLY)
        self.sh_pipe_obsready_filename = self.prefix + "_obsready"
        self.sh_pipe_obsready = open(self.sh_pipe_obsready_filename, 'rt')

    def shmem_client_send_env_id(self):
        """
        Multiplayer Scene can support multiple kinds of environments (robots, actors).
        For example, Stadium supports Hopper and Ant.

        On server side, environment of the same type should be created. To do
        that, we send env_id over pipe.

        Obervations, actions must have size matching that on server. So we open shared memory
        files at this point, after server created those files based on knowledge it now has,
        and sent "accepted" back here.
        """
        os.write(self.sh_pipe_actready, (self.spec.id + "\n").encode("ascii"))
        check = self.sh_pipe_obsready.readline()[:-1]
        assert(check=="accepted")
        self.sh_obs = np.memmap(self.prefix + "_obs",  mode="r+", shape=self.observation_space.shape, dtype=np.float32)
        self.sh_act = np.memmap(self.prefix + "_act",  mode="r+", shape=self.action_space.shape, dtype=np.float32)
        self.sh_rew = np.memmap(self.prefix + "_rew",  mode="r+", shape=(1,), dtype=np.float32)
        self.sh_rgb = np.memmap(self.prefix + "_rgb",  mode="r+", shape=(self.VIDEO_H,self.VIDEO_W,3), dtype=np.uint8)

    def shmem_client_step(self, a):
        """
        [:] notion means to put data into existing array (shared memory), not create new array.
        """
        self.sh_act[:] = a
        os.write(self.sh_pipe_actready, b'a\n')
        check = self.sh_pipe_obsready.readline()[:-1]  # It blocks here, until server responds with "t" or "D" (tuple or tuple+done)
        if check=='t':  # tuple
            done = False
        elif check=='D':
            done = True
        else:
            raise ValueError("multiplayer server returned invalid string '%s', probably was shut down" % check)
        return self.sh_obs, self.sh_rew[0], done, {}

    def shmem_client_reset(self):
        """
        Reset sends "R" and expects "o" for observations, if it sees something else, like "t", it means server is broken.
        """
        os.write(self.sh_pipe_actready, b'R\n')
        check = self.sh_pipe_obsready.readline()[:-1]
        if check=='o':
            return self.sh_obs
        else:
            raise ValueError("multiplayer server returned invalid string '%s' on reset, probably was shut down" % check)

    def shmem_client_rgb_array(self, mode, close):
        """
        This can render video image for render("rgb_array"), also works using shared memory.
        """
        if close:
            return
        if mode=="rgb_array":
            os.write(self.sh_pipe_actready, b'G\n')
            check = self.sh_pipe_obsready.readline()[:-1]
            if check=='i':
                return self.sh_rgb
            else:
                raise ValueError("multiplayer server returned invalid string '%s' on rgb_array, probably was shut down" % check)
        elif mode=="human":
            return None  # ask scene for test window, player requests ignored here
        else:
            assert(0)

    def multiplayer(self, env, game_server_guid, player_n):
        """
        That's the function you call between gym.make() and first env.reset(), to connect to multiplayer server.

        game_server_guid -- is an id that server and client use to identify themselves to belong to the same session.
        player_n -- integer, up to scene.players_count.

        You see here env.reset gets overwritten, that means if you call env.reset(), it will not create
        single player scene on your side (as it usually do), but rather it will communicate to server, reset environment
        there. Same with step() and render().
        """
        self.shmem_client_init(game_server_guid, player_n)
        env.step   = self.shmem_client_step  # replace real function with fake, that communicates with environment on server
        env.reset  = self.shmem_client_reset
        env.render = self.shmem_client_rgb_array
        self.shmem_client_send_env_id()

class SharedMemoryPlayerAgent:
    """
    This object resides on server, communicates with client and acts on its behalf (so it's called agent).

    It contains real env, controlled by appying commands from remote client.
    """
    def __init__(self, scene, game_server_guid, player_n):
        """
        It doesn't know env_id yet, it creates pipes and waits for client to send his env_id.
        """
        self.scene = scene
        assert isinstance(game_server_guid, str)
        assert isinstance(player_n, int)
        self.player_n = player_n
        self.prefix = MULTIPLAYER_FILES_DIR + "/multiplayer_%s_player%02i" % (game_server_guid, player_n)
        self.sh_pipe_actready_filename = self.prefix + "_actready"
        self.sh_pipe_obsready_filename = self.prefix + "_obsready"
        try: os.unlink(self.sh_pipe_actready_filename)
        except: pass
        os.mkfifo(self.sh_pipe_actready_filename)
        try: os.unlink(self.sh_pipe_obsready_filename)
        except: pass
        os.mkfifo(self.sh_pipe_obsready_filename)
        print("Waiting %s" % self.prefix)
        self.need_reset = True
        self.need_response_tuple = False

    def read_env_id_and_create_env(self):
        self.sh_pipe_actready = open(self.sh_pipe_actready_filename, "rt")
        self.sh_pipe_obsready = os.open(self.sh_pipe_obsready_filename, os.O_WRONLY)
        env_id = self.sh_pipe_actready.readline()[:-1]
        if env_id.find("-v")==-1:
            raise ValueError("multiplayer client %s sent here invalid environment id '%s'" % (self.prefix, env_id))
        #
        # And at this point we know env_id.
        #
        print("Player %i connected, wants to operate %s in this scene" % (self.player_n, env_id))
        self.env = gym.make(env_id)  # gym.make() creates at least timeout wrapper, we need it.

        self.env.unwrapped.scene = self.scene
        self.env.unwrapped.player_n = self.player_n
        assert isinstance(self.env.observation_space, gym.spaces.Box)
        assert isinstance(self.env.action_space, gym.spaces.Box)
        self.sh_obs = np.memmap(self.prefix + "_obs",  mode="w+", shape=self.env.observation_space.shape, dtype=np.float32)
        self.sh_act = np.memmap(self.prefix + "_act",  mode="w+", shape=self.env.action_space.shape, dtype=np.float32)
        self.sh_rew = np.memmap(self.prefix + "_rew",  mode="w+", shape=(1,), dtype=np.float32)
        self.sh_rgb = np.memmap(self.prefix + "_rgb",  mode="w+", shape=(self.env.unwrapped.VIDEO_H,self.env.unwrapped.VIDEO_W,3), dtype=np.uint8)
        os.write(self.sh_pipe_obsready, b'accepted\n')

    def read_and_apply_action(self):
        """
        This functions blocks until client sends a command: step() or reset() or requests video image.
        It quits when user has finished communicating and now expects simulation to advance.
        """
        while 1:
            if self.passive:
                self.env.unwrapped.apply_action(np.zeros(shape=self.sh_act.shape))
                return
            check = self.sh_pipe_actready.readline()[:-1]

            if check=='a':
                #assert(not self.need_reset)
                self.env.unwrapped.apply_action(self.sh_act)
                self.need_response_tuple = True
                return

            elif check=='R':
                obs = self.env.reset()
                self.sh_obs[:] = obs
                os.write(self.sh_pipe_obsready, b'o\n')
                self.need_response_tuple = False # Already answered
                if self.need_reset:
                    self.done = False
                    self.passive = False
                    self.need_reset = False
                else:
                    self.done = True  # User has decided he wants to restart, make robot passive
                    self.passive = True
                    self.need_reset = False

            elif check=='G':
                rgb = self.env.render("rgb_array")
                assert rgb.shape==self.sh_rgb.shape
                self.sh_rgb[:] = rgb
                os.write(self.sh_pipe_obsready, b'i\n')
                self.need_response_tuple = False

            else:
                raise ValueError("multiplayer client %s sent here invalid string '%s'" % (self.prefix, check))

    def step_and_push_result_tuple(self):
        """
        env.step() here doesn't step the simulation, scene.global_step() does that.
        But it still calls step(a) with the same actions, in order for actions to affect reward.
        See demo_race1.py for short example of how multiplayer works internally.
        """
        if self.passive:
            return
        if not self.need_response_tuple:
            return
        state, reward, done, _ = self.env.step(self.sh_act)
        if done:
            self.passive = True
            self.done = True
            self.need_reset = True
        self.sh_obs[:] = state
        #print("%s [%s]" % (self.prefix + "obs", ", ".join(["%+0.2f"%x for x in state])))
        self.sh_rew[0] = reward
        self.need_response_tuple = False
        os.write(self.sh_pipe_obsready, b't\n' if not done else b'D\n')   # 't' for tuple

class SharedMemoryServer:
    """
    1. Create scene,
    2. Create this SharedMemoryServer,
    3. Spawn client subprocesses (or manually run server in console, run clients in other consoles),
    4. call serve_forever()
    5. It will quit if any of clients malfunction or finish learning.

    See demo_race1.py example.
    """
    def __init__(self, scene, game_server_guid, want_test_window):
        self.scene = scene
        self.plist = []
        self.want_test_window = want_test_window
        for n in range(scene.players_count):
            player = SharedMemoryPlayerAgent(
                scene,
                game_server_guid,
                player_n=n)
            self.plist.append(player)

    def serve_forever(self):
        for p in self.plist:
            p.read_env_id_and_create_env()

        still_open = True
        episode = 0
        while still_open:
            episode += 1
            self.scene.episode_restart()
            for p in self.plist:
                p.done = False
                p.passive = False

            frame = 0
            while still_open:
                if self.want_test_window:
                    still_open = self.scene.test_window()

                for p in self.plist:
                    p.read_and_apply_action()

                self.scene.global_step()
                frame += 1

                for p in self.plist:
                    p.step_and_push_result_tuple()

                done = [1 for p in self.plist if p.done]
                if len(done)==len(self.plist): break

            #print("episode %i finished after %i frames" % (episode, frame))
