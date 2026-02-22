'''
Created on Jul 30, 2018

@author: florian
'''

import sys
import os
import shutil
import subprocess
from PyQt5.Qt import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
import LabyPython.AllConfig_pb2
import LabyPython.watchAndLaunch
from LabyPython.mazeCreator import Ui_MainWindow
from google.protobuf import   json_format  


class AppWindow(QMainWindow):
    
    def callBinary(self, name):
        project_path = "/home/florian/eclipse-workspace/LabyPath"
        my_env = os.environ.copy()
        my_env["LD_LIBRARY_PATH"] = "/opt/lib:" + my_env.get("LD_LIBRARY_PATH", '')
           
        code = subprocess.run([project_path + "/Debug/LabyPath", str(name)], env=my_env, shell=False)
        if not code: 
            print("the process returns " + str(code.returncode))
        return code

    def parseProtoConfig(self, name):
        with open(name, "r") as text_file:
            message = LabyPython.AllConfig_pb2.AllConfig()
            json_format.Parse(text_file.read(), message)
            return message

    def deleteFile(self, getFileName, model, view):
        fileToDelete = getFileName()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                model.remove(view.currentIndex())
        pass
    
    def getSelectedFile(self, model, view):
        if (not view.currentIndex().isValid()):
            view.setCurrentIndex(view.rootIndex().child(0, 0))
        return model.filePath(view.currentIndex())

    def resetPath(self, model, view):
        model.setRootPath(self.project_dir);
        view.setRootIndex(model.index(self.project_dir))    
        
    def editGedit(self, fileToOpen):
        if (fileToOpen):
            subprocess.run(['gedit "' + fileToOpen + '"&'], shell=True)
      
# # Rendering Generation ###########################################""

    def newRenderingGenerationFile(self):
        print ("newRenderingGenerationFile")
        routeConfFile = self.getSelectedRenderConfigFile() 
        if (not routeConfFile):
            QMessageBox.warning(self, 'Render Conf File Absent', "Cannot find Render Conf", QMessageBox.Ok)
        else:
            self.watch.addWork(lambda:self.callGeometryGeneration(routeConfFile, self.parseProtoConfig(routeConfFile).gGraphicRendering.outputfile))

    def deleteRenderingGenerationFile(self):
        fileToDelete = self.getSelectedRenderingGenerationFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelRenderingGeneration.remove(self.viewRenderingGeneration.currentIndex())

    def editInkscapeRenderingGeneration(self):
        fileToOpen = self.getSelectedRenderingGenerationFile()
        self.callInkscape(fileToOpen) 

    def getSelectedRenderingGenerationFile(self):
        if (not self.viewRenderingGeneration.currentIndex().isValid()):
            self.viewRenderingGeneration.setCurrentIndex(self.viewRenderingGeneration.rootIndex().child(0, 0))
        return self.modelRenderingGeneration.filePath(self.viewRenderingGeneration.currentIndex())
    
    def renderingGenerationResetPath(self):
        self.modelRenderingGeneration.setRootPath(self.project_dir);
        self.viewRenderingGeneration.setRootIndex(self.modelRenderingGeneration.index(self.project_dir))
    
    def renderingGenerationSetup(self):
        self.viewRenderingGeneration = self.ui.ResultList;
        self.modelRenderingGeneration = QFileSystemModel(self.viewRenderingGeneration);
        
        self.modelRenderingGeneration.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelRenderingGeneration.setNameFilters(["*render.svg"])
        self.modelRenderingGeneration.setNameFilterDisables(False)
        self.viewRenderingGeneration.setModel(self.modelRenderingGeneration)
        
        self.renderingGenerationResetPath()
         
        newB = self.ui.pushButton_19
        newB.clicked.connect(self.newRenderingGenerationFile)
        
        delB = self.ui.pushButton_20
        delB.clicked.connect(self.deleteRenderingGenerationFile)
    
        editB = self.ui.pushButton_21
        editB.clicked.connect(self.editInkscapeRenderingGeneration)

