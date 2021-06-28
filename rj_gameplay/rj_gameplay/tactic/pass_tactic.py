from dataclasses import dataclass
from typing import List, Optional
from typing import Dict, Generic, List, Optional, Tuple, Type, TypeVar

import stp.action as action
import stp.rc as rc
import stp.tactic as tactic
import stp.role as role

import rj_gameplay.eval
import rj_gameplay.skill as skills
from rj_gameplay.skill import pivot_kick, receive
import stp.skill as skill
import numpy as np


class ReceiverCost(role.CostFn):
    """
    A cost function for how to choose a robot to pass to
    TODO: Implement a better cost function
    """
    def __init__(self, target_point:Optional[np.ndarray] = None):
        self.target_point = target_point

    def __call__(
        self,
        robot: rc.Robot,
        prev_result: Optional["RoleResult"],
        world_state: rc.WorldState,
    ) -> float:
    
        if robot.id == 7:
            return 0.0
        return 1.0

class PasserCost(role.CostFn):
    """
    A cost function for how to choose a robot that will pass
    TODO: Implement a better cost function
    """
    def __call__(self,
                robot:rc.Robot,
                prev_result:Optional["RoleResult"],
                world_state:rc.WorldState) -> float:
        if robot.has_ball_sense:
            return 0
        else:
            if robot.id == 1:
                return 0
            else:
                return 1



class Pass(tactic.ITactic):
    """
    A passing tactic which captures then passes the ball
    """

    def __init__(self, target_point:np.ndarray):
        self.target_point = target_point
        self.pivot_kick = tactic.SkillEntry(pivot_kick.PivotKick(target_point = target_point, chip=False, kick_speed=4.0))
        self.receive = tactic.SkillEntry(receive.Receive())
        self.receiver_cost = ReceiverCost(target_point)
        self.Passer_cost = PasserCost()
        
    def compute_props(self):
        pass

    def create_request(self, **kwargs) -> role.RoleRequest:
        """Creates a sane default RoleRequest.
        :return: A list of size 1 of a sane default RoleRequest.
        """
        pass

    def get_requests(
        self, world_state:rc.WorldState, props) -> List[tactic.RoleRequests]:
        """ Checks if we have the ball and returns the proper request
        :return: A list of size 2 of role requests
        """

        role_requests: tactic.RoleRequests = {}

        passer_request = role.RoleRequest(role.Priority.HIGH, True, self.Passer_cost)
        role_requests[self.pivot_kick] = [passer_request]
        receive_request = role.RoleRequest(role.Priority.HIGH, True, self.receiver_cost)
        role_requests[self.receive] = [receive_request]

        return role_requests

    def tick(self, role_results:tactic.RoleResults, world_state:rc.WorldState) -> List[tactic.SkillEntry]:
        """
        :return: A list of size 1 or 2 skills depending on which roles are filled and state of aiming
        TODO: Come up with better timings for starting receive
        """
        pivot_result = role_results[self.pivot_kick]
        receive_result = role_results[self.receive]

        if pivot_result and receive_result and pivot_result[0].is_filled() and receive_result[0].is_filled():
            self.pivot_kick.skill.target_point = np.array(receive_result[0].role.robot.pose[0:2])
            if self.pivot_kick.skill.kick.is_done(world_state):
                return [self.pivot_kick, self.receive]
            else:
                return [self.pivot_kick]
        elif pivot_result and pivot_result[0].is_filled():
            return [self.pivot_kick]
        return []

    def is_done(self, world_state:rc.WorldState):
        return self.receive.skill.is_done(world_state)