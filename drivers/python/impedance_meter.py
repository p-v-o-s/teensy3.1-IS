import glob
import serial

DEVPATH_PATTERN = "/dev/ttyACM*"

class Interface(object):
    def __init__(self, serial_devpath):
        self._ser = serial.Serial(serial_devpath)
    def synthesize(self, freq):
        cmd = "SYNTH.FREQ %d\n" % freq
        self._ser.write(cmd)

################################################################################
# TEST CODE
################################################################################
if __name__ == "__main__":
    devpath = glob.glob(DEVPATH_PATTERN)[0]
    im = Interface(devpath)