# # Rendering Configuration ###########################################""
    def getNewRenderConfigName(self, fileOriginal, i):
        return fileOriginal + str(i) + "render.json"
    
    def getNewRenderOutputName(self, fileOriginal, i):
        return fileOriginal + str(i) + "render.svg"
    
    def getSelectedRenderConfigFile(self):
        if (not self.viewRenderConfig.currentIndex().isValid()):
            self.viewRenderConfig.setCurrentIndex(self.viewRenderConfig.rootIndex().child(0, 0))
        return self.modelRenderConfig.filePath(self.viewRenderConfig.currentIndex())
    
    def newRenderConfigFile(self):
        print ("newRenderConfigFile")
        fileOriginal = os.path.basename(self.getSelectedRoutingGenerationFile())
        if (not fileOriginal):
            QMessageBox.warning(self, 'Original File Absent', "Cannot find original svg image", QMessageBox.Ok)
        else:
            fileOriginalPrefix = os.path.splitext(fileOriginal)[0]
            i = 1
            while (self.getNewRenderConfigName(fileOriginalPrefix, i) in os.listdir(self.project_dir)):
                i += 1
            newGridConf = os.path.join(self.project_dir, self.getNewRenderConfigName(fileOriginalPrefix, i))
             
            oldGridConf = self.getSelectedRenderConfigFile()
            allConf = LabyPython.AllConfig_pb2.AllConfig()
            if (oldGridConf):
                try:
                    allConf = self.parseProtoConfig(oldGridConf) 
                except Exception as e:
                    print("There is an Exception during newRenderConfigFile")
                    print(e.__doc__) 
            else:   
                allConf.gGraphicRendering.smoothing_tension = 0.5 
                allConf.gGraphicRendering.smoothing_iterations = 3 
                allConf.gGraphicRendering.translate = 10 
                allConf.gGraphicRendering.color_r = 0 
                allConf.gGraphicRendering.color_g = 0 
                allConf.gGraphicRendering.color_b = 0 
                allConf.gGraphicRendering.smoothing_tension = 1 
                allConf.gGraphicRendering.penConfig.thickness = 0.25 
                
                allConf.gGraphicRendering.penConfig.antisymmetric_amplitude = 0.3 
                allConf.gGraphicRendering.penConfig.antisymmetric_freq = 10 
                allConf.gGraphicRendering.penConfig.antisymmetric_seed = 5 
                allConf.gGraphicRendering.penConfig.symmetric_amplitude = 0.1 
                allConf.gGraphicRendering.penConfig.symmetric_freq = 3 
                allConf.gGraphicRendering.penConfig.symmetric_seed = 8 
                allConf.gGraphicRendering.penConfig.resolution = 1. 
               
            allConf.gGraphicRendering.outputfile = os.path.join(self.project_dir, self.getNewRenderOutputName(fileOriginalPrefix, i));
            allConf.gGraphicRendering.inputfile = os.path.join(self.project_dir, fileOriginal);
            
            jsonObj = json_format.MessageToJson(allConf)
            print(newGridConf + " " + str(jsonObj))
            with open(newGridConf, "w") as text_file:
                print(jsonObj, file=text_file)
        pass
    
    def deleteRenderConfigFile(self):
        fileToDelete = self.getSelectedRenderConfigFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelRenderConfig.remove(self.viewRenderConfig.currentIndex())
        pass
    
    def editGeditRenderConfig(self):
        fileToOpen = self.getSelectedRenderConfigFile()
        if (fileToOpen):
            subprocess.run(['gedit "' + fileToOpen + '"&'], shell=True)
        pass
    
    def renderConfigResetPath(self):
        self.modelRenderConfig.setRootPath(self.project_dir);
        self.viewRenderConfig.setRootIndex(self.modelRenderConfig.index(self.project_dir))
    
    def renderConfigSetup(self):
        self.viewRenderConfig = self.ui.ConfForRenderingList;
        self.modelRenderConfig = QFileSystemModel(self.viewRenderConfig);
        
        self.modelRenderConfig.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelRenderConfig.setNameFilters(["*render.json"])
        self.modelRenderConfig.setNameFilterDisables(False)
        self.viewRenderConfig.setModel(self.modelRenderConfig)
        
        self.renderConfigResetPath()
         
        newB = self.ui.pushButton_16
        newB.clicked.connect(self.newRenderConfigFile)
        
        delB = self.ui.pushButton_17
        delB.clicked.connect(self.deleteRenderConfigFile)
    
        editB = self.ui.pushButton_18
        editB.clicked.connect(self.editGeditRenderConfig)


        
