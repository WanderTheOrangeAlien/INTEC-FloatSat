"""
FloatSat telemetry viewer — minimal working example.

Features:
  - Horizontally scrollable / zoomable plot (native PyQtGraph pan+wheel zoom)
  - Hover crosshair showing (x, y) of the nearest sample
  - Click on the plot to drop a persistent marker (with a small label)
  - Data source is abstracted behind DataSource, so swapping UART for
    LoRa/WiFi later just means writing a new subclass — nothing else changes.

Run with:
    pip install pyqtgraph PyQt5 pyserial numpy
    python floatsat_gui.py            # simulated data
    python floatsat_gui.py --port COM5 --baud 115200   # real UART

Serial line format expected (one sample per line):
    <timestamp_ms>,<value>
e.g.
    1234,3.57
"""

import sys
import argparse
import threading
import time
import queue

import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets


# --------------------------------------------------------------------------
# Data sources — swap this out later for LoRa/WiFi without touching the GUI
# --------------------------------------------------------------------------

class DataSource(threading.Thread):
    """Base class: runs in its own thread, pushes (t, value) tuples to a queue."""

    def __init__(self, out_queue: "queue.Queue"):
        super().__init__(daemon=True)
        self.out_queue = out_queue
        self._stop_flag = threading.Event()

    def stop(self):
        self._stop_flag.set()

    def run(self):
        raise NotImplementedError


class SimulatedSource(DataSource):
    """Fake telemetry generator, useful for developing the GUI without hardware."""

    def run(self):
        t0 = time.time()
        while not self._stop_flag.is_set():
            t = time.time() - t0
            value = np.sin(2 * np.pi * 0.3 * t) + 0.1 * np.random.randn()
            self.out_queue.put((t, value))
            time.sleep(0.02)  # ~50 Hz


class SerialSource(DataSource):
    """Reads '<timestamp_ms>,<value>' lines from a UART port."""

    def __init__(self, out_queue: "queue.Queue", port: str, baud: int = 115200):
        super().__init__(out_queue)
        self.port = port
        self.baud = baud

    def run(self):
        import serial  # imported here so the sim mode doesn't require pyserial

        t0 = None
        with serial.Serial(self.port, self.baud, timeout=1) as ser:
            while not self._stop_flag.is_set():
                line = ser.readline().decode(errors="ignore").strip()
                if not line:
                    continue
                try:
                    ts_ms, value = line.split(",")
                    t = float(ts_ms) / 1000.0
                    value = float(value)
                except ValueError:
                    continue  # skip malformed lines
                if t0 is None:
                    t0 = t
                self.out_queue.put((t - t0, value))


# --------------------------------------------------------------------------
# Ring-buffer data model
# --------------------------------------------------------------------------

class RingBuffer:
    """Simple growable buffer for (t, value) samples backed by numpy arrays."""

    def __init__(self, capacity: int = 200_000):
        self.capacity = capacity
        self.t = np.empty(capacity)
        self.y = np.empty(capacity)
        self.size = 0

    def append(self, t: float, y: float):
        if self.size >= self.capacity:
            # drop oldest half to keep going indefinitely without reallocating
            keep = self.capacity // 2
            self.t[:keep] = self.t[self.size - keep:self.size]
            self.y[:keep] = self.y[self.size - keep:self.size]
            self.size = keep
        self.t[self.size] = t
        self.y[self.size] = y
        self.size += 1

    def arrays(self):
        return self.t[:self.size], self.y[:self.size]


# --------------------------------------------------------------------------
# Main window
# --------------------------------------------------------------------------

