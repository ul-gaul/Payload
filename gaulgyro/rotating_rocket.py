import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import os
import serial
import random as rd

os.environ['SDL_VIDEO_WINDOW_POS'] = "17,38"


class RocketModel:
    def __init__(self):
        # self.ser = serial.Serial('COM7', 9600)
        # Aileron geometry
        self.verticies = [(0.3, 0, 1), (0.3, 0, 0), (0.7, 0, 0.1), (0.7, 0, 0.5)]
        self.face = (0, 1, 2, 3)

        # def datagen(self):
        #     lineser = str(self.ser.readline())[2:-5]
        #     try:
        #         t, x, y, z = lineser.split('#')  # bits
        #     except ValueError:
        #         return self.datagen()
        #     return t, x, y, z

    def base(self):
        glColor3b(120, 120, 120)
        cyl = gluNewQuadric()
        gluQuadricNormals(cyl, GLU_SMOOTH)
        gluCylinder(cyl, 0.3, 0.3, 2.5, 50, 5)  # (obj, base radius, top radius, length, res, res)

    def aileron(self):
        glColor3b(115, 0, 0)
        glBegin(GL_QUADS)
        glTexCoord2f(0, 0)
        glVertex3f(self.verticies[self.face[0]][0], self.verticies[self.face[0]][1], self.verticies[self.face[0]][2])
        glTexCoord2f(0, 1)
        glVertex3f(self.verticies[self.face[1]][0], self.verticies[self.face[1]][1], self.verticies[self.face[1]][2])
        glTexCoord2f(1, 1)
        glVertex3f(self.verticies[self.face[2]][0], self.verticies[self.face[2]][1], self.verticies[self.face[2]][2])
        glTexCoord2f(1, 0)
        glVertex3f(self.verticies[self.face[3]][0], self.verticies[self.face[3]][1], self.verticies[self.face[3]][2])
        glEnd()

    def corps(self):
        glColor3b(120, 120, 40)
        cyl = gluNewQuadric()
        gluQuadricNormals(cyl, GLU_SMOOTH)
        gluCylinder(cyl, 0.3, 0.3, 3.5, 50, 5)

    def cone(self):
        glColor3b(115, 0, 0)
        # glColor3b(0, 127, 0)
        con = gluNewQuadric()
        gluQuadricNormals(con, GLU_SMOOTH)
        gluCylinder(con, 0.3, 0, 1.5, 50, 5)

    def pymain(self):
        # ---- Display options
        pygame.init()
        display = (1358, 383)
        pygame.display.set_mode(display, DOUBLEBUF | OPENGL | GL_DEPTH | GL_FRAGMENT_DEPTH | NOFRAME)
        gluPerspective(45, (display[0]/display[1]), 0, 50.0)  # Perspective
        glTranslatef(0, -1, -11)  # Point d'observation
        glRotatef(270, 1, 0, 0)

        glClearDepthf(1.0)
        glEnable(GL_DEPTH_TEST)
        glDepthFunc(GL_LEQUAL)

        direct = 0  # Direction initale
        glRotatef(direct, 0, 1, 0)

        clock = pygame.time.Clock()  # pour FPS

        # ydeg = 10  # init pour random mode
        # pdeg = 0
        # rdeg = 200

        # Running Loop
        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    quit()
            with open('datatmp.txt') as myfile:
                try:
                    t, x, y, z = list(myfile)[-1].split('#')
                except ValueError:
                    pass

            rdeg = int(x)/5  # Degrees / sec
            pdeg = 512/5 - int(y)/5
            ydeg = 512/5 - int(z)/5

            # FPS
            clock.tick()
            fps = clock.get_fps()
            if fps == 0:
                continue

            # ---- Rotation
            # ydeg += rd.randint(-5, 5)  # Degrees / sec
            # pdeg += rd.randint(-5, 5)
            # rdeg += rd.randint(-5, 5)
            glRotatef(ydeg/fps, 0, 1, 0)  # Yaw ---- // Pitch: front flips // Yaw: plan de face
            glRotatef(pdeg/fps, 1, 0, 0)  # Pitch
            glRotatef(rdeg/fps, 0, 0, 1)  # Roll

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

            # ---- Assemblage des pièces
            cm = 2  # Centre de masse. Unités à partir du bas
            glTranslatef(0, 0, -cm)
            self.base()
            self.aileron()
            glRotatef(120, 0, 0, 1)
            self.aileron()
            glRotatef(120, 0, 0, 1)
            self.aileron()
            glRotatef(120, 0, 0, 1)
            glTranslatef(0, 0, 5.5)
            self.cone()
            glTranslatef(0, 0, -3.5)
            self.corps()
            glTranslatef(0, 0, cm-2)

            # print(fps)

            pygame.display.flip()
            pygame.time.wait(10)


RocketModel().pymain()