# # Routing Generation  ###########################################""
    
    def newRoutingGenerationFile(self):
        print ("newRoutingGenerationFile")
        routeConfFile = self.getSelectedRouteConfigFile() 
        if (not routeConfFile):
            QMessageBox.warning(self, 'Route AltRouteFile Absent', "Cannot find Route Conf", QMessageBox.Ok)
        else:
            self.watch.addWork(lambda:self.callGeometryGeneration(routeConfFile, self.parseProtoConfig(routeConfFile).routing.filepaths.outputfile))

    def deleteRoutingGenerationFile(self):
        fileToDelete = self.getSelectedRoutingGenerationFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelRoutingGeneration.remove(self.viewRoutingGeneration.currentIndex())

    def editInkscapeRoutingGeneration(self):
        fileToOpen = self.getSelectedRoutingGenerationFile()
        self.callInkscape(fileToOpen) 

    def getSelectedRoutingGenerationFile(self):
        if (not self.viewRoutingGeneration.currentIndex().isValid()):
            self.viewRoutingGeneration.setCurrentIndex(self.viewRoutingGeneration.rootIndex().child(0, 0))
        return self.modelRoutingGeneration.filePath(self.viewRoutingGeneration.currentIndex())
    
    def routingGenerationResetPath(self):
        self.modelRoutingGeneration.setRootPath(self.project_dir);
        self.viewRoutingGeneration.setRootIndex(self.modelRoutingGeneration.index(self.project_dir))
    
    def routingGenerationSetup(self):
        self.viewRoutingGeneration = self.ui.RoutingResultList;
        self.modelRoutingGeneration = QFileSystemModel(self.viewRoutingGeneration);
        
        self.modelRoutingGeneration.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelRoutingGeneration.setNameFilters(["*route.svg"])
        self.modelRoutingGeneration.setNameFilterDisables(False)
        self.viewRoutingGeneration.setModel(self.modelRoutingGeneration)
        
        self.routingGenerationResetPath()
         
        newB = self.ui.pushButton_13
        newB.clicked.connect(self.newRoutingGenerationFile)
        
        delB = self.ui.pushButton_14
        delB.clicked.connect(self.deleteRoutingGenerationFile)
    
        editB = self.ui.pushButton_15
        editB.clicked.connect(self.editInkscapeRoutingGeneration)
       
