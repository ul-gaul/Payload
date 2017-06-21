import glob
import sys
import struct
import serial
from threading import Thread

from Producer import Producer
from rocket_packet import RocketPacket
# from csv_file_writer import CsvFileWriter


class SerialReader(Producer):
    def __init__(self, save_file_path=None, baudrate=57600, start_character=b's', sampling_frequency=1):
        super().__init__()
        self.save_file_path = save_file_path
        self.port = serial.Serial()
        self.port.baudrate = baudrate
        self.port.timeout = sampling_frequency
        self.start_character = start_character

        # RocketPacket data + 1 byte for checksum
        self.num_bytes_to_read = RocketPacket.size_in_bytes + 1
        self.format = RocketPacket.format + "B"

        self.flightData = []
        self.thread = Thread(target=self.run)

    def start(self):
        self.port.port = self.detect_serial()[0]
        self.port.open()
        self.is_running = True
        self.thread.start()

    def run(self):
        while self.is_running:
            c = self.port.read(1)
            if c == self.start_character:
                data_array = self.port.read(self.num_bytes_to_read)
                try:
                    data_list = struct.unpack(self.format, data_array)

                    if self.validate_checksum(data_array):
                        rocket_packet = RocketPacket(data_list[:-1])
                        self.rocket_packets.put(rocket_packet)
                        self.flightData.append(rocket_packet)
                except struct.error:
                    """
                    This error can occur if we don't read enough bytes on the serial port or if the packet format is
                    incorrect.
                    """
                    print("Invalid packet")
        self.port.close()

    def stop(self):
        super().stop()
        # if self.save_file_path is not None:
        #     csv_file_writer = CsvFileWriter(self.save_file_path, self.flightData)
        #     csv_file_writer.save()

    @staticmethod
    def detect_serial():
        """ Lists serial port names
        :raises EnvironmentError
            On unsopported or unknown platforms
        :returns:
            A list of the serial ports available on the system
        """

        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
            except (OSError, serial.SerialException):
                pass
        return result

    @staticmethod
    def validate_checksum(data_array):
        checksum = sum(data_array) % 256
        if checksum == 255:
            return True
        else:
            print("Invalid Checksum : expected = 255, calculated = {}".format(checksum))
            return False
