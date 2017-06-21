import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QMessageBox
from gyrogui import Ui_MainWindow

sys._excepthook = sys.excepthook


def my_exception_hook(exctype, value, traceback):
    print(exctype, value, traceback)
    sys.exit(1)


class AppWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        # self.ui.actionExit.triggered.connect(self.close)
        self.show()

    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Quit', "Are you sure you want to quit?", QMessageBox.Yes, QMessageBox.Cancel)
        if reply == QMessageBox.Yes:
            event.accept()
        else:
            event.ignore()


app = QApplication(sys.argv)
w = AppWindow()
w.showMaximized()
sys.exit(app.exec_())