# # Routing Configuration  ###########################################""
    def getNewRouteConfigName(self, fileOriginal, i):
        return fileOriginal + str(i) + "route.json"
    
    def getNewRouteOutputName(self, fileOriginal, i):
        return fileOriginal + str(i) + "route.svg"
    
    def getSelectedRouteConfigFile(self):
        if (not self.viewRouteConfig.currentIndex().isValid()):
            self.viewRouteConfig.setCurrentIndex(self.viewRouteConfig.rootIndex().child(0, 0))
        return self.modelRouteConfig.filePath(self.viewRouteConfig.currentIndex())
    
    def newRouteConfigFile(self):
        print ("newRouteConfigFile")
        fileOriginal = os.path.basename(self.getSelectedGridGenerationFile())
        if (not fileOriginal):
            QMessageBox.warning(self, 'Original File Absent', "Cannot find original svg image", QMessageBox.Ok)
        else:
            fileOriginalPrefix = os.path.splitext(fileOriginal)[0]
            i = 1
            while (self.getNewRouteConfigName(fileOriginalPrefix, i) in os.listdir(self.project_dir)):
                i += 1
            newGridConf = os.path.join(self.project_dir, self.getNewRouteConfigName(fileOriginalPrefix, i))
             
            oldGridConf = self.getSelectedRouteConfigFile()
            allConf = LabyPython.AllConfig_pb2.AllConfig()
            if (oldGridConf):
                try:
                    allConf = self.parseProtoConfig(oldGridConf) 
                except Exception as e:
                    print("There is an Exception during newRouteConfigFile")
                    print(e.__doc__) 
            else:           
                allConf.routing.placement.cell.seed = 1;
                allConf.routing.placement.cell.maxPin = 400;
                allConf.routing.placement.cell.startNet = 30;
                allConf.routing.placement.initial_thickness = 1.8;
                allConf.routing.placement.decrement_factor = 1.5;
                allConf.routing.placement.minimal_thickness = 0.5;
                allConf.routing.placement.smoothing_tension = 1;
                allConf.routing.placement.smoothing_iteration = 3;
                allConf.routing.placement.max_routing_attempt = 300;
                allConf.routing.placement.routing.seed = 5;
                allConf.routing.placement.routing.max_random = 300;
                allConf.routing.placement.routing.distance_unit_cost = 1;
                allConf.routing.placement.routing.via_unit_cost = 10;
               
            allConf.routing.filepaths.outputfile = os.path.join(self.project_dir, self.getNewRouteOutputName(fileOriginalPrefix, i));
            allConf.routing.filepaths.inputfile = os.path.join(self.project_dir, fileOriginal);
            
            jsonObj = json_format.MessageToJson(allConf)
            print(newGridConf + " " + str(jsonObj))
            with open(newGridConf, "w") as text_file:
                print(jsonObj, file=text_file)
        pass
    
    def deleteRouteConfigFile(self):
        fileToDelete = self.getSelectedRouteConfigFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelRouteConfig.remove(self.viewRouteConfig.currentIndex())
        pass
    
    def editGeditRouteConfig(self):
        fileToOpen = self.getSelectedRouteConfigFile()
        if (fileToOpen):
            subprocess.run(['gedit "' + fileToOpen + '"&'], shell=True)
        pass
    
    def routeConfigResetPath(self):
        self.modelRouteConfig.setRootPath(self.project_dir);
        self.viewRouteConfig.setRootIndex(self.modelRouteConfig.index(self.project_dir))
    
    def routeConfigSetup(self):
        self.viewRouteConfig = self.ui.ConfForRoutingList;
        self.modelRouteConfig = QFileSystemModel(self.viewRouteConfig);
        
        self.modelRouteConfig.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelRouteConfig.setNameFilters(["*route.json"])
        self.modelRouteConfig.setNameFilterDisables(False)
        self.viewRouteConfig.setModel(self.modelRouteConfig)
        
        self.routeConfigResetPath()
         
        newB = self.ui.pushButton_10
        newB.clicked.connect(self.newRouteConfigFile)
        
        delB = self.ui.pushButton_11
        delB.clicked.connect(self.deleteRouteConfigFile)
    
        editB = self.ui.pushButton_12
        editB.clicked.connect(self.editGeditRouteConfig)

# # Grid Generation ###########################################""
    
    def callGeometryGeneration(self, name, outputfile):
        if self.callBinary(name):
            self.callInkscape(outputfile) 
    
    def newGridGenerationFile(self):
        print ("newGridGenerationFile")
        gridConfFile = self.getSelectedGridFile() 
        if (not gridConfFile):
            QMessageBox.warning(self, 'Grid Conf File Absent', "Cannot find Grid Conf", QMessageBox.Ok)
        else:
            self.watch.addWork(lambda:self.callGeometryGeneration(gridConfFile, self.parseProtoConfig(gridConfFile).skeletonGrid.outputfile))
            pass

    def deleteGridGenerationFile(self):
        fileToDelete = self.getSelectedGridGenerationFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelGridGeneration.remove(self.viewGridGeneration.currentIndex())
        pass
        pass

    def callInkscape(self, fileToOpen):
        if (fileToOpen):
            subprocess.run(['inkscape "' + fileToOpen + '" 2>/dev/null&'], shell=True)

    def editInkscapeGridGeneration(self):
        fileToOpen = self.getSelectedGridGenerationFile()
        self.callInkscape(fileToOpen) 

    def getSelectedGridGenerationFile(self):
        if (not self.viewGridGeneration.currentIndex().isValid()):
            self.viewGridGeneration.setCurrentIndex(self.viewGridGeneration.rootIndex().child(0, 0))
        return self.modelGridGeneration.filePath(self.viewGridGeneration.currentIndex())
    
    def gridGenerationResetPath(self):
        self.modelGridGeneration.setRootPath(self.project_dir);
        self.viewGridGeneration.setRootIndex(self.modelGridGeneration.index(self.project_dir))
    
    def gridGenerationSetup(self):
        self.viewGridGeneration = self.ui.GridList;
        self.modelGridGeneration = QFileSystemModel(self.viewGridGeneration);
        
        self.modelGridGeneration.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelGridGeneration.setNameFilters(["*grid.svg"])
        self.modelGridGeneration.setNameFilterDisables(False)
        self.viewGridGeneration.setModel(self.modelGridGeneration)
        
        self.gridGenerationResetPath()
         
        newB = self.ui.pushButton_7
        newB.clicked.connect(self.newGridGenerationFile)
        
        delB = self.ui.pushButton_8
        delB.clicked.connect(self.deleteGridGenerationFile)
    
        editB = self.ui.pushButton_9
        editB.clicked.connect(self.editInkscapeGridGeneration)
    