class TelemetryWindow(QtWidgets.QMainWindow):
    def __init__(self, source: DataSource):
        super().__init__()
        self.setWindowTitle("FloatSat Telemetry Viewer")
        self.resize(1000, 600)

        self.buffer = RingBuffer()
        self.sample_queue: "queue.Queue" = source.out_queue
        self.source = source

        # --- plot setup ---
        pg.setConfigOptions(antialias=True)
        self.plot_widget = pg.PlotWidget()
        self.setCentralWidget(self.plot_widget)

        self.plot_item = self.plot_widget.getPlotItem()
        self.plot_item.setLabel("bottom", "Time", units="s")
        self.plot_item.setLabel("left", "Value")
        self.plot_item.showGrid(x=True, y=True, alpha=0.3)

        self.curve = self.plot_item.plot(pen=pg.mkPen(width=2))

        # follow the newest data by default; user can scroll/zoom to inspect history
        self.autoscroll = True
        self.view_span = 10.0  # seconds visible when autoscrolling

        # --- crosshair (hover) ---
        self.vline = pg.InfiniteLine(angle=90, movable=False, pen=pg.mkPen("y", width=1))
        self.hline = pg.InfiniteLine(angle=0, movable=False, pen=pg.mkPen("y", width=1))
        self.plot_item.addItem(self.vline, ignoreBounds=True)
        self.plot_item.addItem(self.hline, ignoreBounds=True)
        self.coord_label = pg.TextItem(color="y", anchor=(0, 1))
        self.plot_item.addItem(self.coord_label)

        self.mouse_proxy = pg.SignalProxy(
            self.plot_widget.scene().sigMouseMoved, rateLimit=60, slot=self._on_mouse_moved
        )

        # --- click-to-mark ---
        self.markers = pg.ScatterPlotItem(size=10, brush=pg.mkBrush("r"), pen=pg.mkPen("w"))
        self.plot_item.addItem(self.markers)
        self._marker_points = []  # list of (x, y) so we can redraw with labels
        self._marker_labels = []
        self.plot_widget.scene().sigMouseClicked.connect(self._on_mouse_clicked)

        # detect manual pan/zoom to disable autoscroll
        self.plot_item.vb.sigRangeChangedManually.connect(self._on_manual_range_change)

        # --- timer to drain queue + redraw ---
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self._update)
        self.timer.start(30)  # ~33 Hz UI refresh

    # ---- data pump ----

    def _update(self):
        drained = False
        try:
            while True:
                t, y = self.sample_queue.get_nowait()
                self.buffer.append(t, y)
                drained = True
        except queue.Empty:
            pass

        if not drained:
            return

        t_arr, y_arr = self.buffer.arrays()
        self.curve.setData(t_arr, y_arr)  # setData(), never clear()+replot for perf

        if self.autoscroll and t_arr.size:
            t_latest = t_arr[-1]
            self.plot_item.setXRange(max(0, t_latest - self.view_span), t_latest, padding=0)

    # ---- hover crosshair ----

    def _on_mouse_moved(self, evt):
        pos = evt[0]
        if not self.plot_item.sceneBoundingRect().contains(pos):
            return
        mouse_point = self.plot_item.vb.mapSceneToView(pos)
        x = mouse_point.x()

        t_arr, y_arr = self.buffer.arrays()
        if t_arr.size == 0:
            return

        idx = np.searchsorted(t_arr, x)
        idx = min(max(idx, 0), t_arr.size - 1)
        nearest_t, nearest_y = t_arr[idx], y_arr[idx]

        self.vline.setPos(nearest_t)
        self.hline.setPos(nearest_y)
        self.coord_label.setText(f"t={nearest_t:.3f}s, y={nearest_y:.3f}")
        self.coord_label.setPos(nearest_t, nearest_y)

    # ---- click-to-mark ----

    def _on_mouse_clicked(self, evt):
        pos = evt.scenePos()
        if not self.plot_item.sceneBoundingRect().contains(pos):
            return
        mouse_point = self.plot_item.vb.mapSceneToView(pos)
        x = mouse_point.x()

        t_arr, y_arr = self.buffer.arrays()
        if t_arr.size == 0:
            return
        idx = np.searchsorted(t_arr, x)
        idx = min(max(idx, 0), t_arr.size - 1)
        mx, my = t_arr[idx], y_arr[idx]

        self._marker_points.append({"pos": (mx, my), "data": len(self._marker_points)})
        self.markers.setData(self._marker_points)

        label = pg.TextItem(text=f"#{len(self._marker_points)}", color="w", anchor=(0.5, 1.3))
        label.setPos(mx, my)
        self.plot_item.addItem(label)
        self._marker_labels.append(label)

    # ---- autoscroll toggling ----

    def _on_manual_range_change(self, *args):
        # user dragged/scrolled -> stop auto-following latest sample
        self.autoscroll = False

    def keyPressEvent(self, event):
        # press 'A' to jump back to auto-follow mode
        if event.key() == QtCore.Qt.Key_A:
            self.autoscroll = True
        super().keyPressEvent(event)

    def closeEvent(self, event):
        self.source.stop()
        super().closeEvent(event)


def main():
    parser = argparse.ArgumentParser(description="FloatSat telemetry viewer")
    parser.add_argument("--port", type=str, default=None, help="UART port, e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    sample_queue: "queue.Queue" = queue.Queue()

    if args.port:
        source = SerialSource(sample_queue, args.port, args.baud)
    else:
        print("No --port given, running with simulated data.")
        source = SimulatedSource(sample_queue)

    source.start()

    app = QtWidgets.QApplication(sys.argv)
    win = TelemetryWindow(source)
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
