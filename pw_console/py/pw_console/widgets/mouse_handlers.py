# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Functions for handling mouse events."""

from typing import Callable

from prompt_toolkit.mouse_events import MouseEvent, MouseEventType


def on_click(on_click_function: Callable, mouse_event: MouseEvent):
    """Run a function on mouse click.

    Here is an example of how to add add click functionality to a piece of
    formatted text. ::

        import functools
        import pw_console.widgets.mouse_handlers

        def get_tokens(self):
            mouse_handler = functools.partial(
                pw_console.widgets.mouse_handlers.on_click,
                self.your_widget.do_something)
            return [
                (
                    'class:text-button',
                    ' Click Here to Do Something ',
                    mouse_handler,
                ),
            ]

    """
    if mouse_event.event_type == MouseEventType.MOUSE_UP:
        on_click_function()
        return None
    return NotImplemented