## Grid Configuration########################################################
    
    def getNewGridName(self, fileOriginal, i):
        return fileOriginal + str(i) + "grid.json"
    
    def getNewGridOutputName(self, fileOriginal, i):
        return fileOriginal + str(i) + "grid.svg"
    
    def getSelectedGridFile(self):
        if (not self.viewGridConfig.currentIndex().isValid()):
            self.viewGridConfig.setCurrentIndex(self.viewGridConfig.rootIndex().child(0, 0))
        return self.modelGridConfig.filePath(self.viewGridConfig.currentIndex())
    
    def newGridConfigFile(self):
        print ("newGridConfigFile")
        fileOriginal = os.path.basename(self.getSelectedOriginalFile())
        if (not fileOriginal):
            QMessageBox.warning(self, 'Original File Absent', "Cannot find original svg image", QMessageBox.Ok)
        else:
            fileOriginalPrefix = os.path.splitext(fileOriginal)[0]
            i = 1
            while (self.getNewGridName(fileOriginalPrefix, i) in os.listdir(self.project_dir)):
                i += 1
            newGridConf = os.path.join(self.project_dir, self.getNewGridName(fileOriginalPrefix, i))
             
            oldGridConf = self.getSelectedGridFile()
            allConf = LabyPython.AllConfig_pb2.AllConfig()
            if (oldGridConf):
                try:
                    allConf = self.parseProtoConfig(oldGridConf) 
                except Exception as e:
                    print("There is an Exception during newGridConfigFile")
                    print(e.__doc__) 
            else:           
                allConf.skeletonGrid.simplificationOfOriginalSVG = 0.1;
                allConf.skeletonGrid.max_sep = 5;
                allConf.skeletonGrid.min_sep = 0.1; 
                allConf.skeletonGrid.seed = 3; 
               
            allConf.skeletonGrid.outputfile = os.path.join(self.project_dir, self.getNewGridOutputName(fileOriginalPrefix, i));
            allConf.skeletonGrid.inputfile = os.path.join(self.project_dir, fileOriginal);
            
            jsonObj = json_format.MessageToJson(allConf)
            print(newGridConf + " " + str(jsonObj))
            with open(newGridConf, "w") as text_file:
                print(jsonObj, file=text_file)
        pass
    
    def deleteGridConfigFile(self):
        fileToDelete = self.getSelectedGridFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelGridConfig.remove(self.viewGridConfig.currentIndex())
        pass
    
    def editGeditGridConfig(self):
        fileToOpen = self.getSelectedGridFile()
        if (fileToOpen):
            subprocess.run(['gedit "' + fileToOpen + '"&'], shell=True)
        pass
    
    def gridConfigResetPath(self):
        self.modelGridConfig.setRootPath(self.project_dir);
        self.viewGridConfig.setRootIndex(self.modelGridConfig.index(self.project_dir))
    
    def gridConfigSetup(self):
        self.viewGridConfig = self.ui.gridConfigList;
        self.modelGridConfig = QFileSystemModel(self.viewGridConfig);
        
        self.modelGridConfig.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelGridConfig.setNameFilters(["*grid.json"])
        self.modelGridConfig.setNameFilterDisables(False)
        self.viewGridConfig.setModel(self.modelGridConfig)
        
        self.gridConfigResetPath()
         
        newB = self.ui.pushButton_4
        newB.clicked.connect(self.newGridConfigFile)
        
        delB = self.ui.pushButton_5
        delB.clicked.connect(self.deleteGridConfigFile)
    
        editB = self.ui.pushButton_6
        editB.clicked.connect(self.editGeditGridConfig)
   
