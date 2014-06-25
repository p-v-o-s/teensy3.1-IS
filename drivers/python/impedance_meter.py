import glob
import serial

DEVPATH_PATTERN = "/dev/ttyACM*"
SERIAL_BAUDRATE = 115200
DEFAULT_EOL = "\n"
DEFAULT_DEBUG = False
DAC_MIN_INTERVAL = 4e-6
DAC_MAX_SAMPNUM = 4096

class Interface(object):
    def __init__(self, serial_devpath, debug = DEFAULT_DEBUG):
        self._ser = serial.Serial(serial_devpath, baudrate=SERIAL_BAUDRATE)
        self._EOL = DEFAULT_EOL
        self._debug = debug

    def synthesize(self,
                   freq,
                   amp = 3.3,
                   sampnum = None,
                   shape = 'sin'
                  ):
        if sampnum is None:
            sampnum = min(int(1.0/(freq*DAC_MIN_INTERVAL)), DAC_MAX_SAMPNUM)
        dt = 1.0/(freq*sampnum)
        if dt < DAC_MIN_INTERVAL:
            raise ValueError("The DAC interval cannot be < %0.2e seconds, must reduce 'sampnum' for this 'freq' value" % (DAC_MINIMAL_INTERVAL,))
        cmd = "SYNTH.FREQ %f" % freq
        self._send(cmd)
        cmd = "SYNTH.AMP %f" % amp
        self._send(cmd)
        cmd = "SYNTH.SAMPNUM %d" % sampnum
        self._send(cmd)
        cmd = "SYNTH.START"
        self._send(cmd)

    def get_synth_state(self):
        info = {}
        info['freq']       = float(self._exchange("SYNTH.FREQ"))
        info['amp']        = float(self._exchange("SYNTH.AMP"))
        info['sampnum']    = int(self._exchange("SYNTH.SAMPNUM"))
        info['interval']   = float(self._exchange("SYNTH.INTERVAL"))
        info['is_running'] = bool(int((self._exchange("SYNTH.IS_RUNNING"))))
        return info

    def stop_synth(self):
        cmd = "SYNTH.STOP"
        self._send(cmd)

    def _send(self, cmd):
        if self._debug:
            print "---> %s" % cmd
        self._ser.write(cmd)
        self._ser.write(self._EOL)

    def _read(self, strip_EOL = True):
        resp = self._ser.readline()
        if strip_EOL:
            resp = resp.rstrip(self._EOL+'\r\n') #make strip comprehensive
        if self._debug:
            print "<-- " + resp
        return resp
        
    def _read_char(self):
        c = self._ser.read(1)
        if self._debug:
            print "<-- char: " + c
        return c

    def _exchange(self, cmd):
        "Relay a command and get the response"
        #remove any possible crud in the buffer
        self._ser.flushInput()         
        self._send(cmd)     #send the command
        resp = self._read()     #read the response
        return resp

################################################################################
# TEST CODE
################################################################################
if __name__ == "__main__":
    devpath = glob.glob(DEVPATH_PATTERN)[0]
    im = Interface(devpath, debug=True)
