import matplotlib
import numpy as np


class CustomFlag:
    _state = False

    def __init__(self, state: bool = False):
        self._state = state
        pass

    def getState(self):
        return self._state

    def setState(self, state):
        self._state = state


class CustomAxes(matplotlib.axes.Axes):
    name = "CustomAxes"
    registeredPoints = np.array([0., 0.])
    stopLiveFlag = CustomFlag()

    zoomFactory = 0

    def drag_pan(self, button, key, x, y):
        matplotlib.axes.Axes.drag_pan(self, button, 'x', x, y)
        visiblePoints = self._get_pan_points(button, key, x, y)
        if visiblePoints is not None:
            visiblePoints = visiblePoints[:, 0]
            self.registeredPoints[0], self.registeredPoints[1] = visiblePoints
            self.stopLiveFlag.setState(True)

    def registerPoints(self, points):
        self.registeredPoints = points
        pass

    def registerFlag(self, flag: CustomFlag):
        self.stopLiveFlag = flag

    def enableZooming(self, base_scale=1+.5):
        def zoom_factory(ax):
            def zoom_fun(event):
                # self.stopLiveFlag.setState(True)
                # get the current x limits
                cur_xlim = ax.get_xlim()
                xdata = event.xdata  # get event x location
                if event.button == 'up':
                    # deal with zoom in
                    scale_factor = 1 / base_scale
                elif event.button == 'down':
                    # deal with zoom out
                    scale_factor = base_scale
                else:
                    # deal with something that should never happen
                    scale_factor = 1

                # self.registeredPoints[0], self.registeredPoints[1] = [(xdata - cur_xlim[0]) * scale_factor,
                #                                                       (xdata - cur_xlim[1]) * scale_factor]
                self.registeredPoints[0], self.registeredPoints[1] = [xdata - (xdata - cur_xlim[0]) * scale_factor,
                                                                          xdata - (xdata - cur_xlim[1]) * scale_factor]
                # ax.set_xlim(self.registeredPoints)

            fig = ax.get_figure()  # get the figure of interest
            # attach the call back
            fig.canvas.mpl_connect('scroll_event', zoom_fun)

            # return the function
            return zoom_fun

        self.zoomFactory = zoom_factory(self)
