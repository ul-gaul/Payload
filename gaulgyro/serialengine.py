import serial
from serial_reader import SerialReader


class SerialEngine:
    def run(self, port):
        # ser = serial.Serial(port, 9600)
        reader = SerialReader()
        reader.start()
        open("datatmp.txt", 'w').close()
        while True:
            packet_list = reader.get_data()
            if len(packet_list) > 0:
                packet = packet_list[-1]
                line = str(int(packet.time_stamp) * 1000)
                line += "#" + str(round(packet.angSpeed_payload_x))
                line += "#" + str(round(packet.angSpeed_payload_y))
                line += "#" + str(round(packet.angSpeed_payload_z)) + "\n"
                # lineser = str(ser.readline())[2:-5] + '\n'
                open("datatmp.txt", "a").writelines(line)

    # def replay(self, replay_filename):
    #     num = 0
    #     open("datatmp.txt", 'w').close()
    #     lines = open(replay_filename).readlines(num)
    #     t_ini = lines[0].split('#')[0]
    #     while True:
    #         line = lines[num]
    #         t = line.split('#')[0]
    #         open("datatmp.txt", "a").writelines(line)  # might have wrong tabbing (in try)
    #         num += 1
    #         # except ValueError:
    #         #     pass


# SerialEngine().replay('data_replay_test2.txt')
SerialEngine().run('COM7')
