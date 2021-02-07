from abc import ABC, abstractmethod

import rj_gameplay.eval as eval
import argparse
import py_trees
import sys
import time

import stp.skill as skill
import stp.role as role
import stp.action as action
from stp.action_behavior import ActionBehavior
import stp.rc as rc

class ICapture(skill.ISkill, ABC):
    ...


"""
A skill version of capture so that actions don't have to be called in tactics
"""
class Capture(ICapture):
    
    def __init__(self, role: stp.Role):
        self.robot = role.robot
        self.capture = action.capture.Capture()
        self.capture_behavior = ActionBehavior('Capture', self.capture)
        self.root = self.capture_behavior
        self.root.setup_with_descendants()

    def tick(self, world_state) -> None:
        self.root.tick_once()