# # selection of Original Image###############################################################" 
 
    def newOriginalFile(self):
        filenames = QFileDialog.getOpenFileName(self, caption='Open file' , filter="SVG (*.svg);;All Files (*)", directory=self.project_dir)
          
        for fileName in filenames:
            print (fileName)
            if fileName and os.path.exists(fileName):
                try:
                    fileOriginalPrefix = os.path.splitext(os.path.basename(fileName))[0] + "orig.svg"
                    newpath = os.path.join(self.project_dir, fileOriginalPrefix)
                    print(newpath)
                    shutil.copyfile(fileName, newpath)
                    print("copy ok")
                except Exception as e:
                    print("There is an Exception during newOriginalFile")
                    print(e.__doc__) 
      
        pass
    
    def deleteOriginalFile(self):
        fileToDelete = self.getSelectedOriginalFile()
        if (fileToDelete):
            msg = QMessageBox.warning(self, 'File deletion', "Do you want to remove this file ?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if msg == QMessageBox.Yes:
                self.modelOriginal.remove(self.viewOriginal.currentIndex())
    
    def editInkscapeOriginal(self):
        fileToOpen = self.getSelectedOriginalFile()
        self.callInkscape(fileToOpen) 
    
    def originalResetPath(self):
        self.modelOriginal.setRootPath(self.project_dir);
        self.viewOriginal.setRootIndex(self.modelOriginal.index(self.project_dir))
    
    def getSelectedOriginalFile(self):
        if (not self.viewOriginal.currentIndex().isValid()):
            self.viewOriginal.setCurrentIndex(self.viewOriginal.rootIndex().child(0, 0))
        return self.modelOriginal.filePath (self.viewOriginal.currentIndex())
        
    def originalImageSetup(self):
        self.viewOriginal = self.ui.OriginalImageList;
        self.modelOriginal = QFileSystemModel(self.viewOriginal);
        
        self.modelOriginal.setFilter(QDir.NoDotAndDotDot | QDir.Files)
        
        self.modelOriginal.setNameFilters(["*orig.svg"])
        self.modelOriginal.setNameFilterDisables(False)
        self.viewOriginal.setModel(self.modelOriginal)
        self.originalResetPath()
         
        newB = self.ui.pushButton_2
        newB.clicked.connect(self.newOriginalFile)
        
        delB = self.ui.pushButton_3
        delB.clicked.connect(self.deleteOriginalFile)
    
        editB = self.ui.pushButton
        editB.clicked.connect(self.editInkscapeOriginal)
        
    def createNewProject(self):
        filename = QFileDialog.getSaveFileName(self, caption='New Project' , filter="All Files (*)", directory=self.project_dir)
       
        if filename[0]: 
            self.project_dir = os.path.dirname(os.path.abspath(filename[0]))
            self.originalResetPath()
            self.gridConfigResetPath()
            self.gridGenerationResetPath()
            self.routeConfigResetPath()
            self.routingGenerationResetPath()
            self.renderConfigResetPath()
            self.renderingGenerationResetPath()
        
        
    
    def projectSetup(self):
        self.viewproject = self.ui.actionNew_Project;
        self.viewproject.triggered.connect(self.createNewProject)
        self.project_dir = os.path.abspath("/mnt/3C32BE5232BE10BE/creation/creation image/natureFraicheLaby/python")
        
    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.projectSetup()
        self.originalImageSetup()
        self.gridConfigSetup()
        self.gridGenerationSetup()
        self.routeConfigSetup()
        self.routingGenerationSetup()
        self.renderConfigSetup()
        self.renderingGenerationSetup()
        self.watch = LabyPython.watchAndLaunch.Watcher()
     

if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = AppWindow()
    w.show()
    sys.exit(app.exec_())
