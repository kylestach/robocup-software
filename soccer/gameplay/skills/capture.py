import single_robot_composite_behavior
import behavior
from enum import Enum
import main
import role_assignment
import robocup
import constants
import skills.settle
import skills.collect
import math
import evaluation.ball


class Capture(single_robot_composite_behavior.SingleRobotCompositeBehavior):

    INTERCEPT_VELOCITY_THRESH_TO_SETTLE = 0.5
    INTERCEPT_VELOCITY_THRESH_TO_COLLECT = 0.4

    PROBABLE_KICK_CHANGE = 0.1

    # We want  to let the probably held count stabilize
    # but to do that we have to let it sit still
    # If it doesn't move to either extreme and stays at 50%
    # we can't transition away, so we need this counter
    MAX_FRAMES_IN_CAPTURED = 20

    # X number of frames in the past to look at
    PROBABLY_HELD_HISTORY_LENGTH = 60

    PROBABLY_HELD_START = .5 * PROBABLY_HELD_HISTORY_LENGTH

    # Anything below this number we think we don't have the ball
    PROBABLY_NOT_HELD_CUTOFF = .4 * PROBABLY_HELD_HISTORY_LENGTH
    # Anything above this number we think we have the ball
    PROBABLY_HELD_CUTOFF = .6 * PROBABLY_HELD_HISTORY_LENGTH


    # Accounts for the scaling needed to balance the robot change cost
    # Increase to decrease the chance of robot change
    # Decrease to improve the responsiveness in regard th changing robots
    SETTLE_COST_MULTIPLIER = 20

    # How "likely" it is for role assignment to choose the closest robot
    # Higher means more likely
    # 1 is the default number for role assignment in general
    # Only used for collect phase
    POSITION_COST_MULTIPLIER = 30

    class State(Enum):
        # May already have the ball so just sit for a split second and keep dribbler on
        captured = 0

        # Don't have ball so try to intercept the moving ball
        settle = 2

        # Don't have ball so try to do the fine approach to control the ball
        collect = 1

    ## Capture Constructor
    # faceBall - If false, any turning functions are turned off,
    # useful for using capture to reflect/bounce moving balls.
    def __init__(self, faceBall=True):
        super().__init__(continuous=False)

        for s in Capture.State:
            self.add_state(s, behavior.Behavior.State.running)

        self.prev_ball = main.ball()
        self.probably_held_cnt = Capture.PROBABLY_HELD_START
        self.frames_in_captured = 0

        
        # By default, move into the settle state since we almost never start with ball
        self.add_transition(behavior.Behavior.State.start,
                            Capture.State.settle,
                            lambda: self.robot is not None and not self.robot.has_ball(),
                            'dont have ball')

        # On the offchance we start with ball, double check it's not a blip on the sensor
        self.add_transition(behavior.Behavior.State.start,
                            Capture.State.captured,
                            lambda: self.robot is not None and self.robot.has_ball(),
                            'may already have ball')

        # We actually don't have the ball, either 50% register rate (faulty sensor?) or we just got a blip
        self.add_transition(Capture.State.captured,
                            Capture.State.settle,
                            lambda: self.probably_held_cnt < Capture.PROBABLY_NOT_HELD_CUTOFF or
                                    self.frames_in_captured > Capture.MAX_FRAMES_IN_CAPTURED,
                            'actually dont have ball')

        # Actually do have the ball, we can just leave
        self.add_transition(Capture.State.captured,
                            behavior.Behavior.State.completed,
                            lambda: self.probably_held_cnt > Capture.PROBABLY_HELD_CUTOFF,
                            'actually have ball')

        # If we intercepted the ball to slow it down enough or it most likely just bounced off our robot and
        # The velocity is just funky
        self.add_transition(Capture.State.settle,
                            Capture.State.collect,
                            lambda: main.ball().vel.mag() < Capture.INTERCEPT_VELOCITY_THRESH_TO_COLLECT, # or
                                    #self.ball_bounced_off_robot() and not self.ball_probably_kicked(),
                            'collecting')

        # Cut back if the velocity is pretty high or it was probably kicked
        self.add_transition(Capture.State.collect,
                            Capture.State.settle,
                            lambda: main.ball().vel.mag() >= Capture.INTERCEPT_VELOCITY_THRESH_TO_SETTLE and
                                    (self.subbehavior_with_name('collector').robot is not None and
                                     (self.subbehavior_with_name('collector').robot.pos - main.ball().pos).mag() > .3), # or 
                                    #self.ball_probably_kicked() and not self.ball_bounced_off_robot(),
                            'settling again')

        #  Once collect is done, we have the ball
        self.add_transition(Capture.State.collect,
                            behavior.Behavior.State.completed,
                            lambda: self.subbehavior_with_name('collector').is_done_running(),
                            'captured')

        # Go back to settle if we loose the ball when completed
        self.add_transition(behavior.Behavior.State.completed,
                            Capture.State.settle,
                            lambda: self.probably_held_cnt < Capture.PROBABLY_NOT_HELD_CUTOFF,
                            'captured')


    def ball_bounced_off_robot(self):
        # Check if ball is near current robot
        # Check if the ball velocity went from coming to us
        #   to moving away
        if (self.robot is not None):
            old_ball_to_bot = self.robot.pos - self.prev_ball.pos
            ball_to_bot = self.robot.pos - main.ball().pos

            # If the old ball is not in the same direction as the current one
            is_bounce = (self.is_same_direction(old_ball_to_bot, self.prev_ball.vel) and
                         not self.is_same_direction(ball_to_bot, main.ball().vel))

            # And we are near the ball
            near_robot = ball_to_bot.mag() < 2*constants.Robot.Radius

            return is_bounce and near_robot

        else:
            return False

    def ball_probably_kicked(self):
        if (self.robot is not None):
            # Check if there is a significant jump in velocity
            prob_kick = (self.prev_ball.vel - main.ball().vel).mag() > Capture.PROBABLE_KICK_CHANGE

            # And we are not near the ball
            near_robot = (self.robot.pos - main.ball().pos).mag() < 2*constants.Robot.Radius

            return prob_kick and not near_robot
        else:
            return False

    def is_same_direction(self, vec1, vec2):
        # Check the angle between is less than 90 degrees
        # On the unit circle, two vectors are within 90 degrees of each other if the distance
        # between two vectors is less than sqrt(1^2 + 1^2)
        #
        #              +1 | a
        #                 |
        #                 |
        #                 |            b
        #                 |_____________
        #                              +1
        #
        # Dist between a and b are sqrt(2)
        #
        #
        #     a                         b
        #     ____________________________
        #     -1                        +1
        #
        # Dist between a and b is 2
        return (vec1.norm() - vec2.norm()).mag() < math.sqrt(2)

    def on_enter_captured(self):
        print("Enter captured")

    def execute_captured(self):
        self.update_held_cnt()
        self.frames_in_captured += 1

    def on_enter_settle(self):
        print("Enter settle")
        self.remove_all_subbehaviors()
        self.add_subbehavior(skills.settle.Settle(),
                             name='settler',
                             required=True,
                             priority=10)

    def on_enter_collect(self):
        print("Enter collect")
        self.remove_all_subbehaviors()
        self.add_subbehavior(skills.collect.Collect(),
                             name='collector',
                             required=True,
                             priority=10)

    def on_enter_completed(self):
        self.probably_held_cnt = Capture.PROBABLY_HELD_START

    def execute_completed(self):
        self.update_held_cnt()

    def update_held_cnt(self):
        if (self.robot is None):
            return

        # If we hold the ball, increment up to max
        # if not, drop to 0
        if (evaluation.ball.robot_has_ball(self.robot)): #self.robot.has_ball()):
        #if (self.robot is not None and self.robot.vel.mag() < Collect.STOP_SPEED):
            self.probably_held_cnt = min(self.probably_held_cnt + 1, Capture.PROBABLY_HELD_HISTORY_LENGTH)
        else:
            self.probably_held_cnt = max(self.probably_held_cnt - 1, 0)

    @staticmethod
    def role_assignment_cost(robot):
        # Estimate the time it takes to intercept the ball

        # Use straight line distance to closest point along line
        # Then movement down the line

        # Represents ball movement line
        ball_move_line = robocup.Line(main.ball().pos, main.ball().pos + main.ball().vel * 10)

        # Closest point to robot on ball movement line
        ball_line_intercept = ball_move_line.nearest_point(robot.pos)

        ball_to_robot = robot.pos - main.ball().pos
        ball_to_intercept = ball_line_intercept - main.ball().pos
        robot_to_intercept = ball_line_intercept - robot.pos

        # If we are actually in front of the ball angle wise
        # OR close enough that it could be a bounce off the robot due to noisy vision
        in_front = ball_move_line.delta().angle_between(ball_to_robot) < math.pi/2 or \
                   ball_to_robot.mag() < 1.5*(constants.Robot.Radius + constants.Ball.Radius)

        # Calculate how much further the ball will move before we reach the point

        max_speed = robocup.MotionConstraints.MaxRobotSpeed.value
        max_acc = robocup.MotionConstraints.MaxRobotAccel.value

        # How long it will take the robot/ball to reach that intercept point
        ball_secs = main.ball().estimate_seconds_to(ball_line_intercept)
        robot_secs = robocup.get_trapezoidal_time(robot_to_intercept.mag(), robot_to_intercept.mag(), max_speed, max_acc, 0, 0)

        time_to_hit = robot_secs

        # If we aren't in front of the ball, we need to get time to catch up
        if (not in_front):
            # Difference in speed between us and ball
            delta_speed = max_speed - main.ball().vel.mag()

            # if we are slower than the ball, don't even try and just exclude it automatically
            if (delta_speed < 0):
                time_to_hit += 100
            # If we are fast, figure out how much more time we need
            else:
                delta_time = robocup.get_trapezoidal_time(ball_to_intercept.mag(), ball_to_intercept.mag(), delta_speed, max_acc, 0, 0)
                time_to_hit += delta_time
        

        main.system_state().draw_text(str(time_to_hit), robot.pos + robocup.Point(0.1,0.1), (255,255,255), "test")

        return time_to_hit * Capture.SETTLE_COST_MULTIPLIER


    def role_requirements(self):
        reqs = super().role_requirements()
        #reqs.require_kicking = True

        # try to be near the ball
        for req in role_assignment.iterate_role_requirements_tree_leaves(reqs):
            if main.ball().valid:
                # Slow ball so we can move in from any direction
                if main.ball().vel.mag() < Capture.INTERCEPT_VELOCITY_THRESH_TO_COLLECT:
                    # Get the closest robot to the ball
                    # Increase the cost significantly for robots far away so that it
                    # picks the robot already controlling the ball
                    req.destination_shape = main.ball().pos
                    req.position_cost_multiplier = Capture.POSITION_COST_MULTIPLIER
                # Fast ball, want something downfield
                else:
                    req.cost_func = Capture.role_assignment_cost
                    req.robot_change_cost = 10
                    req.destination_shape = None
        
        return reqs