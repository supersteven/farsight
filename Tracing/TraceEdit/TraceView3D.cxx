/*=========================================================================
Copyright 2009 Rensselaer Polytechnic Institute
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
=========================================================================*/

/*
Farsight ToolKit 3D Viewer: 
v1: implements render window functionality
render window creation in "View3D::RenderWin"
line objects mapped and actors created in "View3D::LineAct"
actor added and window created in "View3D::addAct"
v2: add picking functionality:
access to observing a pick event is in "View3D::interact"
the interacter calls for "View3D::PickPoint"
v3: include contourFilter and rayCast renderers
v4: converted to Qt, member names changed to fit "VTK style" more closely.
v5: automated functions implemented structure in place for "PACE".
v6: "ALISA" implemented
v7: new GUI and file control
*/

#include "TraceView3D.h"


View3D::View3D(QWidget *parent)
: QMainWindow(parent)
{
	this->QVTK = 0;
	this->GapsPlotView = NULL;
	this->TreePlot = NULL;
	this->FTKTable = NULL;
	this->FL_MeasurePlot = NULL;
	this->FL_histo = NULL;
	this->FL_MeasureTable = NULL;
	this->GapsTableView = NULL;
	this->TreeModel = NULL;
	this->savescreenshotDialog = NULL;
	this->ROIExtrudedpolydata = NULL;
	this->ROIactor = NULL;

	this->tobj = new TraceObject;
	//int num_loaded = 0;
	this->numDeleted = 0;
	this->numMerged = 0;
	this->numSplit = 0;
	this->flag=0;
	//this->Volume=0;
	//bool tracesLoaded = false;
	this->translateImages = false;	//this is for testing a switch is needed
	this->viewIn2D = this->TraceEditSettings.value("mainWin/use2d",false).toBool();
	this->renderTraceBits = false;
	this->projectFilesTableCreated = false;
	this->SlicerBarCreated = false;
	this->projectionStyle = 0;	//should be maximum projection
	this->projection_axis = 2; // z projection
	this->Date.currentDate();
	this->Time.currentTime();
	this->ProjectName.clear();
	this->Image.clear();
	this->TraceFiles.clear();
	this->SomaFile.clear();
	this->NucleiFile.clear();
	this->nucleiTable = vtkSmartPointer<vtkTable>::New();
	this->tempTraceFile.clear();
	this->backColorR = this->TraceEditSettings.value("mainWin/ColorR", .6).toDouble() ;
	this->backColorG = this->TraceEditSettings.value("mainWin/ColorG", .6).toDouble() ;
	this->backColorB = this->TraceEditSettings.value("mainWin/ColorB", .6).toDouble() ;
	this->ImageActors = new ImageRenderActors();
	this->EditLogDisplay = new QTextEdit();
	this->EditLogDisplay->setReadOnly(true);
	this->EditLogDisplay->setLineWrapMode(QTextEdit::NoWrap);
	this->EditLogDisplay->append("Farsight Trace Editor Started at: \nDate: \t" + this->Date.currentDate().toString( "ddd MMMM d yy" ) );
	this->EditLogDisplay->append("Time: \t" + this->Time.currentTime().toString( "h:m:s ap" ) );
	this->InformationDisplays = new QDockWidget("Edit Log Information", this);
	this->InformationDisplays->setWidget(this->EditLogDisplay);
	this->addDockWidget(Qt::LeftDockWidgetArea, this->InformationDisplays);

	//Set up the main window's central widget
	this->CentralWidget = new QWidget(this);
	this->setCentralWidget(this->CentralWidget);
	this->CreateGUIObjects();
	this->CreateLayout();
	this->CreateBootLoader();
	QStringList args = QCoreApplication::arguments();
	// load as many files as possible. Provide offset for differentiating types
	for(int counter=1; counter<args.size(); counter++)
	{
		QString nextFile = args[counter];
		if (nextFile.endsWith("swc"))
		{
			this->TraceFiles.append( nextFile);
			this->EditLogDisplay->append("Trace file: \t" + nextFile);
			this->tobj->ReadFromSWCFile((char*)nextFile.toStdString().c_str());
		}
		else if(nextFile.endsWith("xml",Qt::CaseInsensitive))
		{
			this->EditLogDisplay->append("Trace file: \t" + nextFile);
			this->TraceFiles.append( nextFile);
			this->tobj->ReadFromRPIXMLFile((char*)nextFile.toStdString().c_str());
		}
		else if (nextFile.endsWith("vtk"))
		{
			this->EditLogDisplay->append("Trace file: \t" + nextFile);
			this->TraceFiles.append( nextFile);
			this->tobj->ReadFromVTKFile((char*)nextFile.toStdString().c_str());
		}
		else if (nextFile.endsWith("tiff")||nextFile.endsWith("tif")||nextFile.endsWith("pic",Qt::CaseInsensitive))
		{
			this->Image.append( nextFile);
			this->EditLogDisplay->append("Image file: \t" + nextFile);
			this->ImageActors->loadImage(nextFile.toStdString(), "Image");
		}
	}//end of arg 
	if(!this->TraceFiles.isEmpty() || !this->Image.isEmpty() || !this->SomaFile.isEmpty())
	{
		this->OkToBoot();
	}
}

View3D::View3D(TraceObject *Traces)
{
	this->QVTK = 0;
	this->GapsPlotView = NULL;
	this->TreePlot = NULL;
	this->FTKTable = NULL;
	this->FL_MeasurePlot = NULL;
	this->FL_histo = NULL;
	this->FL_MeasureTable = NULL;
	this->GapsTableView = NULL;

	this->tobj = Traces;
	//	this->Volume=0;
	this->Initialize();
	this->ShowTreeData();
	this->statusBar()->showMessage(tr("Trace Editor Started"));
}

View3D::~View3D()
{
	if(this->QVTK)
	{
		delete this->QVTK;
	}
	if(this->GapsPlotView)
	{
		delete this->GapsPlotView;
	}
	if(this->FTKTable)
	{
		delete this->FTKTable;
	}
	if(this->TreePlot)
	{
		delete this->TreePlot;
	}
	if(this->FL_MeasurePlot)
	{
		delete this->FL_MeasurePlot;
	}
	if(this->FL_histo)
	{
		delete this->FL_histo;
	}
	if(this->FL_MeasureTable)
	{
		delete this->FL_MeasureTable;
	}
	if(this->GapsTableView)
	{
		delete this->GapsTableView;
	}
	delete this->tobj;
	delete this->ImageActors;
}
/*! determine if you can start trace edit*/
void View3D::CreateBootLoader()
{
	// Create a window that allows files to be loaded
	this->bootLoadFiles = new QWidget;
	this->BootTrace = new QPushButton("Trace",this->bootLoadFiles);
	connect(this->BootTrace, SIGNAL(clicked()), this, SLOT(getTraceFile()));
	this->BootImage = new QPushButton("Image",this->bootLoadFiles);
	connect(this->BootImage, SIGNAL(clicked()), this, SLOT(getImageFile()));
	this->BootSoma = new QPushButton("Somas", this->bootLoadFiles);
	connect(this->BootSoma, SIGNAL(clicked()), this, SLOT(getSomaFile()));
	this->GetAUserName =  new QComboBox(this->bootLoadFiles);
	this->GetAUserName->setEditable(true);
	this->GetAUserName->setInsertPolicy(QComboBox::InsertAtCurrent);
	this->GetAUserName->addItems(this->TraceEditSettings.value("boot/userName", "default user").toStringList());
	this->GetLab = new QComboBox(this->bootLoadFiles);
	this->GetLab->setEditable(true);
	this->GetLab->setInsertPolicy(QComboBox::InsertAtCurrent);
	this->GetLab->addItems(this->TraceEditSettings.value("boot/LabName", "Roysam Lab").toStringList());
	/*this->scale = new QDoubleSpinBox(this->bootLoadFiles);
	this->scale->setValue(this->TraceEditSettings.value("boot/scale", 1).toDouble());
	this->scale->setSingleStep(.01);*/
	this->okBoot = new QPushButton("Start",this->bootLoadFiles);
	connect(this->okBoot, SIGNAL(clicked()), this, SLOT(OkToBoot()));
	this->Reload = new QPushButton("Reload", this->bootLoadFiles);
	connect(this->Reload, SIGNAL(clicked()), this, SLOT(ReloadState()));
	this->BootProject = new QPushButton("Project", this->bootLoadFiles);
	connect(this->BootProject, SIGNAL(clicked()), this, SLOT(LoadProject()));
	this->Use2DSlicer = new QCheckBox;
	this->Use2DSlicer->setChecked(this->viewIn2D);
	QFormLayout *LoadLayout = new QFormLayout(this->bootLoadFiles);
	LoadLayout->addRow(tr("User Name "), this->GetAUserName);
	LoadLayout->addRow(tr("Lab Name "), this->GetLab);
	LoadLayout->addRow(tr("Trace Files"), this->BootTrace);
	LoadLayout->addRow(tr("Image File"), this->BootImage);
	LoadLayout->addRow(tr("Default Use Slicer"), this->Use2DSlicer);
	LoadLayout->addRow(tr("Somas File"), this->BootSoma);
	//LoadLayout->addRow(tr("uM Per Voxel"), this->scale);
	LoadLayout->addRow(tr("Reload Previous Session"), this->Reload);
	LoadLayout->addRow(tr("Project"), this->BootProject);
	LoadLayout->addRow(tr("Run Trace Editor"), this->okBoot);
	this->BootDock = new QDockWidget(tr("Start Trace Editor"), this);
	this->BootDock->setAllowedAreas(Qt::LeftDockWidgetArea |
		Qt::RightDockWidgetArea);
	this->BootDock->setWidget(this->bootLoadFiles);
	addDockWidget(Qt::RightDockWidgetArea, this->BootDock);
	/*this->bootLoadFiles->show();
	this->bootLoadFiles->move(this->TraceEditSettings.value("boot/pos",QPoint(40, 59)).toPoint());*/
}

void View3D::ReloadState()
{
	int i;
	this->ProjectName = this->TraceEditSettings.value("lastOpen/Project").toString();
	if (!this->ProjectName.isEmpty())
	{
		this->readProject(this->ProjectName);
	}
	else
	{
		this->Image = this->TraceEditSettings.value("lastOpen/Image").toStringList();
		if(!this->Image.isEmpty()) 
		{
			for (i = 0; i < this->Image.size(); i ++)
			{
				this->ImageActors->loadImage(this->Image.at(i).toStdString(),"Image");
			}
		}

		this->tempTraceFile = this->TraceEditSettings.value("lastOpen/Temp").toStringList();
		if (!this->tempTraceFile.isEmpty())
		{	//opens last saved file 
			bool ok = false; 
			QString temp = QInputDialog::getItem(this, "Reload Saved Version", "open which saved version?", 
				this->tempTraceFile, 0, false, &ok);
			if (ok && !temp.isEmpty())
			{
				QFileInfo tempInfo(temp);
				if (tempInfo.exists())
				{
					this->TraceFiles.append(temp);
				}
				else
				{
					QMessageBox::warning(this, "Error Reading File", QString("Could not open file %1 for reading").arg(temp),QMessageBox::Ok, QMessageBox::Ok);
					//error could not open
				}
			}
			else
			{
				this->TraceFiles.append( this->tempTraceFile.last());
			}
			// if saved file find log file
			QString logFileName = this->TraceEditSettings.value("lastOpen/Log").toString();
			if (!logFileName.isEmpty())
			{	//read and add old log file to new one
				QFile readLog(logFileName);
				if (readLog.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QTextStream in(&readLog);
					this->EditLogDisplay->append(in.readAll());
				}
			}//end of has log
		}
		else
		{	//if the last file was never saved open this
			this->TraceFiles = this->TraceEditSettings.value("lastOpen/Trace").toStringList();
		}
		if(!this->TraceFiles.isEmpty()) 
		{		
			for (i = 0; i < this->TraceFiles.size(); i++)
			{

				std::string traceFile = this->TraceFiles.at(i).toStdString();
				if(this->TraceFiles.at(i).endsWith("swc"))
				{
					this->tobj->ReadFromSWCFile((char*)traceFile.c_str());
				}
				else if(this->TraceFiles.at(i).endsWith("xml"))
				{
					this->tobj->ReadFromRPIXMLFile((char*)traceFile.c_str());
				}
				else if (this->TraceFiles.at(i).endsWith("vtk"))
				{
					this->tobj->ReadFromVTKFile((char*)traceFile.c_str());
				}
			}//end of sending trace file to proper reader
		}// end of trace files
		this->SomaFile = this->TraceEditSettings.value("lastOpen/Soma").toStringList();
		if(!this->SomaFile.isEmpty()) 
		{
			for (i = 0; i < this->SomaFile.size(); i++)
			{
				this->ImageActors->loadImage(this->SomaFile.at(i).toStdString(), "Soma");
			}
		}
	}//end else projectfile.isempty
	this->OkToBoot();
}

void View3D::OkToBoot()
{
	if(!this->TraceFiles.isEmpty() || !this->Image.isEmpty() || !this->SomaFile.isEmpty())
	{
		this->BootDock->hide();
		this->InformationDisplays->hide();
		this->menuBar()->show();
		this->EditsToolBar->show();
		this->BranchToolBar->show();
		this->viewIn2D = this->Use2DSlicer->isChecked();

		this->resize(this->TraceEditSettings.value("mainWin/size",QSize(850, 480)).toSize());
		this->move(this->TraceEditSettings.value("mainWin/pos",QPoint(40, 59)).toPoint());
		//int i = 0;
		/*this->uMperVoxel = this->scale->value();
		this->TraceEditSettings.setValue("boot/scale", this->uMperVoxel);*/
		this->Initialize();
		this->UserName = this->GetAUserName->currentText();
		QStringList allUsers = this->TraceEditSettings.value("boot/userName").toStringList();
		allUsers.removeAll(this->UserName);
		allUsers.prepend(this->UserName);
		this->TraceEditSettings.setValue("boot/userName", allUsers);
		this->EditLogDisplay->append("User: \t" + this->UserName);

		this->LabName = this->GetLab->currentText();
		QStringList allLabs = this->TraceEditSettings.value("boot/LabName").toStringList();
		allLabs.removeAll(this->LabName);
		allLabs.prepend(this->LabName);
		this->TraceEditSettings.setValue("boot/LabName", allLabs);
		this->EditLogDisplay->append("Lab: \t" + this->LabName);
		if (!this->TraceFiles.isEmpty())
		{
			this->ShowTreeData();
			this->cursor3DDock->show();
		}
		this->Rerender();
		if (this->tobj->BranchPoints.size() >1)
		{
			QMessageBox::critical(this,"Branching Incomplete" ,
				"You have traces without defined roots. \nPlease Use the 'Set Root' command",
				QMessageBox::Ok, QMessageBox::Ok);
		}
	}
	else
	{
		QMessageBox *bootFailed = new QMessageBox;
		bootFailed->setText("There are no files to open. Please select a file to continue.");
		bootFailed->show();
		return;
	}
	if (viewIn2D == true)
	{
		this->RaycastBar->toggleViewAction()->setDisabled(1);
		this->chooseInteractorStyle(1);
		renderMode = PROJECTION;
	}
	else
	{
		this->chooseInteractorStyle(0);
		renderMode = RAYCAST;
	}
}

//!Dialogs to Find File Names
QString View3D::getSomaFile()
{
	QString somaDir = this->TraceEditSettings.value("somaDir", ".").toString();
	QString somaFiles = QFileDialog::getOpenFileName(this , "Choose a Soma file to load", somaDir, 
		tr("Image File ( *.tiff *.tif *.pic *.PIC ) "));
	if(!somaFiles.isEmpty())
	{
		somaDir = QFileInfo(somaFiles).absolutePath();
		this->TraceEditSettings.setValue("somaDir", somaDir);
		this->EditLogDisplay->append("Soma file: \t" + somaFiles.section('/',-1));
		this->SomaFile.append( somaFiles);
		this->ImageActors->loadImage(somaFiles.toStdString(), "Soma");
	}
	return somaFiles.section('/',-1);
}

QString View3D::getTraceFile()
{
	std::string traceFile;
	QString traceDir = this->TraceEditSettings.value("traceDir", ".").toString();
	QStringList traces = QFileDialog::getOpenFileNames(this , "Load Trace Data", traceDir,
		tr("All Trace Files ( *.xml *.swc *.vtk );;SWC (*.swc);;VTK (*.vtk);; XML ( *.xml )" ));
	QString trace;
	for (int i = 0; i < traces.size(); ++i)
	{
		trace = traces.at(i);
		if (!trace.isEmpty())
		{
			this->EditLogDisplay->append("Trace file: \t" + trace.section('/',-1));
			traceDir = QFileInfo(trace).absolutePath();
			this->TraceEditSettings.setValue("traceDir", traceDir);
			this->TraceFiles.append( trace);
			traceFile = trace.toStdString();
			if(trace.endsWith("swc"))
			{
				this->tobj->ReadFromSWCFile((char*)traceFile.c_str());
			}
			else if(trace.endsWith("xml"))
			{
				this->tobj->ReadFromRPIXMLFile((char*)traceFile.c_str());
			}
			else if (trace.endsWith("vtk"))
			{
				QMessageBox::StandardButton reply;
				reply = QMessageBox::critical(this, "Branching Warning", 
					"Warning .VTK files does not include connectivity order."
					"\nThe Root Tracess must be set BEFORE other edit operations. \n Should The Trace Editor Guess at the Root nodes?",
					QMessageBox::Yes |QMessageBox::No, QMessageBox::No);
				if (reply == QMessageBox::Yes)
				{
					this->tobj->AutoSolveBranchOrder = true;
				}
				else
				{
					this->tobj->AutoSolveBranchOrder = false;
				}
				this->tobj->ReadFromVTKFile((char*)traceFile.c_str());
				if (this->tobj->AutoSolveBranchOrder)
				{
					this->tobj->BranchPoints.clear();
				}
			}
		}
	}
	return trace.section('/',-1);
}

QString View3D::getImageFile()
{
	QString imageDir = this->TraceEditSettings.value("imageDir", ".").toString();
	QString NewImageFile = QFileDialog::getOpenFileName(this , "Load Trace Image Data", imageDir,
		tr("Trace Image ( *.tiff *.tif *.pic *.PIC *.mhd" ));
	if (!NewImageFile.isEmpty())
	{
		imageDir = QFileInfo(NewImageFile).absolutePath();
		this->TraceEditSettings.setValue("imageDir", imageDir);
		this->EditLogDisplay->append("Trace file: \t" + NewImageFile.section('/',-1));
		this->Image.append( NewImageFile);
		int imgNum = this->ImageActors->loadImage(NewImageFile.toStdString(), "Image");
		if (this->translateImages)
		{
			//get x
			bool ok = false;
			double xx, yy, zz;
			while (!ok)
			{
				xx = QInputDialog::getDouble(this, tr("set x"), tr("Shift x by"), 0, -60000, 60000, 1, &ok);
			}
			ok = false;
			while (!ok)
			{
				yy = QInputDialog::getDouble(this, tr("set y"), tr("Shift y by"), 0, -60000, 60000, 1, &ok);
			}
			ok = false;
			while (!ok)
			{
				zz = QInputDialog::getDouble(this, tr("set z"), tr("Shift z by"), 0, -60000, 60000, 1, &ok);
			}
			this->ImageActors->ShiftImage(imgNum, xx, yy, zz );
		}
	}
	return NewImageFile.section('/',-1);
}
//! Open and load Data to approprite types
void View3D::LoadTraces()
{
	QString trace = this->getTraceFile();
	if (!trace.isEmpty())
	{
		this->statusBar()->showMessage(tr("Loading Trace") + trace);
		this->EditLogDisplay->append("Trace file: \t" + this->TraceFiles.last());
		if (this->tobj->FeatureHeaders.size() >=1)
		{
			this->TreeModel = new TraceModel(this->tobj->GetTraceLines(), this->tobj->FeatureHeaders);
		}
		else
		{
			this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		}
		this->TreeModel->scaleFactor = this->uMperVoxel;
		this->ShowTreeData();
		this->UpdateLineActor();
		this->UpdateBranchActor();
		this->QVTK->GetRenderWindow()->Render();
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
	}	
	else
	{
		this->statusBar()->showMessage("Please select a Trace Data file");
	}
}

void View3D::LoadImageData()
{
	std::cout << "LoadImageData called" << std::endl;
	QString newImage = this->getImageFile();
	if (!newImage.isEmpty())
	{
		this->statusBar()->showMessage("Loading Image file" + newImage);
		this->EditLogDisplay->append("Image file: \t" + this->Image.last());

		//QMessageBox::StandardButton reply;
		//reply = QMessageBox::critical(this, "Render Type", 
		//	"Option to use slicer"
		//	"Do you want to use the slicer view?",
		//	QMessageBox::Yes |QMessageBox::No, QMessageBox::No);
		//if (reply == QMessageBox::No)
		if (!this->viewIn2D)
		{
			this->Renderer->AddVolume(this->ImageActors->RayCastVolume(-1));
			this->ImageActors->setRenderStatus(-1, true);
			this->QVTK->GetRenderWindow()->Render();
		}
		else
		{
			//std::cout << "View in 2D and CreateImageSlice" << std::endl;
			//this->Renderer->AddActor(this->ImageActors->CreateImageSlice(-1));
			//this->Renderer->AddViewProp(this->ImageActors->CreateImageSlice(-1));
			this->ImageActors->setIs2D(-1, true);
			this->chooseInteractorStyle(1);//set to image interactor
		}
		//this->Rerender();
		this->statusBar()->showMessage("Image File Rendered");
	}
	else
	{
		this->statusBar()->showMessage("Please select an Image file");
	}
}

void View3D::LoadSomaFile()
{
	QString somaFile = this->getSomaFile();
	if(!somaFile.isNull())
	{
		this->Renderer->AddActor(this->ImageActors->ContourActor(-1));
		this->ImageActors->setRenderStatus(-1, true);
		this->QVTK->GetRenderWindow()->Render();
		this->EditLogDisplay->append("Soma file: \t" + this->SomaFile.last());
		this->statusBar()->showMessage("Somas Rendered");
	}
}


void View3D::LoadProject()
{
	QString projectDir = this->TraceEditSettings.value("projectDir", ".").toString();
	QString projectFile= QFileDialog::getOpenFileName(this , "Load Trace Project File", projectDir,
		tr("project ( *.xml" ));
	if (!projectFile.isEmpty())
	{
		projectDir = QFileInfo(projectFile).absolutePath();
		this->TraceEditSettings.setValue("projectDir", projectDir);
		this->readProject(projectFile);
		this->OkToBoot();
	}// end of project !empty
}

bool View3D::readProject(QString projectFile)
{	
	QString RelativeProjectPath = NULL;
	unsigned int i =0;

	if (!projectFile.isEmpty())
	{
		this->ProjectName = projectFile;
		QFileInfo ProjectFileInfo(projectFile);
		if (!ProjectFileInfo.exists())
		{
			return false;
		}
		RelativeProjectPath = ProjectFileInfo.absolutePath();
		ftk::ProjectManager * project = new ftk::ProjectManager((char*)projectFile.toStdString().c_str());
		for ( i = 0; i < project->size(); i++)
		{ 
			bool found = false;
			std::string FileName = project->GetFileName(i);

			//this->projectFilesTable->setRowCount(project->size());

			//QTableWidgetItem *newfileItem = new QTableWidgetItem(QString::fromStdString(FileName));
			//newfileItem->setFlags(newfileItem->flags() & (~Qt::ItemIsEditable));
			//projectFilesTable->setItem(i,0,newfileItem);

			//{
			//	QTableWidgetItem *newrenderItem = new QTableWidgetItem(tr("on"));
			//	newrenderItem->setFlags(newrenderItem->flags() & (~Qt::ItemIsEditable));
			//	projectFilesTable->setItem(i,2,newrenderItem);
			//	//this->ImageActors->setRenderStatus(i);
			//}

			QFileInfo  NewFileInfo(QString(FileName.c_str()));
			if (!NewFileInfo.exists())
			{
				std::cout << "file not found " << FileName << std::endl;
				QFileInfo testFile(QString(RelativeProjectPath + "/" +  NewFileInfo.fileName()));
				if (testFile.exists())
				{
					FileName = testFile.absoluteFilePath().toStdString();
					std::cout << "found " << FileName << std::endl;
					found = true;
				}
			}
			else
			{
				found = true;
			}  
			if (found)
			{
				QString type = QString(project->GetFileType(i).c_str());

				/*QTableWidgetItem *newtypeItem = new QTableWidgetItem(type);
				newtypeItem->setFlags(newtypeItem->flags() & (~Qt::ItemIsEditable));
				projectFilesTable->setItem(i,1,newtypeItem);*/

				if ((type == "Image")||(type == "Soma"))
				{
					if (type == "Image")
					{
						this->Image.append(QString(FileName.c_str()));
						//this->EditLogDisplay->append("Image file: \t" + this->Image.last());
					}
					else {
						this->SomaFile.append(QString(FileName.c_str()));
						//this->EditLogDisplay->append("Soma file: \t" + this->SomaFile.last());
					}
					this->ImageActors->loadImage(FileName, project->GetFileType(i), 
						project->GetTranslationX(i),project->GetTranslationY(i),project->GetTranslationZ(i));

				}//end type image
				else if (type == "Trace")
				{
					this->tobj->SetTraceOffset(project->GetTranslationX(i),
						project->GetTranslationY(i),project->GetTranslationZ(i));
					QString trace = QString(FileName.c_str());
					this->TraceFiles.append(QString(FileName.c_str()));
					if(trace.endsWith("swc"))
					{
						this->tobj->ReadFromSWCFile((char*)FileName.c_str());
					}
					else if(trace.endsWith("xml"))
					{
						this->tobj->ReadFromRPIXMLFile((char*)FileName.c_str());
					}
					else if (trace.endsWith("vtk"))
					{
						this->tobj->ReadFromVTKFile((char*)FileName.c_str());
					}
					//this->EditLogDisplay->append("Trace file: \t" + this->TraceFiles.last());
				}//end type trace
				else if (type == "Log")
				{
					//the log file reader should be a function and called from here
				}//end type log
				else if (type == "Nuclei_Table")
				{
					this->NucleiFile.append(FileName.c_str());
					//reads nuclei table into linked space
					this->nucleiTable = ftk::AppendLoadTable(FileName.c_str(), this->nucleiTable, project->GetTranslationX(i),
						project->GetTranslationY(i),project->GetTranslationZ(i));
				}
			} //end if newFileInfo exists
		}//end of for project size
		//int maxrow;
		if (!this->TraceFiles.isEmpty() )
		{	
			//this->projectFilesTable->setRowCount(this->TraceFiles.size());
			this->EditLogDisplay->append("Trace file:");
			for (i = 0; i < (unsigned int) this->TraceFiles.size(); i++)
			{
				this->EditLogDisplay->append("\t" + this->TraceFiles.at(i));
			}
		}
		if (!this->Image.isEmpty()) //project table
		{
			//if (this->projectFilesTable->rowCount() < this->Image.size())
			//{
			//	//this->projectFilesTable->setRowCount(this->Image.size());
			//}
			this->EditLogDisplay->append("Image file:");
			for (i=0; i < (unsigned int) this->Image.size(); i++)
			{
				this->EditLogDisplay->append("\t" + this->Image.at(i));
			}
		}
		if (!this->SomaFile.isEmpty())
		{
			//if (this->projectFilesTable->rowCount() < this->SomaFile.size())
			//{
			//	//this->projectFilesTable->setRowCount(this->SomaFile.size());
			//}
			this->EditLogDisplay->append("Soma file: ");
			for (i=0; i < (unsigned int) this->SomaFile.size(); i++)
			{
				this->EditLogDisplay->append("\t" + this->SomaFile.at(i));
			}
		}
		//************************************************************************//
		if (Image.size()>1 || SomaFile.size()>1)
		{
			this->ShowProjectTable();
		}
		//************************************************************************//
		if (!this->NucleiFile.isEmpty())
		{
			// test functions 
			//ftk::SaveTable("C:/testOutput.txt", nucleiTable);
		}
		return true;
	}// end of project !empty
	else
	{
		return false;
	}
}
void View3D::ShowProjectTable()
{
	QString RelativeProjectPath = NULL;
	unsigned int i = 0;
	QFileInfo ProjectFileInfo(this->ProjectName);
	RelativeProjectPath = ProjectFileInfo.absolutePath();
	ftk::ProjectManager * project = new ftk::ProjectManager((char*)this->ProjectName.toStdString().c_str());

	//table set-up
	this->projectFilesDock->show();
	int setrow = Image.size() + SomaFile.size();
	this->projectFilesTable->setRowCount(setrow);
	int j = 0;
	for ( i = 0; i < project->size(); i++)
	{
		bool found = false;
		std::string FileName = project->GetFileName(i);
		std::string FileName1 = FileName;
		QString type = QString(project->GetFileType(i).c_str());
		if ((type == "Image")||(type == "Soma"))
		{
			QFileInfo  NewFileInfo(QString(FileName.c_str()));
			if (!NewFileInfo.exists())
			{
				//std::cout << "file not found " << FileName << std::endl;
				QFileInfo testFile(QString(RelativeProjectPath + "/" +  NewFileInfo.fileName()));
				if (testFile.exists())
				{
					FileName = testFile.absoluteFilePath().toStdString();
					//std::cout << "found " << FileName << std::endl;
					found = true;
				}
			} //end if newFileInfo exists
			else
			{
				found = true;
			}  
			
			if (found && type == "Image")
			{
				//1st column of table
				QTableWidgetItem *newfileItem = new QTableWidgetItem(QString::fromStdString(FileName1));
				newfileItem->setFlags(newfileItem->flags() & (~Qt::ItemIsEditable));
				projectFilesTable->setItem(j,0,newfileItem);
				//2nd column of table
				QTableWidgetItem *newtypeItem = new QTableWidgetItem(type);
				newtypeItem->setFlags(newtypeItem->flags() & (~Qt::ItemIsEditable));
				projectFilesTable->setItem(j,1,newtypeItem);		
				//3rd column of table
				QTableWidgetItem *newrenderItem = new QTableWidgetItem(tr("on"));
				newrenderItem->setFlags(newrenderItem->flags() & (~Qt::ItemIsEditable));
				projectFilesTable->setItem(j,2,newrenderItem);
				//4th column of projectfiletable
				if (this->viewIn2D || this->Use2DSlicer->isChecked()){
					QTableWidgetItem *dimensionItem = new QTableWidgetItem(tr("2d"));
					dimensionItem->setFlags(dimensionItem->flags() & (~Qt::ItemIsEditable));
					projectFilesTable->setItem(j,3,dimensionItem);
				}
				else
				{
					QTableWidgetItem *dimensionItem = new QTableWidgetItem(tr("3d"));
					dimensionItem->setFlags(dimensionItem->flags() & (~Qt::ItemIsEditable));
					projectFilesTable->setItem(j,3,dimensionItem);
				}
				j++; //move on to the next row of the project table
				//this->ImageActors->setRenderStatus(i);	
			} //end of found and imagetype
		} //end of filetype is image or soma
	}// end of project !empty
	this->projectFilesDock->show();
	this->projectFilesTableCreated = true;
}
//! 2-D projection and raycast (no slice image)
void View3D::choosetoRender(int row, int col)
{
	QList<QTableWidgetSelectionRange> ranges = this->projectFilesTable->selectedRanges(); //for future use
	//add buttons to change all highlighted cells to "on" or "off"

	/*std::cout << "row count: " << ranges.first().rowCount() << "\n";
	std::cout << "column count: " << ranges.first().columnCount() << "\n";
	std::cout << "top row: " << ranges.first().topRow() << "\n";
	std::cout << "bottom row: " << ranges.first().bottomRow() << std::endl;
	std::cout << "left column: " << ranges.first().leftColumn() << "\n";
	std::cout << "right column: " << ranges.first().rightColumn() << std::endl;*/

	if(col == 2) //click on one cell in the 3rd column (Renderstatus) only to activate
	{
		QTableWidgetItem *onItem = new QTableWidgetItem(tr("on"));
		onItem->setFlags(onItem->flags() & (~Qt::ItemIsEditable));
		QTableWidgetItem *offItem = new QTableWidgetItem(tr("off"));
		offItem->setFlags(offItem->flags() & (~Qt::ItemIsEditable));

		//int rowselected = ranges.first().topRow();
		if(this->projectFilesTable->item(row,2)->text() == "on") //turn off
		{
			//std::cout << this->projectFilesTable->item(rowselected,2) << std::endl;
			this->projectFilesTable->setItem(row,2,offItem);
			//this->ImageActors->setRenderStatus(row, false);
			
			//remove all actors
			this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(row));
			this->Renderer->RemoveVolume(this->ImageActors->GetRayCastVolume(row));
			this->Renderer->RemoveActor(this->ImageActors->GetContourActor(row));

			this->QVTK->GetRenderWindow()->Render();
			//this->Renderer->UpdateCamera();
		}
		else //turn on
		{
			this->projectFilesTable->setItem(row,2,onItem);
			//this->ImageActors->setRenderStatus(row, true);
			//if (this->ImageActors->isRayCast(row))
			//{
				if (this->projectFilesTable->item(row,3)->text() == "3d")
				{
					this->Renderer->AddVolume(this->ImageActors->RayCastVolume(row));
					this->ImageActors->setRenderStatus(row, true);
					this->ImageActors->setIs2D(row, false);
					this->RaycastBar->show();
				}else if (this->projectFilesTable->item(row,3)->text() == "2d")
				{
					//this->Renderer->AddActor(this->ImageActors->CreateImageSlice(row));
					this->Renderer->AddActor(this->ImageActors->createProjection(row, this->projectionStyle,this->projection_axis));
					this->ImageActors->setIs2D(row, true);
					//this->SlicerBar->show();
				}
			//}
			else
			{
				this->Renderer->AddActor(this->ImageActors->ContourActor(row));
				//this->ImageActors->setRenderStatus(row, true);
			}
		}
	}
}
void View3D::changeDimension(int row, int col)
{
	if (this->projectFilesTable->item(row,2)->text() == "on")
	{
		if(col == 3) //click on one cell in the 4th column (2D/3D) only to activate
		{
			QTableWidgetItem *Item2D = new QTableWidgetItem(tr("2d"));
			Item2D->setFlags(Item2D->flags() & (~Qt::ItemIsEditable));
			QTableWidgetItem *Item3D = new QTableWidgetItem(tr("3d"));
			QFont font;
			font.setBold(true);
			Item3D->setFont(font);
			Item3D->setFlags(Item3D->flags() & (~Qt::ItemIsEditable));
			renderMode = SLICERRAYCAST;

			if(this->projectFilesTable->item(row,3)->text() == "3d")
			{
				//std::cout << this->projectFilesTable->item(rowselected,3) << std::endl;
				//std::cout << "Row selected was 3D" << std::endl;
				this->projectFilesTable->setItem(row,3,Item2D);
				if ((this->ImageActors->getRenderStatus(row))&&(this->ImageActors->isRayCast(row)))
				{
					//std::cout << "Image is Raycast" << std::endl;
					this->Renderer->RemoveVolume(this->ImageActors->GetRayCastVolume(row));
					this->Renderer->AddActor(this->ImageActors->createProjection(row,this->projectionStyle,this->projection_axis));
					this->ImageActors->setIs2D(row, true);
					this->ImageActors->setRenderStatus(row, false);
				}
				//this->SlicerBar->show();
				//this->QVTK->GetRenderWindow()->Render();
				//this->chooseInteractorStyle(1);
			} //end if 3d to 2d
			else
			{
				this->projectFilesTable->setItem(row,3,Item3D);
				//if (this->ImageActors->is2D(row))
				//{
				//std::cout << "Removing projection... and set is not 2D" << std::endl;
				this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(row));
				this->ImageActors->setIs2D(row, false);
				this->Renderer->AddVolume(this->ImageActors->RayCastVolume(row));
				this->ImageActors->setRenderStatus(row, true);
				//}
			} //end else 2d to 3d
		}
		// Check if there are 3D images
		int numof3d=0;
		for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
		{ 
			if (!this->ImageActors->is2D(i))
			{
				numof3d++;
			}
		}
		if (numof3d>0) {
			this->chooseInteractorStyle(0);
			this->RaycastBar->toggleViewAction()->setDisabled(0);
			this->RaycastBar->show();
		}
		else 
		{
			this->chooseInteractorStyle(1);
			this->RaycastBar->toggleViewAction()->setDisabled(1);
			if (this->RaycastBar->isVisible())
			{
				this->RaycastBar->hide();
			}
		}
	}//end if "on"
}
void View3D::SetImgInt()
{
	if (this->ImageActors->NumberOfImages()>=1)
	{	//! image intensity values at each Trace Bit of trace line
		this->tobj->ImageIntensity(this->ImageActors->GetImageData(-1));
	}
}

void View3D::TraceBitImageIntensity(int ImgID)
{
	if (this->ImageActors->NumberOfImages()>=1)
	{	//! image intensity values at each Trace Bit of trace line
		this->tobj->ImageIntensity(this->ImageActors->GetImageData(ImgID));
	}
}


void View3D::Initialize()
{
	//	this->OpacitySlider = 0;
	//	this->BrightnessSlider = 0;

	this->tobj->setSmallLineColor(.25);
	this->tobj->setFalseLineColor(.25);
	this->tobj->setMergeLineColor(.4);
	this->Ascending = Qt::AscendingOrder;

	//Set up a QVTK Widget for embedding a VTK render window in Qt.
	this->QVTK = new QVTKWidget(this->CentralWidget);
	this->Renderer = vtkSmartPointer<vtkRenderer>::New();
	this->Renderer->SetBackground(this->backColorR,this->backColorG,this->backColorB);
	this->QVTK->GetRenderWindow()->AddRenderer(this->Renderer);
	QGridLayout *viewerLayout = new QGridLayout(this->CentralWidget);
	viewerLayout->addWidget(this->QVTK, 0, 0);
	//may add a tree view here
	//layout for the settings window
	this->CreateInteractorStyle();
	this->CreateActors();
	if (!this->TraceFiles.isEmpty())
	{
		this->setWindowTitle(tr("Trace Editor: ")+ this->TraceFiles.last().section('/',-1));
	}
	else
	{
		this->setWindowTitle(tr("Trace Editor"));
	}
	// scalebar incase its needed 
	/*this->scalebar = vtkSmartPointer<vtkLegendScaleActor>::New();
	this->Renderer->AddActor(this->scalebar);*/
	//this->QVTK->GetRenderWindow()->Render();
	this->setupLinkedSpace();
}
void View3D::setupLinkedSpace()
{  
	this->tobj->Gaps.clear();
	this->MergeGaps = new MergeModel(this->tobj->Gaps);
	this->MergeGaps->setParent(this);
	if (this->tobj->FeatureHeaders.size() >=1)
	{
		this->TreeModel = new TraceModel(this->tobj->GetTraceLines(), this->tobj->FeatureHeaders);
	}
	else
	{
		this->TreeModel = new TraceModel(this->tobj->GetTraceLines());
	}
	this->TreeModel->setParent(this);
	this->connect(this->MergeGaps->GetObjectSelection(), SIGNAL(changed()), 
		this,SLOT(updateSelectionHighlights()));
	this->connect(this->TreeModel->GetObjectSelection(), SIGNAL(changed()), 
		this, SLOT(updateTraceSelectionHighlights()));
	this->CellModel = new CellTraceModel();
	this->CellModel->setParent(this);
	this->connect(this->CellModel->GetObjectSelection(), SIGNAL(changed()), 
		this, SLOT(updateSelectionFromCell()));
	this->connect(this->TreeModel->GetObjectSelection(), SIGNAL(changed()), this, SLOT(updateStatistics()));
}

/*!Set up the components of the interface */
void View3D::CreateGUIObjects()
{

	//Set up the menu bar

	this->saveAction = new QAction(tr("&Save as..."), this->CentralWidget);
	connect(this->saveAction, SIGNAL(triggered()), this, SLOT(SaveToFile()));
	this->saveAction->setShortcut(QKeySequence::Save);
	this->saveAction->setStatusTip("Save results to file");

	this->SaveComputedCellFeaturesTableAction = new QAction(tr("&Save Computed Cell Features Table"), this->CentralWidget);
	connect(this->SaveComputedCellFeaturesTableAction, SIGNAL(triggered()), this, SLOT(SaveComputedCellFeaturesTable()));

	this->saveSelectedAction = new QAction(tr("&Save Selected Trees"), this->CentralWidget);
	connect(this->saveSelectedAction, SIGNAL(triggered()), this, SLOT(SaveSelected()));
	this->saveSelectedAction->setStatusTip("Save Selected tree structures to seperate file");

	this->exitAction = new QAction(tr("&Exit"), this->CentralWidget);
	connect(this->exitAction, SIGNAL(triggered()), this, SLOT(close()));
	this->exitAction->setShortcut(QKeySequence::Close);
	this->exitAction->setStatusTip("Exit the Trace Editor");

	this->loadTraceAction = new QAction("Load Traces", this->CentralWidget);
	connect(this->loadTraceAction, SIGNAL(triggered()), this, SLOT(LoadTraces()));
	this->loadTraceAction->setStatusTip("Load traces from .xml or .swc file");
	this->loadTraceAction->setShortcut(QKeySequence::Open);

	this->loadTraceImage = new QAction("Load Image", this->CentralWidget);
	connect (this->loadTraceImage, SIGNAL(triggered()), this, SLOT(LoadImageData()));
	this->loadTraceImage->setStatusTip("Load an Image to RayCast Rendering");

	this->CloseAllImage = new QAction("Remove Image Actors", this->CentralWidget);
	connect (this->CloseAllImage, SIGNAL(triggered()), this, SLOT(removeImageActors()));
	this->CloseAllImage->setStatusTip("remove images from rendering");

	this->loadSoma = new QAction("Load Somas", this->CentralWidget);
	connect(this->loadSoma, SIGNAL(triggered()), this, SLOT(LoadSomaFile()));
	this->loadSoma->setStatusTip("Load image file to Contour rendering");

	this->ScreenshotAction = new QAction("Screen Shot", this->CentralWidget);
	connect(this->ScreenshotAction, SIGNAL(triggered()), this, SLOT(SaveScreenShot()));

	this->AutoCellExportAction = new QAction("Export Cells", this->CentralWidget);
	connect(this->AutoCellExportAction, SIGNAL(triggered()), this, SLOT(AutoCellExport()));

	//Set up the buttons that the user will use to interact with this program. 
	this->ListButton = new QAction("List", this->CentralWidget);
	connect(this->ListButton, SIGNAL(triggered()), this, SLOT(ListSelections()));
	this->ListButton->setStatusTip("List all selections");

	this->ClearButton = new QAction("Clear", this->CentralWidget); 
	connect(this->ClearButton, SIGNAL(triggered()), this, SLOT(ClearSelection()));
	this->ClearButton->setStatusTip("Clear all selections");

	this->SelectTreeAction = new QAction("Select Tree", this->CentralWidget); 
	connect(this->SelectTreeAction, SIGNAL(triggered()), this, SLOT(SelectTrees()));
	this->SelectTreeAction->setStatusTip("Select the entire tree");

	this->DeleteButton = new QAction("Delete", this->CentralWidget);
	connect(this->DeleteButton, SIGNAL(triggered()), this, SLOT(DeleteTraces()));
	this->DeleteButton->setStatusTip("Delete all selected traces");
	this->DeleteButton->setShortcut(QKeySequence(Qt::Key_D));

	this->MergeButton = new QAction("Merge", this->CentralWidget);
	connect(this->MergeButton, SIGNAL(triggered()), this, SLOT(MergeTraces()));
	this->MergeButton->setStatusTip("Start Merge on selected traces");
	this->MergeButton->setShortcut(QKeySequence(Qt::Key_M));

	this->SplitButton = new QAction("Split", this->CentralWidget); 
	connect(this->SplitButton, SIGNAL(triggered()), this, SLOT(SplitTraces()));
	this->SplitButton->setStatusTip("Split traces at point where selected");

	this->FlipButton = new QAction("Flip", this->CentralWidget);
	connect(this->FlipButton, SIGNAL(triggered()), this, SLOT(FlipTraces()));
	this->FlipButton->setStatusTip("Flip trace direction");

	/*this->AutomateButton = new QAction("Automatic Edits", this->CentralWidget);
	connect(this->AutomateButton, SIGNAL(triggered()), this, SLOT(AutomaticEdits()));
	this->AutomateButton->setStatusTip("Automatic selection of all small lines");*/
	//Branching tools
	this->root = new QAction("Set Root", this->CentralWidget);
	connect(this->root, SIGNAL(triggered()), this, SLOT(SetRoots()));
	this->root->setStatusTip("Solve Branch order by defining Root Trace Lines");
	this->root->setShortcut(QKeySequence(Qt::Key_R));

	this->BreakButton = new QAction("Break", this->CentralWidget);
	connect(this->BreakButton, SIGNAL(triggered()), this, SLOT( BreakBranch()));
	this->BreakButton->setStatusTip("Breaks a branch off of the tree");
	this->BreakButton->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_B));
	this->BreakButton->setToolTip("Shift + B");

	this->explodeTree = new QAction("Explode", this->CentralWidget);
	connect(this->explodeTree, SIGNAL(triggered()), this, SLOT( ExplodeTree()));
	this->explodeTree->setStatusTip("Break tree into segments,aka Explode. Tree can be rebuilt using set root");
	this->explodeTree->setShortcut(QKeySequence(Qt::Key_E));
	this->explodeTree->setToolTip("E");

	this->BranchButton = new QAction("Branch", this->CentralWidget);
	connect(this->BranchButton, SIGNAL(triggered()), this, SLOT(AddNewBranches()));
	this->BranchButton->setStatusTip("Add branches to trunk");
	this->BranchButton->setShortcut(QKeySequence(Qt::Key_B));
	this->BranchButton->setToolTip("B");

	this->BranchesLabel = new QLabel(this);
	this->BranchesLabel->setText("0");
	//intensity 
	this->ImageIntensity = new QAction("Intensity", this->CentralWidget);
	this->ImageIntensity->setStatusTip("Calculates intensity of trace bits from one image");
	connect(this->ImageIntensity, SIGNAL(triggered()), this, SLOT(SetImgInt()));
	
	//this->SetRaycastToSlicer = new QAction("Raycast To Slicer", this->CentralWidget);
	//connect(this->SetRaycastToSlicer, SIGNAL(triggered()), this, SLOT(raycastToSlicer()));
	 
	this->SetSlicer = new QAction("Set Slicer", this->CentralWidget);
	connect(this->SetSlicer, SIGNAL(triggered()), this, SLOT(setSlicerMode()));

	this->SetProjection = new QAction("Set Projection", this->CentralWidget);
	connect(this->SetProjection, SIGNAL(triggered()), this, SLOT(setProjectionMode()));

	this->SetRaycast = new QAction("Set Raycast", this->CentralWidget);
	connect(this->SetRaycast, SIGNAL(triggered()), this, SLOT(setRaycastMode()));
	

	this->ColorByTreesAction = new QAction("Color By Trees", this->CentralWidget);
	this->ColorByTreesAction->setCheckable(true);
	this->ColorByTreesAction->setChecked(false);
	connect(this->ColorByTreesAction, SIGNAL(triggered()), this, SLOT(ToggleColorByTrees()));
	// 3d cursor actions 
	this->CursorActionsWidget = new QWidget(this);

	//vessel segmentation actions
	this->vesselSegWidget = new QWidget(this);

	this->ArunVesselTracingButton = new QPushButton("Trace Centerlines", this->CentralWidget);
	connect(this->ArunVesselTracingButton, SIGNAL(clicked()), this, SLOT(ArunCenterline()));
	this->ArunVesselTracingButton->setDisabled(true);

	this->updatePT3D = new QPushButton("Update Location", this->CentralWidget);
	connect(this->updatePT3D, SIGNAL(clicked()), this, SLOT(getPosPTin3D()));
	this->updatePT3D->setShortcut(QKeySequence(Qt::Key_U));

	this->setSoma = new QPushButton("Create Soma", this->CentralWidget);
	connect(this->setSoma, SIGNAL(clicked()), this, SLOT(setPTtoSoma()));

	this->createNewBitButton = new QPushButton("Create New TraceBit", this->CentralWidget);
	connect(this->createNewBitButton, SIGNAL(clicked()), this, SLOT(createNewTraceBit()));
	this->createNewBitButton->setShortcut(QKeySequence(Qt::Key_P));

	this->createNewROIPointButton = new QPushButton("Create New ROI point", this->CentralWidget);
	connect(this->createNewROIPointButton, SIGNAL(clicked()), this, SLOT(AddROIPoint()));

	this->ExtrudeROIButton = new QPushButton("Extrude ROI points", this->CentralWidget);
	connect(this->ExtrudeROIButton, SIGNAL(clicked()), this, SLOT(DrawROI()));

	this->CalculateDistanceToDeviceButton = new QPushButton("Calculate Distance To Device", this->CentralWidget);
	connect(this->CalculateDistanceToDeviceButton, SIGNAL(clicked()), this, SLOT(CalculateDistanceToDevice()));

	this->CalculateCellDistanceButton = new QPushButton("Calculate Cell to Cell Distance Graph", this->CentralWidget);
	connect(this->CalculateCellDistanceButton, SIGNAL(clicked()), this, SLOT(CalculateCellToCellDistanceGraph()));

	this->LoadNucleiTable = new QAction("Load Nuclei Table", this->CentralWidget);
	connect(this->LoadNucleiTable, SIGNAL(triggered()), this, SLOT(readNucleiTable()));

	this->AssociateCellToNucleiAction = new QAction("Associate Nuclei To Cells", this->CentralWidget);
	connect(this->AssociateCellToNucleiAction, SIGNAL(triggered()), this, SLOT(AssociateNeuronToNuclei()));
	this->AssociateCellToNucleiAction->setDisabled(true);

	this->ShowPointer = new QCheckBox("Use 3D Cursor", this->CentralWidget);
	this->ShowPointer->setStatusTip("Show Pointer Automatically?");
	this->ShowPointer3DDefault = true;
	this->ShowPointer->setChecked(this->ShowPointer3DDefault);
	connect(this->ShowPointer, SIGNAL(stateChanged(int)), this, SLOT(setUsePointer(int)));

	this->posX = new QDoubleSpinBox(this);
	this->posX->setValue(0);
	this->posX->setRange(-60000, 60000);
	connect(this->posX, SIGNAL(valueChanged(double)), this, SLOT(showPTin3D(double)));

	this->posY = new QDoubleSpinBox(this);
	this->posY->setValue(0);
	this->posY->setRange(-60000, 60000);
	connect(this->posY, SIGNAL(valueChanged(double)), this, SLOT(showPTin3D(double)));

	this->posZ = new QDoubleSpinBox(this->CentralWidget);
	this->posZ->setValue(0);
	this->posZ->setRange(-60000, 60000);
	connect(this->posZ, SIGNAL(valueChanged(double)), this, SLOT(showPTin3D(double)));
	//advanced render actions	
	this->FocusAction = new QAction("Render Focus", this->CentralWidget); 
	connect(this->FocusAction, SIGNAL(triggered()), this, SLOT(focusOn()));

	//Setup the settings editing window
	this->SettingsWidget = new QWidget(this);
	this->MaxGapField = new QSpinBox(this->SettingsWidget);
	this->MaxGapField->setRange(0,1000);

	this->GapToleranceField = new QDoubleSpinBox(this->SettingsWidget);
	this->GapToleranceField->setRange(0,100);
	this->GapToleranceField->setSingleStep(.1);

	this->ColorValueField = new QDoubleSpinBox(this->SettingsWidget);
	this->ColorValueField->setRange(0,1);
	this->ColorValueField->setSingleStep(.01);

	this->LineWidthField = new QSpinBox(this->SettingsWidget);
	this->LineWidthField->setRange(1,5);

	this->markTraceBits = new QCheckBox("Mark all traced Points",this->SettingsWidget);
	this->markTraceBits->setChecked(this->renderTraceBits);

	this->BackgroundRBox = new QDoubleSpinBox(this->SettingsWidget);
	this->BackgroundRBox->setRange(0,1);
	this->BackgroundRBox->setSingleStep(.01);

	this->BackgroundGBox = new QDoubleSpinBox(this->SettingsWidget);
	this->BackgroundGBox->setRange(0,1);
	this->BackgroundGBox->setSingleStep(.01);

	this->BackgroundBBox = new QDoubleSpinBox(this->SettingsWidget);
	this->BackgroundBBox->setRange(0,1);
	this->BackgroundBBox->setSingleStep(.01);

	this->RollBox = new QDoubleSpinBox(this->SettingsWidget);
	this->RollBox->setRange(-360,360);
	this->RollBox->setSingleStep(1);

	this->ElevationBox = new QDoubleSpinBox(this->SettingsWidget);
	this->ElevationBox->setRange(-90,90);
	this->ElevationBox->setSingleStep(1);

	this->AzimuthBox = new QDoubleSpinBox(this->SettingsWidget);
	this->AzimuthBox->setRange(-360,360);
	this->AzimuthBox->setSingleStep(1);

	this->ApplySettingsButton = new QDialogButtonBox(QDialogButtonBox::SaveAll | QDialogButtonBox::Close);
	connect(this->ApplySettingsButton, SIGNAL(accepted()), this, SLOT(ApplyNewSettings()));
	connect(this->ApplySettingsButton, SIGNAL(rejected()), this, SLOT(HideSettingsWindow()));

	this->tobj->gapTol = this->TraceEditSettings.value("mainWin/gapTol", .5).toDouble() ;
	this->tobj->gapMax = this->TraceEditSettings.value("mainWin/gapMax", 10).toInt();
	this->SmallLineLength = this->TraceEditSettings.value("mainWin/smallLine", 10).toDouble();
	this->SelectColor =this->TraceEditSettings.value("mainWin/selectColor", .1).toDouble();
	this->lineWidth= this->TraceEditSettings.value("mainWin/LineWidth", 2).toDouble();
	this->MaxGapField->setValue(this->tobj->gapMax);
	this->GapToleranceField->setValue(this->tobj->gapTol);
	this->ColorValueField->setValue(this->SelectColor);
	this->LineWidthField->setValue(this->lineWidth);
	this->BackgroundRBox->setValue(this->backColorR);
	this->BackgroundGBox->setValue(this->backColorG);
	this->BackgroundBBox->setValue(this->backColorB);

	QStringList types;
	types <<"0 = undefined" << "1 = soma" <<"2 = axon" <<"3 = dendrite" 
		<<"4 = apical dendrite" <<"5 = fork point" <<"6 = end point" <<"7 = custom";
	this->typeCombo = new QComboBox;
	this->typeCombo->addItems(types);
	connect(this->typeCombo, SIGNAL(activated( int )), this, SLOT(SetTraceType(int )));

	QStringList ZoomStyles;
	ZoomStyles<< "Track Ball" << "Image" << "RubberBandZoom" << "Slicer";
	this->StyleCombo = new QComboBox;
	this->StyleCombo->addItems(ZoomStyles);
	connect(this->StyleCombo, SIGNAL(activated(int)), this, SLOT(chooseInteractorStyle(int)));

	QStringList ProjectionStyles;
	ProjectionStyles<< "Maximum" << "Mean" << "Minimum";
	this->ProjectionCombo = new QComboBox;
	this->ProjectionCombo->addItems(ProjectionStyles);
	connect(this->ProjectionCombo, SIGNAL(activated(int)), this, SLOT(SetProjectionMethod(int)));

	QStringList AxisList;
	AxisList<< "X-Y" << "X-Z" << "Y-Z";
	this->RotateImageUpCombo = new QComboBox;
	this->RotateImageUpCombo->addItems(AxisList);
	connect(this->RotateImageUpCombo, SIGNAL(activated(int)), this, SLOT(rotateImage(int)));

	/*this->ProjectionAxisCombo = new QComboBox;
	this->ProjectionAxisCombo->addItems(AxisList);
	connect(this->ProjectionAxisCombo, SIGNAL(activated(int)), this, SLOT(projectionAlongAxis(int)));*/
	
	this->aboutAction = new QAction("About", this->CentralWidget);
	this->aboutAction->setStatusTip("About Trace Edit");
	connect(this->aboutAction, SIGNAL(triggered()), this, SLOT(About()));

	this->ShowPlots = new QAction("Show Plots", this);
	this->ShowPlots->isCheckable();
	connect (this->ShowPlots, SIGNAL(triggered()), this, SLOT(ShowTreeData()));	  
	//Automation widget setup
	this->AutomationWidget = new QWidget(this);

	this->SmallLinesButton = new QRadioButton(tr("Small Lines"));
	connect(this->SmallLinesButton, SIGNAL(clicked()), this, SLOT(ShowAutomatedEdits()));
	this->FalseSpinesButton = new QRadioButton(tr("False Spines"));
	connect(this->FalseSpinesButton, SIGNAL(clicked()), this, SLOT(ShowAutomatedEdits()));
	this->FalseBridgesButton = new QRadioButton(tr("Bridges"));
	connect(this->FalseBridgesButton, SIGNAL(clicked()), this, SLOT(ShowAutomatedEdits()));
	this->HalfBridgesButton = new QRadioButton(tr("Half Bridges"));
	connect(this->HalfBridgesButton, SIGNAL(clicked()), this, SLOT(ShowAutomatedEdits()));

	this->LineLengthField = new QDoubleSpinBox(this->SettingsWidget);
	this->LineLengthField->setRange(0,1000);
	this->LineLengthField->setValue(this->SmallLineLength);
	connect(this->LineLengthField, SIGNAL(valueChanged(double)), this, SLOT( SLine(double)));

	//this->RotateRight90Button = new QPushButton(tr("Roll +90"), this->SettingsWidget);
	//connect(this->RotateRight90Button, SIGNAL(clicked()), this, SLOT(rotateImageRight90()));

	this->updateRotationButton = new QPushButton(tr("Update"),this->SettingsWidget);
	connect(this->updateRotationButton, SIGNAL(clicked()), this, SLOT(rotationOptions()));

	this->MaxSpineBit = new QDoubleSpinBox(this->AutomationWidget);
	this->MaxSpineBit->setRange(1,20);
	this->MaxSpineBit->setValue(4);
	connect(this->MaxSpineBit, SIGNAL(valueChanged(double)), this, SLOT( FakeSpines(double)));

	this->MaxBridgeBits =  new QDoubleSpinBox(this->AutomationWidget);
	this->MaxBridgeBits->setRange(1,20);
	this->MaxBridgeBits->setValue(3);
	connect(this->MaxBridgeBits, SIGNAL(valueChanged(double)), this, SLOT( FakeBridges(double)));

	this->MaxHalfBridgeBits =  new QDoubleSpinBox(this->AutomationWidget);
	this->MaxHalfBridgeBits->setRange(1,20);
	this->MaxHalfBridgeBits->setValue(10);
	connect(this->MaxHalfBridgeBits, SIGNAL(valueChanged(double)), this, SLOT( HalfBridges(double)));

	this->MaxSpinePathLength = new QDoubleSpinBox(this->AutomationWidget);
	this->MaxSpinePathLength->setRange(1,30);
	this->MaxSpinePathLength->setSingleStep(.01);
	this->MaxSpinePathLength->setValue(3);
	connect(this->MaxSpinePathLength, SIGNAL(valueChanged(double)), this, SLOT( FakeSpines(double)));

	this->MinDistanceToParent = new QDoubleSpinBox(this->AutomationWidget);
	this->MinDistanceToParent->setRange(1,40);
	this->MinDistanceToParent->setSingleStep(.01);
	this->MinDistanceToParent->setValue(6);
	connect(this->MinDistanceToParent, SIGNAL(valueChanged(double)), this, SLOT( HalfBridges(double)));

	this->AutomateButton = new QPushButton("Automate Correction",this->AutomationWidget);
	connect(this->AutomateButton, SIGNAL(clicked()), this, SLOT(AutomaticEdits()));
	this->AutomateButton->setShortcut(QKeySequence(Qt::Key_A));

	this->CellAnalysis = new QAction("Cell Analysis", this->CentralWidget);
	connect (this->CellAnalysis, SIGNAL(triggered()), this, SLOT(ShowCellAnalysis()));
	this->CellAnalysis->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));

	this->StartActiveLearningAction = new QAction("Start Active Learning", this->CentralWidget);
	connect (this->StartActiveLearningAction, SIGNAL(triggered()), this, SLOT(StartActiveLearning()));

	// Lables for the status bar to show edit counts
	this->SplitLabel = new QLabel(this);
	this->SplitLabel->setText(QString::number(this->numSplit));

	this->MergeLabel = new QLabel(this);
	this->MergeLabel->setText(QString::number(this->numMerged));

	this->DeleteLabel = new QLabel(this);
	this->DeleteLabel->setText(QString::number(this->numDeleted));

	/********************************************************************************************/
	QStringList projecttableHeaders;
	projecttableHeaders << tr("Filename") << tr("Type") << tr("Renderstatus") << tr("2d/3d");
	this->projectFilesTable = new QTableWidget(this);
	this->projectFilesTable->setColumnCount(4);	
	this->projectFilesTable->setColumnWidth(1,40);
	this->projectFilesTable->setColumnWidth(2,75);
	this->projectFilesTable->setColumnWidth(3,35);

	QObject::connect(this->projectFilesTable, SIGNAL(cellClicked(int,int)), this, SLOT(choosetoRender(int,int)));
	QObject::connect(this->projectFilesTable, SIGNAL(cellClicked(int,int)), this, SLOT(changeDimension(int,int)));
	this->projectFilesTable->setHorizontalHeaderLabels(projecttableHeaders);
	//this->projectFilesTable->setSortingEnabled(true); //complicated

	//this->Item2D = new QTableWidgetItem(tr("2d"));
	//Item2D->setFlags(Item2D->flags() & (~Qt::ItemIsEditable));
	//this->projectFilesTable->setItem(i,3,Item2D);

	//this->Item3D = new QTableWidgetItem(tr("3d"));
	//Item3D->setFlags(Item3D->flags() & (~Qt::ItemIsEditable));
	//QFont font;
	//font.setBold(true);
	//Item3D->setFont(font);

	/********************************************************************************************/

	CropBorderCellsButton = new QPushButton("Crop border cells");
	connect(CropBorderCellsButton, SIGNAL(clicked()), this, SLOT(CropBorderCells()));

}

void View3D::CreateLayout()
{
	this->fileMenu = this->menuBar()->addMenu(tr("&File"));
	this->fileMenu->addAction(this->loadTraceAction);
	this->fileMenu->addAction(this->loadTraceImage);
	this->fileMenu->addAction(this->loadSoma);
	this->fileMenu->addAction(this->LoadNucleiTable);
	this->fileMenu->addSeparator();
	this->fileMenu->addAction(this->saveAction);
	this->fileMenu->addAction(this->SaveComputedCellFeaturesTableAction);
	this->fileMenu->addAction(this->saveSelectedAction);
	this->fileMenu->addSeparator();
	this->fileMenu->addAction(this->ScreenshotAction);
	this->fileMenu->addAction(this->AutoCellExportAction);
	this->fileMenu->addSeparator();
	this->fileMenu->addAction(this->CloseAllImage);
	this->fileMenu->addAction(this->exitAction);

	this->ShowToolBars = this->menuBar()->addMenu(tr("Tool Bars"));
	this->DataViews = this->menuBar()->addMenu(tr("Visualization"));
	this->analysisViews = this->menuBar()->addMenu(tr("Analysis"));

	this->EditsToolBar = addToolBar(tr("Edit Toolbar"));
	this->EditsToolBar->setToolTip("EditToolBar");
	this->ShowToolBars->addAction(this->EditsToolBar->toggleViewAction());

	//this->EditsToolBar->addAction(this->AutomateButton);
	this->EditsToolBar->addAction(this->ListButton);
	this->EditsToolBar->addAction(this->ClearButton);
	this->EditsToolBar->addAction(this->SelectTreeAction);
	this->EditsToolBar->addSeparator();
	this->EditsToolBar->addAction(this->DeleteButton);
	this->EditsToolBar->addAction(this->MergeButton);
	this->EditsToolBar->addAction(this->SplitButton);
	this->EditsToolBar->addAction(this->FlipButton);
	this->EditsToolBar->addSeparator();
	this->EditsToolBar->addWidget(this->typeCombo);
	this->EditsToolBar->addAction(this->ImageIntensity);
	this->EditsToolBar->addAction(this->FocusAction);
	this->EditsToolBar->hide();

	// 3d cursor dock 
	this->cursor3DDock = new QDockWidget("3D Cursor",this);
	QVBoxLayout * CursorToolsLayout = new QVBoxLayout(this->CursorActionsWidget);
	QGroupBox * CursorLocationBox = new QGroupBox("Cursor Location");
	QFormLayout *CursorLocationLayout = new QFormLayout();
	CursorLocationLayout->addRow("X",this->posX);
	CursorLocationLayout->addRow("Y",this->posY);
	CursorLocationLayout->addRow("Z",this->posZ);
	CursorLocationBox->setLayout(CursorLocationLayout);
	CursorToolsLayout->addWidget(CursorLocationBox);
	CursorToolsLayout->addWidget(this->ShowPointer);
	//CursorToolsLayout->addWidget(this->MoveSphere);
	CursorToolsLayout->addWidget(this->updatePT3D);
	CursorToolsLayout->addWidget(this->setSoma);
	CursorToolsLayout->addWidget(this->createNewBitButton);

	QGroupBox * CursorROIBox = new QGroupBox("ROI Tools");
	QVBoxLayout *CursorROILayout = new QVBoxLayout();
	CursorROILayout->addWidget(this->createNewROIPointButton);
	CursorROILayout->addWidget(this->ExtrudeROIButton);
	this->ExtrudeROIButton->setEnabled(false);
	CursorROILayout->addWidget(this->CalculateDistanceToDeviceButton);
	this->CalculateDistanceToDeviceButton->setEnabled(false);
	CursorROIBox->setLayout(CursorROILayout);
	CursorToolsLayout->addWidget(CursorROIBox);
	CursorToolsLayout->addWidget(this->CalculateCellDistanceButton);
	CursorToolsLayout->addStretch();

	this->CursorActionsWidget->setMaximumSize(256,500);
	this->cursor3DDock->setWidget(this->CursorActionsWidget);
	this->addDockWidget(Qt::LeftDockWidgetArea, this->cursor3DDock);
	this->ShowToolBars->addAction(this->cursor3DDock->toggleViewAction());
	this->cursor3DDock->hide();

	
	//vessel segmentation toolbar
	this->vesselSegDock = new QDockWidget("Segment Vessels",this);
	QVBoxLayout * VesselSegDockLayout = new QVBoxLayout(this->vesselSegWidget);
	VesselSegDockLayout->addWidget(this->ArunVesselTracingButton);
	this->vesselSegWidget->setMaximumSize(256,500);
	this->vesselSegDock->setWidget(this->vesselSegWidget);
	this->addDockWidget(Qt::LeftDockWidgetArea, this->vesselSegDock);
	this->ShowToolBars->addAction(this->vesselSegDock->toggleViewAction());
	this->vesselSegDock->hide();

	// branching toolbars
	this->BranchToolBar = addToolBar(tr("Branch Toolbar"));
	this->BranchToolBar->setToolTip("Branch Toolbar");
	//this->menuBar()->addAction(this->BranchToolBar->toggleViewAction());
	this->ShowToolBars->addAction(this->BranchToolBar->toggleViewAction());
	this->BranchToolBar->addAction(this->BreakButton);
	this->BranchToolBar->addAction(this->explodeTree);
	this->BranchToolBar->addAction(this->BranchButton);
	this->BranchToolBar->addAction(this->root);
	this->BranchToolBar->addWidget(new QLabel("Unsolved Branches: "));
	this->BranchToolBar->addWidget(this->BranchesLabel);
	this->BranchToolBar->hide();

	//settings widget layout
	QVBoxLayout * SettingsBox = new QVBoxLayout(this->SettingsWidget);
	QGroupBox *selectionSettings = new QGroupBox("Selection Settings");
	QFormLayout *settingsLayout = new QFormLayout(selectionSettings);
	settingsLayout->addRow(tr("Maximum gap length:"), this->MaxGapField);
	settingsLayout->addRow(tr("Gap length tolerance:"),this->GapToleranceField);
	//settingsLayout->addRow(tr("Small line length:"),this->LineLengthField);
	//selectionSettings->setLayout(settingsLayout);
	SettingsBox->addWidget(selectionSettings);

	QGroupBox *displaySettings = new QGroupBox("Display Settings");
	QFormLayout *DisplayLayout = new QFormLayout(displaySettings);
	DisplayLayout->addRow(tr("Line Color RGB 0 to 1:"),this->ColorValueField);
	DisplayLayout->addRow(tr("Line width:"),this->LineWidthField);
	DisplayLayout->addRow(tr("Interactor style:"),this->StyleCombo);
	DisplayLayout->addRow(tr("Projection style:"),this->ProjectionCombo);
	DisplayLayout->addRow(tr("Projection plane: "),this->RotateImageUpCombo);
	//DisplayLayout->addRow(tr("2D Projection: "),this->ProjectionAxisCombo);
	DisplayLayout->addRow(this->markTraceBits);
	SettingsBox->addWidget(displaySettings);

	QGroupBox *rotationSettings = new QGroupBox("Rotation");
	QFormLayout *RotateLayout = new QFormLayout(rotationSettings);
	RotateLayout->addRow(tr("Roll: "),this->RollBox);
	RotateLayout->addRow(tr("Elevation: "),this->ElevationBox);
	RotateLayout->addRow(tr("Azimuth: "),this->AzimuthBox);
	RotateLayout->addWidget(this->updateRotationButton);
	SettingsBox->addWidget(rotationSettings);

	QGroupBox *BackgroundSettings = new QGroupBox(("Background RGB Color"));
	QFormLayout *BackgroundLayout = new QFormLayout(BackgroundSettings);
	BackgroundLayout->addRow(tr("Value Red: "), this->BackgroundRBox);
	BackgroundLayout->addRow(tr("Value Blue: "),this->BackgroundGBox);
	BackgroundLayout->addRow(tr("Value Green: "),this->BackgroundBBox);
	SettingsBox->addWidget(BackgroundSettings);

	SettingsBox->addWidget(this->ApplySettingsButton);
	SettingsBox->addStretch();

	this->SettingsWidget->setMaximumSize(256,600);

	this->settingsDock = new QDockWidget("Editor Settings", this);
	this->settingsDock->setWidget(this->SettingsWidget);
	this->addDockWidget(Qt::LeftDockWidgetArea, this->settingsDock);
	this->DataViews->addAction(this->settingsDock->toggleViewAction());
	this->settingsDock->hide();

	showStatisticsAction = new QAction(tr("Show Statistics Toolbar"), this);
	connect(showStatisticsAction,SIGNAL(triggered()), this, SLOT(showStatistics()));
	updateStatisticsAction = new QAction(tr("Update Statistics"), this);
	connect(updateStatisticsAction, SIGNAL(triggered()), this, SLOT(updateStatistics()));
	//////////////////////
	// Automation Dock	//
	//////////////////////
	this->AutomationDock = new QDockWidget("Automated Edits", this);
	QVBoxLayout * AutomationDockLayout = new QVBoxLayout(this->AutomationWidget);

	QGroupBox *SelectedErrorGroup = new QGroupBox("Select Error Type");
	QGridLayout * SelectedErrorLayout = new QGridLayout(SelectedErrorGroup);
	SelectedErrorLayout->addWidget( this->SmallLinesButton, 0, 0);
	SelectedErrorLayout->addWidget( this->FalseSpinesButton, 0, 1);
	SelectedErrorLayout->addWidget( this->FalseBridgesButton, 1, 0);
	SelectedErrorLayout->addWidget( this->HalfBridgesButton, 1, 1);
	AutomationDockLayout->addWidget(SelectedErrorGroup);

	this->SmallLinesGroup = new QGroupBox("Detect Small Lines");
	this->SmallLinesGroup->setEnabled(0);
	QFormLayout * SmallLineLayout = new QFormLayout(this->SmallLinesGroup);
	SmallLineLayout->addRow("Size", this->LineLengthField);
	AutomationDockLayout->addWidget(this->SmallLinesGroup);

	this->FakeSpinesGroup = new QGroupBox("Detect Fake Spines");
	this->FakeSpinesGroup->setEnabled(0);
	QFormLayout * FakeSpinesLayout = new QFormLayout(this->FakeSpinesGroup);
	FakeSpinesLayout->addRow("Size", this->MaxSpineBit);
	FakeSpinesLayout->addRow("Path Length", this->MaxSpinePathLength);
	AutomationDockLayout->addWidget(this->FakeSpinesGroup);

	this->FakeBridgeGroup = new QGroupBox("Detect Bridges");
	this->FakeBridgeGroup->setEnabled(0);
	QFormLayout * FakeBridgesLayout = new QFormLayout(this->FakeBridgeGroup);
	FakeBridgesLayout->addRow("Size", this->MaxBridgeBits);
	AutomationDockLayout->addWidget(this->FakeBridgeGroup);

	this->HalfBridgeGroup = new QGroupBox("Detect Half Bridges");
	this->HalfBridgeGroup->setEnabled(0);
	QFormLayout * HalfBridgesLayout = new QFormLayout(this->HalfBridgeGroup);
	HalfBridgesLayout->addRow("Size", this->MaxHalfBridgeBits);
	HalfBridgesLayout->addRow("Distance From Parent", this->MinDistanceToParent);
	AutomationDockLayout->addWidget(this->HalfBridgeGroup);
	AutomationDockLayout->addWidget(this->AutomateButton);
	
	//Select border cells
	BorderCellsCroppingGroup = new QGroupBox("Border Cells Cropping");
	QFormLayout *BorderCellsCroppingLayout = new QFormLayout(BorderCellsCroppingGroup);
	BorderCellsCroppingLayout->addWidget(CropBorderCellsButton);
	AutomationDockLayout->addWidget(BorderCellsCroppingGroup);
	AutomationDockLayout->addStretch();

	this->AutomationDock->setWidget(this->AutomationWidget);
	this->addDockWidget(Qt::RightDockWidgetArea, this->AutomationDock);
	this->ShowToolBars->addAction(this->AutomationDock->toggleViewAction());
	this->AutomationDock->hide();

	////////////////
	// Status Bar //
	////////////////

	this->statusBar()->addPermanentWidget(new QLabel("Statistics: Split: ", this));
	this->statusBar()->addPermanentWidget(this->SplitLabel,0);
	this->statusBar()->addPermanentWidget(new QLabel(" Merged: ", this));
	this->statusBar()->addPermanentWidget(this->MergeLabel,0);
	this->statusBar()->addPermanentWidget(new QLabel(" Deleted: ", this));
	this->statusBar()->addPermanentWidget(this->DeleteLabel,0);

	this->analysisViews->addAction(this->InformationDisplays->toggleViewAction());
	this->analysisViews->addAction(this->ShowPlots);
	this->analysisViews->addAction(this->showStatisticsAction);
	// this->analysisViews->addAction(this->updateStatisticsAction);
	this->analysisViews->addAction(this->CellAnalysis);
	this->analysisViews->addAction(this->StartActiveLearningAction);
	this->analysisViews->addAction(this->AssociateCellToNucleiAction);

	//this->ShowToolBars->addSeparator();
	QMenu *renderer_sub_menu = this->DataViews->addMenu(tr("Renderer Mode"));
	renderer_sub_menu->addAction(this->SetSlicer);
	renderer_sub_menu->addAction(this->SetProjection);
	renderer_sub_menu->addAction(this->SetRaycast);
	this->DataViews->addAction(this->ColorByTreesAction);

	this->createRayCastSliders();
	this->menuBar()->addSeparator();
	this->help = this->menuBar()->addMenu("Help");
	this->help->addAction(this->aboutAction);

	//this->createSlicerSlider();

	this->menuBar()->hide();

	/**************************************************************************/
	this->projectFilesDock = new QDockWidget(tr("List of Image Files"), this);
	this->projectFilesDock->setWidget(this->projectFilesTable);
	this->addDockWidget(Qt::LeftDockWidgetArea, this->projectFilesDock);
	this->ShowToolBars->addAction(this->projectFilesDock->toggleViewAction());
	this->projectFilesDock->hide();
	/**************************************************************************/

	/**************************************************************************/


}

void View3D::ShowAutomatedEdits()
{
	this->SmallLinesGroup->setEnabled(0);
	this->FakeSpinesGroup->setEnabled(0);
	this->FakeBridgeGroup->setEnabled(0);
	this->HalfBridgeGroup->setEnabled(0);
	if (this->SmallLinesButton->isChecked())
	{
		//this->TreeModel->GetObjectSelection()->clear();
		this->SmallLinesGroup->setEnabled(1);
		this->SLine(1);
	}
	else if (this->FalseSpinesButton->isChecked())
	{
		//this->TreeModel->GetObjectSelection()->clear();
		this->FakeSpinesGroup->setEnabled(1);
		this->FakeSpines(1);
	}
	else if (this->FalseBridgesButton->isChecked())
	{
		//this->TreeModel->GetObjectSelection()->clear();
		this->FakeBridgeGroup->setEnabled(1);
		this->FakeBridges(1);
	}
	else if (this->HalfBridgesButton->isChecked())
	{
		//this->TreeModel->GetObjectSelection()->clear();
		this->HalfBridgeGroup->setEnabled(1);
		this->HalfBridges(1);
	}
}

/*! create interactors*/
void View3D::CreateInteractorStyle()
{
	this->Interactor = this->QVTK->GetRenderWindow()->GetInteractor();
	//keep mouse command observers, but change the key ones
	this->keyPress = vtkSmartPointer<vtkCallbackCommand>::New();
	this->keyPress->SetCallback(HandleKeyPress);
	this->keyPress->SetClientData(this);

	this->Interactor->RemoveObservers(vtkCommand::KeyPressEvent);
	this->Interactor->RemoveObservers(vtkCommand::KeyReleaseEvent);
	this->Interactor->RemoveObservers(vtkCommand::CharEvent);
	this->Interactor->AddObserver(vtkCommand::KeyPressEvent, this->keyPress);

	//use trackball control for mouse commands

	this->chooseInteractorStyle(0);
	this->CellPicker = vtkSmartPointer<vtkCellPicker>::New();
	this->CellPicker->SetTolerance(0.004);
	this->Interactor->SetPicker(this->CellPicker);
	this->isPicked = vtkSmartPointer<vtkCallbackCommand>::New();
	this->isPicked->SetCallback(PickCell);

	//isPicked caller allows observer to intepret click 
	this->isPicked->SetClientData(this);            
	this->Interactor->AddObserver(vtkCommand::RightButtonPressEvent,isPicked);
}

void View3D::chooseInteractorStyle(int iren)
{
	if (iren== 1)
	{
		vtkCamera *cam = this->Renderer->GetActiveCamera();
		cam->SetFocalPoint(0,0,0);
		cam->SetPosition(0,0,1);
		cam->ComputeViewPlaneNormal();
		cam->SetViewUp(0,1,0);
		cam->OrthogonalizeViewUp();
		cam->ParallelProjectionOn();
		vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
		this->Interactor->SetInteractorStyle(style);
		this->Renderer->ResetCamera();
		this->QVTK->GetRenderWindow()->Render();
	}else if (iren== 2)
	{
		vtkSmartPointer<vtkInteractorStyleRubberBandZoom> style = vtkSmartPointer<vtkInteractorStyleRubberBandZoom>::New();
		this->Interactor->SetInteractorStyle(style);
	}else if (iren == 3)
	{
		vtkSmartPointer<vtkInteractorStyleImage> styleImage = vtkSmartPointer<vtkInteractorStyleImage>::New();
		styleImage->SetInteractionModeToImage3D();
		this->Interactor->SetInteractorStyle(styleImage);
		this->SlicerBar->show();
	}else
	{
		vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
		this->Interactor->SetInteractorStyle(style);
		this->Renderer->ResetCamera();
		this->QVTK->GetRenderWindow()->Render();
	}
}

void View3D::rotateImage(int axis)
{
	int projection_axis;
	this->RollBox->setValue(0.00);
	this->AzimuthBox->setValue(0.00);
	this->ElevationBox->setValue(0.00);
	vtkCamera *cam = this->Renderer->GetActiveCamera();
	cam->SetFocalPoint(0,0,0);
	cam->SetPosition(0,0,1);
	cam->ComputeViewPlaneNormal();
	double x = 0; double y = 1; double z = 0;
	cam->SetViewUp(x,y,z);	

	switch(axis)
	{
		case 0:	
			projection_axis = 2;
			projection_base.roll = 0; projection_base.azimuth = 0;   projection_base.elevation = 0;		break; // x-y plane; z projection
		case 1: 
			cam->Elevation(90);	
			cam->OrthogonalizeViewUp();
			projection_axis = 1; 
			projection_base.roll = 0; projection_base.azimuth = 0;   projection_base.elevation = -90;	break; // x-z plane; y projection
		case 2: 
			cam->Azimuth(90); cam->Roll(-90);
			projection_axis = 0; 
			projection_base.roll = -90; projection_base.azimuth = 90; projection_base.elevation = 0;	break; // y-z plane; x projection
		default: std::cerr << "View3D::rotateImage cannot handle axis = " << axis << ". Defaulting to y-axis" << std::endl;
	}
	this->projection_axis = projection_axis;
	this->SetProjectionMethod(projectionStyle); //for projection and slicer mode

	cam->ParallelProjectionOn();
	this->Renderer->ResetCamera();
	this->QVTK->GetRenderWindow()->Render();
}
void View3D::rotationOptions()
{
	//std::cout << "Rotation Updated " << std::endl;
	vtkCamera *cam = this->Renderer->GetActiveCamera();
	cam->SetFocalPoint(0,0,0);
	cam->SetPosition(0,0,1);
	cam->ComputeViewPlaneNormal();
	double x = 0; double y = 1; double z = 0;
	cam->SetViewUp(x,y,z);

	double rollAngle = this->RollBox->value();
	double elevationAngle = this->ElevationBox->value();
	double azimuthAngle = this->AzimuthBox->value();

	//std::cout << projection_base.roll + rollAngle << " " << projection_base.azimuth + azimuthAngle << " " << projection_base.elevation + elevationAngle << std::endl;

	cam->Azimuth(projection_base.azimuth + azimuthAngle);
	cam->Elevation(projection_base.elevation + elevationAngle);	
	cam->Roll(projection_base.roll + rollAngle);
	cam->OrthogonalizeViewUp();

	this->Renderer->ResetCamera();
	this->QVTK->GetRenderWindow()->Render();

	//std::cout << projection_base.roll << " " << projection_base.azimuth << " " << projection_base.elevation << std::endl;
}
void View3D::SetProjectionMethod(int style)
{
	if (renderMode == PROJECTION)
	{
		this->projectionStyle = style;
		for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
		{
			if (this->ImageActors->is2D(i))
			{
				this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(i));
				this->Renderer->AddActor(this->ImageActors->createProjection(i,this->projectionStyle,this->projection_axis));
			}
		}//end for num images
	}
	if (renderMode == SLICER)
	{
		for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
		{
			if (this->ImageActors->is2D(i))
			{	
				this->Renderer->RemoveActor(this->ImageActors->GetImageSlice(i));
				this->Renderer->AddActor(this->ImageActors->GetImageSlice(i));
			}
		}//end for num images
	}
	this->QVTK->GetRenderWindow()->Render();
}

//projection image
void View3D::CreateActors()
{
	this->LineActor = vtkSmartPointer<vtkActor>::New();
	this->LineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	this->UpdateLineActor();
	this->LineActor->SetPickable(1);
	this->Renderer->AddActor(this->LineActor);

	this->UpdateBranchActor();
	this->Renderer->AddActor(this->BranchActor);
	this->QVTK->GetRenderWindow()->Render();
	for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	{
		if (this->ImageActors->isRayCast(i))
		{
			if (!this->viewIn2D)
			{
				this->Renderer->AddVolume(this->ImageActors->RayCastVolume(i));
				this->ImageActors->setRenderStatus(i, true);
				this->RaycastBar->show();
				renderMode = RAYCAST;
			}else
			{
				this->Renderer->AddActor(this->ImageActors->createProjection(i, this->projectionStyle,this->projection_axis));
				this->ImageActors->setIs2D(i, true);
				renderMode = PROJECTION;

			}
			this->Renderer->AddActor(ImageActors->GetImageSlice(i));
			this->Renderer->RemoveActor(ImageActors->GetImageSlice(i));
			createSlicerSlider();
		}
		else
		{
			this->Renderer->AddActor(this->ImageActors->ContourActor(i));
			this->ImageActors->setRenderStatus(i, true);
		}
		//this->ImageActors->SetSliceCreate(i,false);
	}
	//sphere is used to mark the picks
	this->CreateSphereActor();
	Renderer->AddActor(this->SphereActor);
	this->axes = vtkSmartPointer<vtkAxesActor>::New();
	this->UCSMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
	this->UCSMarker->SetOutlineColor(0.9300, 0.5700, 0.1300 );
	this->UCSMarker->SetOrientationMarker(this->axes);
	this->UCSMarker->SetInteractor(this->Interactor);
	this->UCSMarker->SetViewport(0,0,.1,.1);
	this->UCSMarker->SetEnabled(1);
	//this->UCSMarker->InteractiveOn();
	this->QVTK->GetRenderWindow()->Render();
}

void View3D::removeImageActors()
{
	for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	{  
		this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(i));
		this->ImageActors->setIs2D(i, false);
		this->Renderer->RemoveVolume(this->ImageActors->GetRayCastVolume(i));
		this->Renderer->RemoveActor(this->ImageActors->GetContourActor(i));

		/*if(this->ImageActors->is2D(i))
		{
			this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(i));
			this->ImageActors->setIs2D(i, false);
		}
		else if (this->ImageActors->isRayCast(i))
		{
			this->Renderer->RemoveVolume(this->ImageActors->GetRayCastVolume(i));
		} 
		else
		{
			this->Renderer->RemoveActor(this->ImageActors->GetContourActor(i));
		}			*/

		this->ImageActors->setRenderStatus(i, false);
		//std::cout << "Turning off item " << i << std::endl;
		QTableWidgetItem *offItem = new QTableWidgetItem(tr("off"));
		offItem->setFlags(offItem->flags() & (~Qt::ItemIsEditable));
		this->projectFilesTable->setItem(i,2,offItem);

	}//end num images
	if (this->RaycastBar->isVisible())
	{
		this->RaycastBar->hide();
	}
	if (this->SlicerBar->isVisible())
	{
		this->SlicerBar->hide();
	}
	this->QVTK->GetRenderWindow()->Render();
}

//void View3D::raycastToSlicer()
//{
//	if (!this->viewIn2D)
//	{
//		for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
//		{  
//			if ((this->ImageActors->getRenderStatus(i))&&(this->ImageActors->isRayCast(i)))
//			{
//			  //this->Renderer->AddActor(this->ImageActors->CreateImageSlice(i));
//			  this->Renderer->AddActor(this->ImageActors->createProjection(i,this->projectionStyle,this->projection_axis));
//			  this->ImageActors->setIs2D(i, true);
//			renderMode = PROJECTION;
//			  this->Renderer->RemoveVolume(this->ImageActors->GetRayCastVolume(i));
//			  this->ImageActors->setRenderStatus(i, false);
//				/***************************************************************/
//				QTableWidgetItem *Item2D = new QTableWidgetItem(tr("2d"));
//				Item2D->setFlags(Item2D->flags() & (~Qt::ItemIsEditable));
//				this->projectFilesTable->setItem(i,3,Item2D);
//				/***************************************************************/
//			}
//		}//end num images
//		this->RacastBar->toggleViewAction()->setDisabled(1);
//		if (this->RacastBar->isVisible())
//		{
//			this->RacastBar->hide();
//		}
//		//this->SlicerBar->show();
//		//this->QVTK->GetRenderWindow()->Render();
//		this->chooseInteractorStyle(1);
//		this->viewIn2D = true;
//	}//end if 3d to 2d
//	else
//	{
//		for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
//		{  
//			if (this->ImageActors->is2D(i))
//			{
//				//this->Renderer->RemoveActor(this->ImageActors->GetImageSlice(i));
//				this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(i));
//				this->ImageActors->setIs2D(i, false);
//				this->Renderer->AddVolume(this->ImageActors->RayCastVolume(i));
//				this->ImageActors->setRenderStatus(i, true);
//				/***************************************************************/
//				QTableWidgetItem *Item3D = new QTableWidgetItem(tr("3d"));
//				Item3D->setFlags(Item3D->flags() & (~Qt::ItemIsEditable));
//				QFont font;
//				font.setBold(true);
//				Item3D->setFont(font);
//				this->projectFilesTable->setItem(i,3,Item3D);
//				/***************************************************************/
//			}
//		}
//		this->RacastBar->toggleViewAction()->setDisabled(0);
//		if (this->SlicerBar->isVisible())
//		{
//			this->SlicerBar->hide();
//		}
//		this->RacastBar->show();
//		this->chooseInteractorStyle(0);
//		this->viewIn2D = false;
//	}//end else 2d to 3d
//}

void View3D::setSlicerMode()
{
	for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	{
		ClearRenderer(i);
		if (projectFilesTableCreated)
		{
			//if (this->projectFilesTable->item(i,2)->text() == "on") //slice not connected to table
			{
				Renderer->AddActor(ImageActors->GetImageSlice(i));
				ImageActors->setRenderStatus(i, false);
			}
		}
		else
		{
			Renderer->AddActor(ImageActors->GetImageSlice(i));
			ImageActors->setRenderStatus(i, false);
		}
	}

	this->QVTK->GetRenderWindow()->Render();
	//std::cout << "Setting mode slicer" << std::endl;
	if (!SlicerBarCreated)
		this->createSlicerSlider();
	else
		this->SlicerBar->show();
	this->chooseInteractorStyle(3);
	this->setSlicerZValue(-1);
	renderMode = SLICER;
}

void View3D::setProjectionMode()
{
	for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	{
		ClearRenderer(i);
		if (projectFilesTableCreated)
		{
			if (this->projectFilesTable->item(i,2)->text() == "on")
			{
				this->Renderer->AddActor(this->ImageActors->createProjection(i,this->projectionStyle,this->projection_axis));
			}
		}
		else
			this->Renderer->AddActor(this->ImageActors->createProjection(i,this->projectionStyle,this->projection_axis));

		//this->ImageActors->setIs2D(i, true); //this line incompatible with multiple mode renderer
		this->ImageActors->setRenderStatus(i, false);
		
		//Incompatible with slicer/projection upgrade
		/***************************************************************/
		QTableWidgetItem *Item2D = new QTableWidgetItem(tr("2d"));
		Item2D->setFlags(Item2D->flags() & (~Qt::ItemIsEditable));
		this->projectFilesTable->setItem(i,3,Item2D);
		//std::cout << "i: " << i << "make 2D." << std::endl;
		/***************************************************************/
	}

	//this->QVTK->GetRenderWindow()->Render();
	this->chooseInteractorStyle(1);
	//std::cout << "Setting mode projection" << std::endl;
	renderMode = PROJECTION;
}

void View3D::setRaycastMode()
{
	for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	{  
		ClearRenderer(i);			
		//this->ImageActors->setIs2D(i, false); //this line incompatible with multiple mode renderer
		if (projectFilesTableCreated)
		{
			if (this->projectFilesTable->item(i,2)->text() == "on")
			{
				this->Renderer->AddVolume(this->ImageActors->RayCastVolume(i));
			}
		}
		else
			this->Renderer->AddVolume(this->ImageActors->RayCastVolume(i));
			
		this->ImageActors->setRenderStatus(i, true);

		//Incompatible with slice/projection upgrade
		/***************************************************************/
		QTableWidgetItem *Item3D = new QTableWidgetItem(tr("3d"));
		Item3D->setFlags(Item3D->flags() & (~Qt::ItemIsEditable));
		QFont font;
		font.setBold(true);
		Item3D->setFont(font);
		this->projectFilesTable->setItem(i,3,Item3D);
		/***************************************************************/
	}
	this->RaycastBar->toggleViewAction()->setDisabled(0);

	this->RaycastBar->show();
	this->chooseInteractorStyle(0);
	this->viewIn2D = false;
	//std::cout << "Setting mode raycast" << std::endl;
	
	renderMode = RAYCAST;
}

void View3D::ClearRenderer(int i)
{
	if (this->SlicerBar->isVisible())
		this->SlicerBar->hide();
	Renderer->RemoveActor(ImageActors->GetImageSlice(i));
	Renderer->RemoveActor(ImageActors->GetProjectionImage(i));
	if (this->RaycastBar->isVisible())
		this->RaycastBar->hide();
	Renderer->RemoveVolume(ImageActors->GetRayCastVolume(i));

	//if (renderMode == SLICER)
	//{
	//	Renderer->RemoveActor(ImageActors->GetImageSlice(i));
	//	if (this->SlicerBar->isVisible())
	//		this->SlicerBar->hide();
	//	std::cout << "Removing slicer" << std::endl;
	//}
	//else if (renderMode == PROJECTION)
	//{
	//	Renderer->RemoveActor(ImageActors->GetProjectionImage(i));
	//	std::cout << "Removing projection" << std::endl;
	//}
	//else if (renderMode == RAYCAST)
	//{
	//	Renderer->RemoveVolume(ImageActors->GetRayCastVolume(i));
	//	if (this->RaycastBar->isVisible())
	//		this->RaycastBar->hide();
	//	this->RaycastBar->toggleViewAction()->setDisabled(1);
	//	std::cout << "Removing raycast" << std::endl;
	//}
	//else if (renderMode == SLICERRAYCAST) //Slicer with raycast
	//{
	//}
}

void View3D::focusOn()
{
	std::vector<CellTrace*> cellsSelected = this->CellModel->GetSelectedCells();
	std::vector<TraceLine*> traceSelected = this->TreeModel->GetSelectedTraces();
	//////////////////////////////////////

	
	if (cellsSelected.size() >=1)
	{
		this->FocusOnCell(cellsSelected.back());
	}
	else if (this->pointer3d->GetEnabled())
	{		
		double newPT[3];
		this->pointer3d->GetPosition(newPT);
		this->setRenderFocus(newPT, 3);
	}
	else if (traceSelected.size() >= 1)
	{
		double traceBounds[6];
		traceSelected.back()->getEndPtBounds(traceBounds);
		this->setRenderFocus(traceBounds, 6);
	}
	else
	{
		double test [6];
		this->Renderer->ComputeVisiblePropBounds(test);
		this->setRenderFocus(test, 6);
		//std::cout <<" test set render focus \n";
	}
}
void View3D::FocusOnCell(CellTrace* SelectedCell)
{
	double somaCoord[3];
	SelectedCell->getSomaCoord(somaCoord);
	this->pointer3DLocation(somaCoord);
	// render after move curser
	double cellBounds[6];
	SelectedCell->getCellBounds(cellBounds);
	if ((cellBounds[1]-cellBounds[0]) > 3)
	{
		this->setRenderFocus(cellBounds, 6);
	}//zoom to entire cell
	else
	{
		double sceneBounds[6];
		//this->setRenderFocus(somaCoord, 3);
		//this->ImageActors->getImageBounds(cellBounds);
		this->Renderer->ComputeVisiblePropBounds(sceneBounds);
		this->setRenderFocus(sceneBounds,6);
	}//focus on soma coord 
}
void View3D::setRenderFocus(double renderBounds[], int size)
{	
	if (size <= 3)
	{
		this->Renderer->GetActiveCamera()->SetFocalPoint(renderBounds);
		//this->Renderer->GetActiveCamera()->SetPosition(0,0,1);
		this->Renderer->GetActiveCamera()->ComputeViewPlaneNormal();
		this->Renderer->GetActiveCamera()->SetViewUp(0,1,0);
		this->Renderer->GetActiveCamera()->OrthogonalizeViewUp();
		this->Renderer->GetActiveCamera()->ParallelProjectionOn();
		//this->Renderer->ResetCamera();
	}
	else
	{
		this->Renderer->ResetCamera(renderBounds);
	}
	this->QVTK->GetRenderWindow()->Render();
}
void View3D::createSlicerSlider()
{
	double* bounds = this->ImageActors->getSliceBounds();
	double upperBound = bounds[5]-(bounds[4]+1);

	this->SlicerBar = new QToolBar("Slicer", this);
	this->SlicerBar->setAllowedAreas(Qt::RightToolBarArea | Qt::LeftToolBarArea);
	this->addToolBar(Qt::RightToolBarArea, this->SlicerBar);

	QLabel * SliceThicknessLabel = new QLabel("Slice Thickness",this);
	this->SliceThicknessSpinBox = new QSpinBox(this);
	this->SliceThicknessSpinBox->setRange(1,upperBound);

	this->SliceSpinBox = new QSpinBox(this);
	this->SliceSpinBox->setRange(1,upperBound);

	this->SliceSlider = new QSlider(Qt::Vertical);
	this->SliceSlider->setSingleStep(1);
	this->SliceSlider->setRange(1,upperBound);
	this->SliceSlider->setTickInterval(5);
	this->SliceSlider->setTickPosition(QSlider::TicksRight);
	this->SliceSlider->setValue(1);
	connect(this->SliceSlider, SIGNAL(valueChanged(int)), this->SliceSpinBox, SLOT(setValue(int)));
	connect(this->SliceSpinBox, SIGNAL(valueChanged(int)), this->SliceSlider, SLOT(setValue(int)));
	connect(this->SliceSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setSlicerZValue(int)));
	connect(this->SliceThicknessSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setSliceThickness(int)));
	this->SlicerBar->addWidget(this->SliceSpinBox);
	this->SlicerBar->addWidget(this->SliceSlider);
	this->SlicerBar->addWidget(SliceThicknessLabel);
	this->SlicerBar->addWidget(this->SliceThicknessSpinBox);
	this->SlicerBar->hide();
	this->SlicerBarCreated = true;
}
void View3D::setSlicerZValue(int value)
{
	//for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	//{
	//	if(this->ImageActors->is2D(i))
	//	{
	//		std::cout << "setSlicerZValue called" << std::endl;
	//		this->Renderer->RemoveActor(this->ImageActors->GetProjectionImage(i));
	//		//this->Renderer->RemoveActor(this->ImageActors->GetImageSlice(i));
	//		this->Renderer->RemoveViewProp(this->ImageActors->GetImageSlice(i));
	//		//this->ImageActors->SetSliceNumber(i, value);
	//		//this->Renderer->AddActor(this->ImageActors->GetImageSlice(i));
	//		this->Renderer->AddViewProp(this->ImageActors->GetImageSlice(i));
	//		vtkInteractorStyleImage * styleImage = vtkInteractorStyleImage::New();
	//		styleImage->SetInteractionModeToImage3D();
	//		this->Interactor->SetInteractorStyle(styleImage);
	//	}
	//}
	//this->QVTK->GetRenderWindow()->Render();

	//for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
	//{
	//	if(this->ImageActors->is2D(i))
	//	{
	//		this->Renderer->RemoveViewProp(this->ImageActors->GetImageSlice(i));
	//		//this->Renderer->RemoveActor(ImageActors->GetImageSlice(i));
	//		//this->Renderer->AddViewProp(ImageActors->GetImageSlice(i));
	//	}
	//}	
	double* bounds = this->ImageActors->getSliceBounds();
	double image_center_x = (bounds[0]+bounds[1])/2;
	double image_center_y = (bounds[2]+bounds[3])/2;
	//double image_center_z = (double)bounds[5]-(bounds[5]-(bounds[4]+1))*value/100.0;
	double image_center_z = bounds[5]-abs(value);

	vtkCamera *cam = this->Renderer->GetActiveCamera();
		double prevPosition[3];
		cam->GetPosition(prevPosition);
		double prevFocalPoint[3];
		cam->GetFocalPoint(prevFocalPoint);

	//std::cout << "Z Value: " << value << std::endl;
	if (value == -1)
	{
		//std::cout << "image_center_x: "  << image_center_x << " image_center_y = " << image_center_y << " image_center_z " << image_center_z << std::endl;

		double curDistance = cam->GetDistance();
		double ViewUp[3];
		cam->GetViewUp(ViewUp);

		//cam->SetFocalPoint(prevFocalPoint[0],prevFocalPoint[1],image_center_z);
		cam->SetFocalPoint(image_center_x,image_center_y,image_center_z);
		//cam->SetFocalPoint(0,0,image_center_z);

		cam->SetPosition(image_center_x,image_center_y,image_center_z+1);
		//cam->SetPosition(prevFocalPoint[0],prevFocalPoint[1],image_center_z+1);
		cam->SetViewUp(0,1,0);
		cam->ComputeViewPlaneNormal();
		cam->OrthogonalizeViewUp();

		this->Renderer->ResetCamera();
		//cam->SetFocalPoint(image_center_x,image_center_y,image_center_z);
		cam->SetFocalPoint(prevFocalPoint[0],prevFocalPoint[1],image_center_z);
		cam->SetPosition(prevPosition);
		cam->SetViewUp(ViewUp);
		cam->ComputeViewPlaneNormal();
		cam->OrthogonalizeViewUp();
		//cam->SetDistance(curDistance);
		this->QVTK->GetRenderWindow()->Render();
	}
	else
	{
		cam->SetFocalPoint(prevFocalPoint[0],prevFocalPoint[1],image_center_z);
		//std::cout << "Position: " << prevPosition[0] << " " << prevPosition[1] << std::endl;
		//cam->SetPosition(prevPosition);
		this->QVTK->GetRenderWindow()->Render();
		//std::cout << "SlicerZvalue changed" << std::endl;
	}

}
void View3D::setSliceThickness(int sliceThickness)
{
	this->ImageActors->SetSliceThickness(sliceThickness - 1);
}
void View3D::ToggleColorByTrees()
{
	bool colorByTrees = !this->tobj->GetColorByTrees();
	this->tobj->SetColorByTrees(colorByTrees);
	this->ColorByTreesAction->setChecked(colorByTrees);
	if(colorByTrees)
	{
		this->tobj->UpdateRootToTree();
	}
	this->tobj->RecolorTraces();
	this->UpdateLineActor();
	this->Rerender();
}

void View3D::CreateSphereActor()
{
	this->Sphere = vtkSmartPointer<vtkSphereSource>::New();
	this->Sphere->SetRadius(3);
	this->SphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	this->SphereMapper->SetInput(this->Sphere->GetOutput());
	this->SphereMapper->GlobalImmediateModeRenderingOn();

	this->SphereActor = vtkSmartPointer<vtkActor>::New();
	this->SphereActor->SetMapper(this->SphereMapper);
	this->SphereActor->GetProperty()->SetOpacity(.3);
	this->SphereActor->VisibilityOff();
	this->SphereActor->SetPickable(0);            //dont want to pick the sphere itself
	this->pointer3d = vtkSmartPointer<vtkPointWidget>::New();
	double sceneBounds[6];
	this->Renderer->ComputeVisiblePropBounds(sceneBounds);
	this->pointer3d->PlaceWidget(sceneBounds);
	//this->pointer3d->PlaceWidget(-60000, 60000,-60000, 60000,-60000, 60000);
	this->pointer3d->SetInteractor(this->QVTK->GetInteractor());
	this->pointer3d->AllOff();
}

void View3D::createRayCastSliders()
{
	this->RaycastBar = new QToolBar("RayCast Tools", this);
	this->RaycastBar->setAllowedAreas(Qt::BottomToolBarArea);
	this->RaycastBar->setMovable(false);
	this->addToolBar(Qt::BottomToolBarArea,this->RaycastBar);
	this->RaycastBar->setToolTip("Racaster settings");
	//functions to control raycast opacity 
	this->OpacitySpin = new QSpinBox(this);
	this->OpacitySpin->setRange(0,250);

	this->OpacityValueSpin = new QDoubleSpinBox(this);
	this->OpacityValueSpin->setRange(0,1);
	this->OpacityValueSpin->setSingleStep(.01);
	this->ImageActors->setOpacityValue(this->TraceEditSettings.value("RayCast/OpacityValue", .1).toDouble() );
	this->OpacityValueSpin->setValue(this->ImageActors->getOpacityValue());
	connect (this->OpacityValueSpin, SIGNAL(valueChanged(double)), this, SLOT(RayCastOpacityValueChanged(double)));

	this->OpacitySlider = new QSlider(Qt::Horizontal);
	this->OpacitySlider->setRange(0,250);
	this->OpacitySlider->setSingleStep(1);
	this->OpacitySlider->setTickInterval(5);
	this->OpacitySlider->setTickPosition(QSlider::TicksAbove);
	connect (this->OpacitySlider, SIGNAL(valueChanged(int)), this->OpacitySpin, SLOT(setValue(int)));
	this->ImageActors->setOpacity(this->TraceEditSettings.value("RayCast/Opacity", 50).toInt() );
	this->OpacitySlider->setValue((int) this->ImageActors->getOpacity());
	connect (this->OpacitySpin, SIGNAL(valueChanged(int)), this->OpacitySlider, SLOT(setValue(int)));
	connect (this->OpacitySpin, SIGNAL(valueChanged(int)), this, SLOT(RayCastOpacityChanged(int)));

	//functions to control raycast Brightness
	this->BrightnessSpin = new QSpinBox(this);
	this->BrightnessSpin->setRange(0,250);

	this->BrightnessSlider = new QSlider(Qt::Horizontal);
	this->BrightnessSlider->setRange(0,255);
	this->BrightnessSlider->setSingleStep(1);
	this->BrightnessSlider->setTickInterval(5);
	this->BrightnessSlider->setTickPosition(QSlider::TicksAbove);
	connect (this->BrightnessSlider, SIGNAL(valueChanged(int)), this->BrightnessSpin, SLOT(setValue(int)));
	this->ImageActors->setBrightness(this->TraceEditSettings.value("RayCast/Brightness", 150).toInt() );
	this->BrightnessSlider->setValue(this->ImageActors->getBrightness());
	connect (this->BrightnessSpin, SIGNAL(valueChanged(int)), this->BrightnessSlider, SLOT(setValue(int)));
	connect (this->BrightnessSpin, SIGNAL(valueChanged(int)), this , SLOT(RayCastBrightnessChanged(int)));
	//add the widgets to the bar
	this->RaycastBar->addWidget(new QLabel("Opacity Threshold"));
	this->RaycastBar->addWidget(this->OpacitySpin);
	this->RaycastBar->addWidget(this->OpacitySlider);
	this->RaycastBar->addWidget(new QLabel("Opacity Value"));
	this->RaycastBar->addWidget(this->OpacityValueSpin);
	this->RaycastBar->addSeparator();
	this->RaycastBar->addWidget(new QLabel("Brightness"));
	this->RaycastBar->addWidget(this->BrightnessSpin);
	this->RaycastBar->addWidget(this->BrightnessSlider);
	this->DataViews->addAction(this->RaycastBar->toggleViewAction());
	if(this->ImageActors->NumberOfImages() < 1)
	{
		this->RaycastBar->hide();
	}
}

void View3D::RayCastBrightnessChanged(int value)
{
	this->ImageActors->setBrightness(value);
	this->TraceEditSettings.setValue("RayCast/Brightness", value);
	this->QVTK->GetRenderWindow()->Render();
}

void View3D::RayCastOpacityChanged(int value)
{
	this->ImageActors->setOpacity(value);
	this->TraceEditSettings.setValue("RayCast/Opacity", value);
	this->QVTK->GetRenderWindow()->Render();

}

void View3D::RayCastOpacityValueChanged(double value)
{
	this->ImageActors->setOpacityValue(value);
	this->TraceEditSettings.setValue("RayCast/OpacityValue", value);
	this->QVTK->GetRenderWindow()->Render();
}

void View3D::EditHelp()
{
	//will write help documentation here
}

void View3D::About()
{
	QString version = QString::number(CPACK_PACKAGE_VERSION_MAJOR);
	version += ".";
	version += QString::number(CPACK_PACKAGE_VERSION_MINOR);
	version += ".";
	version += QString::number(CPACK_PACKAGE_VERSION_PATCH);
	QMessageBox::about(this, tr("About Application"),
		QCoreApplication::organizationName() + "\n" + 
		QCoreApplication::organizationDomain() + "\n" + 
		QCoreApplication::applicationName() + "\n Version:\t" + 
		version+
		"\nThe Farsight Trace Editor is intended to provide validation through editing\n"
		"The linked space provides group editing and helps automate many tasks\n"
		"Licensed under the Apache License, Version 2.0 (the 'License');\n"
		"you may not use this file except in compliance with the License.\n"
		"You may obtain a copy of the License at\n"

		"http://www.apache.org/licenses/LICENSE-2.0"

		"\nUnless required by applicable law or agreed to in writing,\n"
		"software distributed under the License is distributed on an 'AS IS' BASIS,\n"
		"WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
		"See the License for the specific language governing permissions and\n"
		"limitations under the License. \n");
}

/* update settings */
void View3D::ShowSettingsWindow()
{
	//make sure the values in the input fields are up-to-date
	this->MaxGapField->setValue(this->tobj->gapMax);
	this->GapToleranceField->setValue(this->tobj->gapTol);
	this->LineLengthField->setValue(this->SmallLineLength);
	this->ColorValueField->setValue(this->SelectColor);
	this->LineWidthField->setValue(this->lineWidth);
	this->BackgroundRBox->setValue(this->backColorR);
	this->BackgroundGBox->setValue(this->backColorG);
	this->BackgroundBBox->setValue(this->backColorB);
	this->markTraceBits->setChecked(this->renderTraceBits);
	this->SettingsWidget->show();
}

void View3D::ApplyNewSettings()
{
	this->tobj->gapMax = this->MaxGapField->text().toInt();
	this->tobj->gapTol = this->GapToleranceField->value();
	this->SmallLineLength = (float)this->LineLengthField->value();
	this->SelectColor = this->ColorValueField->value();
	this->lineWidth = (float)this->LineWidthField->value();
	this->backColorR = this->BackgroundRBox->value();
	this->backColorG = this->BackgroundGBox->value();
	this->backColorB = this->BackgroundBBox->value();
	this->Renderer->SetBackground(this->backColorR,this->backColorG,this->backColorB);
	this->statusBar()->showMessage(tr("Applying new settings"),3000);
	this->TraceEditSettings.setValue("mainWin/gapTol", this->tobj->gapTol ) ;
	this->TraceEditSettings.setValue("mainWin/gapMax", this->tobj->gapMax);
	this->TraceEditSettings.setValue("mainWin/smallLine", this->SmallLineLength);
	this->TraceEditSettings.setValue("mainWin/selectColor", this->SelectColor);
	this->TraceEditSettings.setValue("mainWin/LineWidth", this->lineWidth);
	this->TraceEditSettings.setValue("mainWin/ColorR", this->backColorR);
	this->TraceEditSettings.setValue("mainWin/ColorG", this->backColorG);
	this->TraceEditSettings.setValue("mainWin/ColorB", this->backColorB);
	this->TraceEditSettings.sync();
	this->renderTraceBits = this->markTraceBits->isChecked();
	this->poly_line_data->Modified();
	this->updateTraceSelectionHighlights();
	this->QVTK->GetRenderWindow()->Render();  
}

void View3D::HideSettingsWindow()
{
	//this->SettingsWidget->hide();
	this->settingsDock->hide();
}

/*  picking */
void View3D::PickCell(vtkObject* caller, unsigned long event, void* clientdata, void* callerdata)
{ /*  PickPoint allows fot the point id and coordinates to be returned 
  as well as adding a marker on the last picked point
  R_click to select point on line  */
	//printf("Entered pickCell\n");
	View3D* view = (View3D*)clientdata;       //acess to view3d class so view-> is used to call functions

	int *pos = view->Interactor->GetEventPosition();
	view->Interactor->GetPicker()->Pick(pos[0],pos[1],0.0,view->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
	vtkCellPicker *cell_picker = (vtkCellPicker *)view->Interactor->GetPicker();
	if (cell_picker->GetCellId() == -1) 
	{
		view->SphereActor->VisibilityOff();     //not working quite yet but sphere will move
	}
	else if(cell_picker->GetViewProp()!=NULL) 
	{
		//double selPt[3];
		double pickPos[3];
		view->CellPicker->GetPickPosition(pickPos);    //this is the coordinates of the pick
		//view->cell_picker->GetSelectionPoint(selPt);    //not so usefull but kept
		unsigned int cell_id = cell_picker->GetCellId();  
		view->SelectedTraceIDs.push_back(cell_id);
		TraceLine *tline = reinterpret_cast<TraceLine*>(view->tobj->hashc[cell_id]);
		if (view->tobj->Gaps.size() < 1)
		{
			view->TreeModel->SelectByIDs(tline->GetId());
			//view->HighlightSelected(tline, view->SelectColor);
			//tline->Getstats();              //prints the id and end coordinates to the command prompt 
			view->SphereActor->SetPosition(pickPos);    //sets the selector to new point
			view->pointer3DLocation(pickPos);
			view->SphereActor->VisibilityOn();      //deleteTrace can turn it off 
			view->poly_line_data->Modified();
		}
		else
		{ //int size = view->tobj->Gaps.size();
			int id = tline->GetId();
			view->MergeGaps->SelectbyTraceID(id);
			view->poly_line_data->Modified();
		}
		//update the head Qt view here too...
	}// end if pick
	view->QVTK->GetRenderWindow()->Render();             //update the render window
}

void View3D::showPTin3D(double value)
{
	//bool ok = false;
	double pos[3];
	/*while (!ok)
	{
	pos[0] = QInputDialog::getDouble(this, tr("set x"), tr("x value"), 0, -60000, 60000, 1, &ok);
	}
	ok = false;
	while (!ok)
	{
	pos[1] = QInputDialog::getDouble(this, tr("set y"), tr("y value"), 0, -60000, 60000, 1, &ok);
	}
	ok = false;
	while (!ok)
	{
	pos[2] = QInputDialog::getDouble(this, tr("set z"), tr("z value"), 0, -60000, 60000, 1, &ok);
	}*/
	pos[0] = this->posX->value();
	pos[1] = this->posY->value();
	pos[2] = this->posZ->value();
	this->SphereActor->SetPosition(pos );
	this->SphereActor->VisibilityOn();
	this->pointer3DLocation(pos);
	this->QVTK->GetRenderWindow()->Render();
}

void View3D::getPosPTin3D()
{
	//this->cursor3DDock->show();
	this->pointer3d->SetEnabled(1);//if not shown
	double newPT[3];
	this->pointer3d->GetPosition(newPT);
	this->posX->blockSignals(1);
	this->posY->blockSignals(1);
	this->posZ->blockSignals(1);
	this->posX->setValue(newPT[0]);
	this->posY->setValue(newPT[1]);
	this->posZ->setValue(newPT[2]);

	this->posX->blockSignals(0);
	this->posY->blockSignals(0);
	this->posZ->blockSignals(0);
	//this->pointer3DPos = newPT;
}

void View3D::pointer3DLocation(double pos[])
{
	if (this->ShowPointer3DDefault)
	{
		this->pointer3d->SetPosition(pos);
		this->pointer3d->SetEnabled(1);
	}
	this->posX->blockSignals(1);
	this->posY->blockSignals(1);
	this->posZ->blockSignals(1);

	this->posX->setValue(pos[0]);
	this->posY->setValue(pos[1]);
	this->posZ->setValue(pos[2]);

	this->posX->blockSignals(0);
	this->posY->blockSignals(0);
	this->posZ->blockSignals(0);
}

void View3D::setPTtoSoma()
{
	if(this->stems.size() <2)
	{
		this->stems = this->TreeModel->GetSelectedTraces();
		if(this->stems.size() <2)
		{
			return;
		}
	}
	if (this->pointer3d->GetEnabled())
	{	
		double newPT[3];
		this->pointer3d->GetPosition(newPT);
		this->tobj->createSomaFromPT(newPT,this->stems);
		this->stems.clear();
		this->ClearSelection();
		this->Rerender();
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
	}
}

void View3D::setUsePointer(int i)
{
	if (this->ShowPointer->isChecked())
	{
		this->ShowPointer3DDefault = true;
		this->pointer3d->SetEnabled(1);
	}
	else
	{
		this->ShowPointer3DDefault = false;
		this->pointer3d->SetEnabled(0);
	}
}

void View3D::ArunCenterline()
{
	//stub for now to check if it works
	std::cout<<"What is this?  Is this a working button?  WHY YES I BELIEVE IT IS." <<std::endl;
}

void View3D::createNewTraceBit()
{
	if (this->pointer3d->GetEnabled())
	{
		double newPT[3];
		this->pointer3d->GetPosition(newPT);
		//this->stems = this->TreeModel->GetSelectedTraces();
		if(this->stems.size() <1)
		{
			this->stems = this->TreeModel->GetSelectedTraces();
		}//check for selected traces
		this->ClearSelection();
		if (this->stems.size() <1)
		{
			TraceBit tbit= this->tobj->CreateBitAtCoord(newPT);
			TraceLine *TraceCreated = this->tobj->CreateTraceFromBit(tbit);
			this->stems.push_back(TraceCreated);
		}
		else if(this->stems.size() == 1)
		{
			this->tobj->ExtendTraceTo(this->stems.at(0), newPT);
		}//end extend trace
		else if(this->stems.size() > 1)
		{
			this->setPTtoSoma();
			this->stems.clear();
		}//end create soma
		this->Rerender();
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		//this->TreeModel->s
		if(this->stems.size() == 1)
		{
			this->TreeModel->SelectByIDs(this->stems.at(0)->GetId());
			QMessageBox Myquestion;
			Myquestion.setText("Selected line:  " 
				+ QString::number(this->stems.at(0)->GetId()));
			Myquestion.setInformativeText("Continue extending this line?" );
			Myquestion.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			Myquestion.setDefaultButton(QMessageBox::Yes);
			int ret = Myquestion.exec();
			switch (ret) 
			{ 
			case QMessageBox::Yes:
				{
					this->pointer3d->SetEnabled(1);
				}
				break;
			case QMessageBox::No:
				{
					this->stems.clear();
				}
				break;
			}
		}//end stems size = 1
	}
}
void View3D::AddROIPoint()
{
	if (this->pointer3d->GetEnabled())
	{	
		double* newPT = new double[3];
		this->pointer3d->GetPosition(newPT);
		//std::cout << "adding point xyz " << newPT[0] << " " << newPT[1] << " " << newPT[2] << " \n" ;
		this->ROIPoints.push_back(newPT);
		if (this->ROIPoints.size() >= 3)
		{
			this->ExtrudeROIButton->setEnabled(true);
		}
	}
}
void View3D::DrawROI()
{
	
	if (this->ROIPoints.size() < 3)
	{
		std::cout<< "not enough points\n";
		return;
	}

	//// Add the points to a vtkPoints object
	vtkSmartPointer<vtkPoints> vtkROIpoints = vtkSmartPointer<vtkPoints>::New();
	
	std::vector<double*>::iterator ROIPoints_iter;

	int count = 0;
	this->EditLogDisplay->append(QString("\nDefining ROI\n X\tY\tZ"));
	for (ROIPoints_iter = ROIPoints.begin(); ROIPoints_iter != ROIPoints.end(); ROIPoints_iter++)
	{
		vtkROIpoints->InsertNextPoint(*ROIPoints_iter);
		this->EditLogDisplay->append((QString("%1\t%2\t%3").arg((*ROIPoints_iter)[0]).arg((*ROIPoints_iter)[1]).arg((*ROIPoints_iter)[2])));
		delete *ROIPoints_iter;
		*ROIPoints_iter = NULL;
		count++; //size of polygon vertex
	}

	//
	vtkSmartPointer<vtkPolygon> ROI_Poly = vtkSmartPointer<vtkPolygon>::New();
	ROI_Poly->GetPointIds()->SetNumberOfIds(count);
	for (int i =0; i< count; i++)
	{
		ROI_Poly->GetPointIds()->SetId(i,i);
	}

	//build cell array
	vtkSmartPointer<vtkCellArray> ROI_Poly_CellArray = vtkSmartPointer<vtkCellArray>::New();
	ROI_Poly_CellArray->InsertNextCell(ROI_Poly);

	// Create a polydata to store everything in
	vtkSmartPointer<vtkPolyData> ROIpolydata = vtkSmartPointer<vtkPolyData>::New();
	ROIpolydata->SetPoints(vtkROIpoints);
	ROIpolydata->SetPolys(ROI_Poly_CellArray);

	vtkSmartPointer<vtkLinearExtrusionFilter> extrude = vtkSmartPointer<vtkLinearExtrusionFilter>::New();
	extrude->SetInput( ROIpolydata);
	extrude->SetExtrusionTypeToNormalExtrusion();
	//extrude->SetVector(0, 0, 1 );
	extrude->SetScaleFactor (100);
	//extrude->CappingOff();
	extrude->Update();
	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> ROImapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	this->ROIExtrudedpolydata = extrude->GetOutput();
	ROImapper->SetInput(this->ROIExtrudedpolydata);

	this->ROIactor = vtkSmartPointer<vtkActor>::New();
	ROIactor->SetMapper(ROImapper);

	this->Renderer->AddActor(ROIactor);
	this->QVTK->GetRenderWindow()->Render();
	this->createNewROIPointButton->setEnabled(false);
	this->ExtrudeROIButton->setEnabled(false);
	this->CalculateDistanceToDeviceButton->setEnabled(true);
}

void View3D::CalculateDistanceToDevice()
{
	vtkSmartPointer<vtkCellLocator> cellLocator = vtkSmartPointer<vtkCellLocator>::New();
	cellLocator->SetDataSet(this->ROIExtrudedpolydata);
	cellLocator->BuildLocator();
	
	unsigned int cellCount= this->CellModel->getCellCount();
	if (cellCount >= 1)
	{
		for (unsigned int i = 0; i < cellCount; i++)
		{
			//double testPoint[3] = {500, 600, 50};
			double somaPoint[3];
			CellTrace* currCell = this->CellModel->GetCellAtNoSelection( i);
			currCell->getSomaCoord(somaPoint);
			//Find the closest points to TestPoint
			double closestPoint[3];//the coordinates of the closest point will be returned here
			double closestPointDist2; //the squared distance to the closest point will be returned here
			vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
			int subId; //this is rarely used (in triangle strips only, I believe)
			cellLocator->FindClosestPoint(somaPoint, closestPoint, cellId, subId, closestPointDist2);
			currCell->setDistanceToROI( std::sqrt(closestPointDist2), closestPoint[0], closestPoint[1], closestPoint[2]);
		}//end for cell count
		this->ShowCellAnalysis();
	}
	vtkIdType nucleiRowCount = this->nucleiTable->GetNumberOfRows();
	if (nucleiRowCount > 0)
	{
		// create new column
		vtkSmartPointer<vtkDoubleArray> column = vtkSmartPointer<vtkDoubleArray>::New();
		column->SetName("Distance_To_Device");
		column->SetNumberOfValues(nucleiRowCount);
		this->nucleiTable->AddColumn(column);
		// read coordinates 
		for (vtkIdType rowID = 0; rowID < nucleiRowCount; rowID++)
		{
			double centroid[3];
			centroid[0] = this->nucleiTable->GetValueByName(rowID,"centroid_x").ToDouble();
			centroid[1] = this->nucleiTable->GetValueByName(rowID,"centroid_y").ToDouble();
			centroid[2] = this->nucleiTable->GetValueByName(rowID,"centroid_z").ToDouble();

			double closestPoint[3];//the coordinates of the closest point will be returned here
			double closestPointDist2; //the squared distance to the closest point will be returned here
			vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
			int subId; //this is rarely used (in triangle strips only, I believe)
			cellLocator->FindClosestPoint(centroid, closestPoint, cellId, subId, closestPointDist2);

			this->nucleiTable->SetValueByName(rowID,"Distance_To_Device", vtkVariant(std::sqrt(closestPointDist2)));
		}
		QString nucleifileName = QFileDialog::getSaveFileName(
			this, tr("Save Nuclei feature File"), "", tr(".txt(*.txt)"));
		ftk::SaveTable(nucleifileName.toStdString(),this->nucleiTable);
	} // end nuclei dist to device
}
void View3D::readNucleiTable()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Open Nuclei Table", "",tr(".txt(*.txt)"));
	if (!fileName.isEmpty())
	{
		this->nucleiTable = ftk::LoadTable(fileName.toStdString());
	}
	this->AssociateCellToNucleiAction->setDisabled(false);
}
void View3D::AssociateNeuronToNuclei()
{
	unsigned int cellCount= this->CellModel->getCellCount();
	vtkIdType nucleiRowCount = this->nucleiTable->GetNumberOfRows();
	if ((cellCount >= 1)&&(nucleiRowCount > 0))
	{
		vtkIdType nucleiColSize = this->nucleiTable->GetNumberOfColumns();
		vtkIdType nucleiRowSize = this->nucleiTable->GetNumberOfRows();
		for (vtkIdType nucleiColIter = 0; nucleiColIter< nucleiColSize; nucleiColIter++)
		{
			this->CellModel->AddNewFeatureHeader(this->nucleiTable->GetColumnName(nucleiColIter));
		}
		for (unsigned int i = 0; i < cellCount; i++)
		{
			// search for nucli thats within radii of soma 
			double distance =0, somaRadii = 0;//,  x1, y1, z1; 
			double somaPoint[3];
			CellTrace* currCell = this->CellModel->GetCellAtNoSelection( i);
			currCell->getSomaCoord(somaPoint);
			somaRadii = currCell->SomaRadii;
			bool found = false;
			vtkIdType nucleiRowIter = 0;
			while (!found && (nucleiRowIter < nucleiRowSize))
			{
				double x, y, z, x2, y2, z2;
				x2 = this->nucleiTable->GetValueByName(nucleiRowIter,"centroid_x").ToDouble();
				y2 = this->nucleiTable->GetValueByName(nucleiRowIter,"centroid_y").ToDouble();
				z2 = this->nucleiTable->GetValueByName(nucleiRowIter,"centroid_z").ToDouble();
				x = pow((somaPoint[0] - x2),2);
				y = pow((somaPoint[1] - y2),2);
				z = pow((somaPoint[2] - z2),2);
				distance = sqrt(x +y +z);
				if (distance < somaRadii)
				{
					for (vtkIdType nucleiColIter = 0; nucleiColIter< nucleiColSize; nucleiColIter++)
					{
						vtkVariant colData = this->nucleiTable->GetValue(nucleiRowIter, nucleiColIter);
						currCell->addNewFeature(colData);
						/*OutputTable->SetValueByName(somaRowIter, colName, colData);*/
					}
					found = true;
				}
				nucleiRowIter++;
			}//end nuclei match search
			if (!found)
			{
				for (vtkIdType nucleiColIter = 0; nucleiColIter< nucleiColSize; nucleiColIter++)
				{
					/*const char* colName = this->nucleiTable->GetColumnName(nucleiColIter);
					OutputTable->SetValueByName(somaRowIter, colName, vtkVariant(-PI));*/
					currCell->addNewFeature(vtkVariant(-PI));
				}
			}
		}//end of soma row		
		this->ShowCellAnalysis();
	}// end of matching soma to nuclei
}
void View3D::CalculateCellToCellDistanceGraph()
{
	int cellCount= this->CellModel->getCellCount();
	if (cellCount >= 1)
	{
		this->CellModel->createCellToCellGraph();
	}
}
/*Selections*/
void View3D::updateTraceSelectionHighlights()
{
	this->UpdateLineActor();
	std::vector<TraceLine*> Selections = this->TreeModel->GetSelectedTraces();
	for (unsigned int i = 0; i < Selections.size(); i++)
	{
		this->HighlightSelected(Selections[i],this->SelectColor);
	}
	this->poly_line_data->Modified();
	this->QVTK->GetRenderWindow()->Render();
	this->statusBar()->showMessage(tr("Selected\t")
		+ QString::number(Selections.size()) +tr("\ttraces"));
}

void View3D::HighlightSelected(TraceLine* tline, double color)
{
	TraceLine::TraceBitsType::iterator iter = tline->GetTraceBitIteratorBegin();
	TraceLine::TraceBitsType::iterator iterend = tline->GetTraceBitIteratorEnd();
	if (color == -1)
	{
		color = tline->getTraceColor();
	}
	while(iter!=iterend)
	{
		//poly_line_data->GetPointData()->GetScalars()->SetTuple1(iter->marker,1/t);
		poly_line_data->GetPointData()->GetScalars()->SetTuple1(iter->marker,color);
		++iter;
	}
}

void View3D::Rerender()
{
	this->statusBar()->showMessage(tr("Rerender Image"));
	this->tobj->cleanTree();
	this->SphereActor->VisibilityOff();
	this->SelectedTraceIDs.clear();
	/*this->MergeGaps->GetSelectionModel()->clearSelection();*/
	this->Renderer->RemoveActor(this->BranchActor);
	this->Renderer->RemoveActor(this->PointsActor);

	std::vector<TraceBit> vec = this->tobj->CollectTraceBits();
	if (this->renderTraceBits)
	{
		this->AddPointsAsPoints(vec);
	}
	this->AddDebugPoints(this->tobj->debug_points);
	this->UpdateLineActor();
	this->UpdateBranchActor();
	this->Renderer->AddActor(this->BranchActor); 
	this->TreeModel->SetTraces(this->tobj->GetTraceLines()); 
	this->QVTK->GetRenderWindow()->Render();
	if (this->FTKTable)
	{
		this->FTKTable->setModels(this->TreeModel->getDataTable(), this->TreeModel->GetObjectSelection());
		this->FTKTable->update();
	}
	if (this->TreePlot)
	{
		this->TreePlot->setModels(this->TreeModel->getDataTable(), this->TreeModel->GetObjectSelection());
		this->TreePlot->update();
	}
	this->SplitLabel->setText(QString::number(this->numSplit));
	this->MergeLabel->setText(QString::number(this->numMerged));
	this->DeleteLabel->setText(QString::number(this->numDeleted));
	this->BranchesLabel->setText(QString::number(this->tobj->BranchPoints.size()));
	if(this->FL_MeasurePlot || this->FL_MeasureTable)
	{
		std::vector<CellTrace*> NewCells = this->tobj->CalculateCellFeatures();
		this->CellModel->setCells(NewCells);
	}//end if has cell calculations
	if(this->FL_MeasurePlot)
	{
		this->FL_MeasurePlot->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
		this->FL_MeasurePlot->update();
	}
	if (this->FL_MeasureTable)
	{
		this->FL_MeasureTable->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
		this->FL_MeasureTable->update();
	}
	if (this->FL_histo)
	{
		this->FL_histo->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
		this->FL_histo->update();
	}
	this->statusBar()->showMessage(tr("Finished Rerendering Image"));
}

void View3D::UpdateLineActor()
{
	this->poly_line_data = this->tobj->GetVTKPolyData();
	this->poly_line_data->Modified();
	this->LineMapper->SetInput(this->poly_line_data);
	this->LineActor->SetMapper(this->LineMapper);
	this->LineActor->GetProperty()->SetColor(0,1,0);
	this->LineActor->GetProperty()->SetPointSize(2);
	this->LineActor->GetProperty()->SetLineWidth(lineWidth);
}

void View3D::UpdateBranchActor()
{
	this->poly = tobj->generateBranchIllustrator();
	this->polymap = vtkSmartPointer<vtkPolyDataMapper>::New();
	polymap->SetInput(this->poly);
	this->BranchActor = vtkSmartPointer<vtkActor>::New();
	this->BranchActor->SetMapper(this->polymap);
	this->BranchActor->SetPickable(0);
}

void View3D::AddPointsAsPoints(std::vector<TraceBit> vec)
{
	vtkSmartPointer<vtkCubeSource> cube_src = vtkSmartPointer<vtkCubeSource>::New();
	cube_src->SetBounds(-0.2,0.2,-0.2,0.2,-0.2,0.2);
	vtkSmartPointer<vtkPolyData> point_poly = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPoints> points=vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cells=vtkSmartPointer<vtkCellArray>::New();
	for(unsigned int counter=0; counter<vec.size(); counter++)
	{
		int return_id = points->InsertNextPoint(vec[counter].x,vec[counter].y,vec[counter].z);
		cells->InsertNextCell(1);
		cells->InsertCellPoint(return_id);
	}
	//printf("About to create poly\n");
	point_poly->SetPoints(points);
	point_poly->SetVerts(cells);
	vtkSmartPointer<vtkGlyph3D> glyphs = vtkSmartPointer<vtkGlyph3D>::New();
	glyphs->SetSource(cube_src->GetOutput());
	glyphs->SetInput(point_poly);
	vtkSmartPointer<vtkPolyDataMapper> cubemap = vtkSmartPointer<vtkPolyDataMapper>::New();
	cubemap->SetInput(glyphs->GetOutput());
	cubemap->GlobalImmediateModeRenderingOn();
	PointsActor = vtkSmartPointer<vtkActor>::New();
	PointsActor->SetMapper(cubemap);
	PointsActor->SetPickable(0);
	PointsActor->GetProperty()->SetPointSize(5);
	PointsActor->GetProperty()->SetOpacity(.5);
	Renderer->AddActor(PointsActor);
}

void View3D::AddDebugPoints(std::vector<TraceBit> vec)
{
	if(vec.size() ==0)
		return;
	vtkSmartPointer<vtkCubeSource> cube_src = vtkSmartPointer<vtkCubeSource>::New();
	cube_src->SetBounds(-1,1,-1,1,-1,1);
	vtkSmartPointer<vtkPolyData> point_poly = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPoints> points=vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cells=vtkSmartPointer<vtkCellArray>::New();
	for(unsigned int counter=0; counter<vec.size(); counter++)
	{
		int return_id = points->InsertNextPoint(vec[counter].x,vec[counter].y,vec[counter].z);
		cells->InsertNextCell(1);
		cells->InsertCellPoint(return_id);
	}
	//printf("About to create poly\n");
	point_poly->SetPoints(points);
	point_poly->SetVerts(cells);
	vtkSmartPointer<vtkGlyph3D> glyphs = vtkSmartPointer<vtkGlyph3D>::New();
	glyphs->SetSource(cube_src->GetOutput());
	glyphs->SetInput(point_poly);
	vtkSmartPointer<vtkPolyDataMapper> cubemap = vtkSmartPointer<vtkPolyDataMapper>::New();
	cubemap->SetInput(glyphs->GetOutput());
	cubemap->GlobalImmediateModeRenderingOn();
	PointsActor = vtkSmartPointer<vtkActor>::New();
	PointsActor->SetMapper(cubemap);
	PointsActor->SetPickable(0);
	PointsActor->GetProperty()->SetPointSize(5);
	PointsActor->GetProperty()->SetOpacity(.5);
	PointsActor->GetProperty()->SetColor(1,1,0);
	Renderer->AddActor(PointsActor);
}

void View3D::HandleKeyPress(vtkObject* caller, unsigned long event,
							void* clientdata, void* callerdata)
{
	View3D* view = (View3D*)clientdata;
	char key = view->Interactor->GetKeyCode();
	switch (key)
	{
	case 'a':
		view->AutomationDock->toggleViewAction();
		break;

	case 'l':
		view->ListSelections();
		break;

	case 'c':
		view->ClearSelection();
		break;

		//case 'd':
		////view->tobj->Gaps.clear();
		//  view->DeleteTraces();
		//  break;

		/*case 'm':
		view->MergeTraces();
		break;*/

	case 's':
		view->SplitTraces();
		break;

	case 'f':
		view->FlipTraces();
		break;

	case 'w':
		view->SaveToFile();
		break;

	case 't':
		view->SelectTrees();
		//view->ShowSettingsWindow();
		break;

	case 'q':
		view->updateSelectionHighlights();
		break;
	default:
		break;
	}
}
/*  Automated Selection Actions   */
void View3D::AutomaticEdits()
{
	this->HalfBridges(1);
	this->BreakBranch();
	this->FakeBridges(1);
	this->DeleteTraces();
	this->FakeSpines(1);
	this->DeleteTraces();
	this->SLine(1);
	this->DeleteTraces();

}

void View3D::SLine(double d)
{
	int numLines;
	if(this->flag==1)//if statistics toolbar in use
	{
		this->flag=0;//statistics wont update after clear
		this->TreeModel->GetObjectSelection()->clear();
		this->tobj->FindMinLines((int) this->LineLengthField->value());
		numLines= (int) this->tobj->SmallLines.size();
		this->flag=1;//now statistics will update
		;
	}

	else
	{
		this->TreeModel->GetObjectSelection()->clear();
		this->tobj->FindMinLines((int) this->LineLengthField->value());
		numLines= (int) this->tobj->SmallLines.size();
	}
	this->TreeModel->SelectByIDs(this->tobj->SmallLines);
}

void View3D::FakeSpines(double d)
{
	int numLines;
	if(this->flag==1)
	{
		this->flag=0;
		this->TreeModel->GetObjectSelection()->clear();
		this->tobj->FindFalseSpines((int) this->MaxSpineBit->value(), (int) this->MaxSpinePathLength->value());
		numLines = (int) this->tobj->FalseSpines.size();
		this->flag=1;
	}

	else
	{
		this->TreeModel->GetObjectSelection()->clear();
		this->tobj->FindFalseSpines((int) this->MaxSpineBit->value(), (int) this->MaxSpinePathLength->value());
		numLines = (int) this->tobj->FalseSpines.size();
	}
	this->TreeModel->SelectByIDs(this->tobj->FalseSpines);
}
void View3D::FakeBridges(double d)
{
	int numLines;
	if(this->flag==1)
	{
		this->flag=0;
		this->TreeModel->GetObjectSelection()->clear();	
		this->tobj->FindFalseBridges((int) this->MaxBridgeBits->value());
		numLines= this->tobj->FalseBridges.size();
		this->flag=1;
	}
	else
	{
		this->TreeModel->GetObjectSelection()->clear();	
		this->tobj->FindFalseBridges((int) this->MaxBridgeBits->value());
		numLines= this->tobj->FalseBridges.size();
	}
	this->TreeModel->SelectByIDs(this->tobj->FalseBridges);
}

void View3D::HalfBridges(double d)
{
	int numLines;
	if(this->flag==1)
	{
		this->flag=0;
		this->TreeModel->GetObjectSelection()->clear();	
		this->tobj->FindHalfBridges((int) this->MaxHalfBridgeBits->value(), (int) this->MinDistanceToParent->value());
		numLines= this->tobj->HalfBridges.size();
		this->flag=1;
	}

	else
	{
		this->TreeModel->GetObjectSelection()->clear();	
		this->tobj->FindHalfBridges((int) this->MaxHalfBridgeBits->value(), (int) this->MinDistanceToParent->value());
		numLines= this->tobj->HalfBridges.size();
	}
	this->TreeModel->SelectByIDs(this->tobj->HalfBridges);
}
/*	Statistics	*/

void View3D::ListSelections()
{
	std::vector<TraceLine* > IDs = this->TreeModel->GetSelectedTraces();
	QMessageBox *selectionInfo = new QMessageBox;
	QString listText;
	QString selectedText;
	if(this->tobj->BranchPoints.size()> 0)
	{
		listText += QString::number(this->tobj->BranchPoints.size())
			+ " Branches are unsolved/n";
	}
	else if (IDs.size()<= 0)
	{
		listText = tr("No traces selected");
	}
	else
	{
		listText += QString::number(IDs.size()) + " lines are selected\n";
		selectedText += QString(IDs[0]->statHeaders().c_str()) + "\n";
		for (unsigned int i = 0; i < IDs.size(); i++)
		{
			selectedText += QString(IDs[i]->stats().c_str()) + "\n";   
		} 
	}
	this->statusBar()->showMessage(listText);
	selectionInfo->setWindowTitle("List of all Trace Editor Selections");
	selectionInfo->setIcon(QMessageBox::Information);
	//selectionInfo->setMinimumWidth(512);
	selectionInfo->setText(listText);
	selectionInfo->setDetailedText(selectedText);
	selectionInfo->show();
}
void View3D::ShowTreeData()
{
	this->CloseTreePlots();

	this->FTKTable = new TableWindow();
	this->FTKTable->setModels(this->TreeModel->getDataTable(), this->TreeModel->GetObjectSelection());
	this->FTKTable->setWindowTitle("Trace Object Features Table");
	this->FTKTable->move(this->TraceEditSettings.value("TraceTable/pos",QPoint(32, 561)).toPoint());
	this->FTKTable->resize(this->TraceEditSettings.value("TraceTable/size",QSize(600, 480)).toSize());
	this->FTKTable->show();

	this->TreePlot = new PlotWindow();
	this->TreePlot->setModels(this->TreeModel->getDataTable(), this->TreeModel->GetObjectSelection());
	this->TreePlot->setWindowTitle("Trace Object Features Plot");
	this->TreePlot->move(this->TraceEditSettings.value("TracePlot/pos",QPoint(890, 59)).toPoint());
	this->TreePlot->show();
}

void View3D::showStatistics(void)
{
	if (this->flag == 1)
	{
		this->statisticsToolbar->statisticsDockWidget->close();
	}
	this->statisticsDockWidget = new QDockWidget();
	this->statisticsToolbar = new StatisticsToolbar(statisticsDockWidget);

	statisticsDockWidget->setWidget(statisticsToolbar->statisticsDockWidget);
	statisticsDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
	addDockWidget(Qt::BottomDockWidgetArea, statisticsToolbar->statisticsDockWidget);

	this->statisticsToolbar->setTable(this->TreeModel->getDataTable(), this->TreeModel->GetObjectSelection());
	this->flag = 1;	
}

void View3D::updateStatistics(void)
{
	if (this->flag == 1)
	{
		//std::cout<< "updattteeeee" << std::endl;
		//this->statisticsToolbar->statisticsDockWidget->close();
		//this->flag = 0;
		showStatistics();
	}

}

void View3D::CloseTreePlots()
{
	if (this->FTKTable)
	{
		this->TraceEditSettings.setValue("TraceTable/pos", this->FTKTable->pos());
		this->TraceEditSettings.setValue("TraceTable/size", this->FTKTable->size());
		this->FTKTable->close();
	}
	if (this->TreePlot)
	{
		this->TraceEditSettings.setValue("TracePlot/pos", this->TreePlot->pos());
		this->TreePlot->close();
	}
}
void View3D::ClearSelection()
{
	QMessageBox::StandardButton clearMessageBox;
	if(this->GapsPlotView)
	{
		this->GapsPlotView->close();
	}
	if (this->GapsTableView)
	{
		this->GapsTableView->close();
	}
	this->tobj->Gaps.clear();
	this->candidateGaps.clear();
	if (this->tobj->BranchPoints.size() >0)
	{
		clearMessageBox = QMessageBox::warning(this, tr("Clear selections"), 
			"This action will clear unsolved branching. \nDo you want to Discard or Save unsolved branch points", 
			QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);
		if (clearMessageBox == QMessageBox::Discard)
		{
			this->tobj->BranchPoints.clear();
		}
	}
	this->myText.clear();
	this->dtext.clear();
	this->pointer3d->SetEnabled(0);
	//printf("About to rerender\n");
	this->Rerender();
	//printf("Finished rerendering\n");
	this->statusBar()->showMessage("All Clear", 4000);

	//cout << this->TreePlot->pos().x() << ", " << this->TreePlot->pos().y() << endl;
}

void View3D::SelectTrees()
{
	std::vector<TraceLine*> roots = this->TreeModel->GetSelectedRoots();
	if (roots.size() > 0)
	{
		//this->ClearSelection();
		std::vector<int> ids;
		this->CellModel->SelectByRootTrace(roots);
		ids = this->tobj->GetTreeIDs(roots);
		this->TreeModel->SelectByIDs(ids);
	}//end root size
}
void View3D::updateSelectionFromCell()
{
	/*! 
	* Links CellModel selection to TraceModel Selection
	*/
	this->TreeModel->GetObjectSelection()->clear();
	this->TreeModel->SelectByIDs(this->CellModel->GetSelectedIDs());
}
/*  delete traces functions */
void View3D::DeleteTraces()
{
	/*! removes selected segments from the tree
	* if removing a branch attempt to attach sibling to parent
	* if removing parent children become roots
	*/
	SelectedTraceIDs.clear();
	statusBar()->showMessage(tr("Deleting"));
	std::vector<TraceLine*> traceList = TreeModel->GetSelectedTraces();
	if (traceList.size() >=1)
	{
		EditLogDisplay->append(tr("Deleted\t") + QString::number(traceList.size()) + tr("\ttraces"));
		EditLogDisplay->append(traceList[0]->statHeaders().c_str());
		//this->EditLogDisplay->append( "\tID\tType\tSize\tLength\tEuclidian Length\tRadii\tFragmentation Smoothness\tParent ID");
		for (unsigned int i=0; i<traceList.size()-1; i++)
		{			
			if (traceList[i]->isLeaf()&&!traceList[i]->isRoot())
			{
				for (unsigned int j = i +1; j <traceList.size(); j++)
				{
					if (traceList[j]->isRoot())
					{
						continue;
					}
					if(traceList[i]->GetParentID() == traceList[j]->GetParentID())
					{
						TraceLine * parent = traceList[i]->GetParent();
						traceList[i]->SetParent(NULL);
						traceList[j]->SetParent(NULL);
						if (parent->GetBranchPointer()->size() == 2)
						{
							parent->GetBranchPointer()->clear();
						}
						else
						{
							std::vector<TraceLine*> siblings = *parent->GetBranchPointer();
							std::vector<TraceLine*>::iterator iter = siblings.begin();
							std::vector<TraceLine*>::iterator iterend = siblings.end();
							while(iter != iterend)
							{
								if((*iter== traceList[i])||(*iter== traceList[j]))
								{
									siblings.erase(iter);
									break;
								}
								++iter;
							}//end while
							if (siblings.size() == 1)
							{
								TraceLine *tother1 =siblings[0];
								TraceLine::TraceBitsType::iterator iter1,iter2;
								iter1= parent->GetTraceBitIteratorEnd();
								iter2 = tother1->GetTraceBitIteratorBegin();
								iter1--;
								tobj->mergeTraces((*iter1).marker,(*iter2).marker);
							}//end sibling size
						}//end branch pointer
					}//end parent id
					continue;
				}//end for j < tracelist.size
			}//end ifLeaf
		}//end for i < tracelist.size

		for (unsigned int i = 0; i < traceList.size(); i++)
		{				
			EditLogDisplay->append( QString(traceList[i]->stats().c_str()));
			DeleteTrace(traceList[i]); 
		}
		numDeleted += (int) traceList.size();
		ClearSelection();
		statusBar()->showMessage(tr("Deleted\t") + QString::number(traceList.size()) + tr("\ttraces"));
	}
	else
	{
		statusBar()->showMessage(tr("Nothing to Delete \n"));
	}
}

void View3D::DeleteTrace(TraceLine *tline)
{
	std::vector<TraceLine*> *children = tline->GetBranchPointer();
	if(children->size()!=0)
	{
		for(unsigned int counter=0; counter<children->size(); counter++)
		{
			this->tobj->GetTraceLinesPointer()->push_back((*children)[counter]);  
			(*children)[counter]->SetParent(NULL);
		}
		// remove the children now
		children->clear();
	}         //finds and removes children
	// remove from parent
	std::vector<TraceLine*>* siblings;
	if(tline->GetParent()!=NULL)
	{
		if(this->tobj->BreakOffBranch(tline, false))
		{
			return;		//returns if sibling merged to parent
		}
		siblings = tline->GetParent()->GetBranchPointer();
	}
	else
	{
		siblings = this->tobj->GetTraceLinesPointer();
	}
	std::vector<TraceLine*>::iterator iter = siblings->begin();
	std::vector<TraceLine*>::iterator iterend = siblings->end();
	while(iter != iterend)
	{
		if(*iter== tline)
		{
			siblings->erase(iter);
			break;
		}
		++iter;
	}
	tline->SetParent(NULL);
}
/*	branching functions	*/
void View3D::SetRoots()
{
	this->EditLogDisplay->append("Setting roots");
	this->SelectedTraceIDs.clear();
	if (this->tobj->BranchPoints.size()>1)
	{
		std::vector<int> ids = this->TreeModel->GetSelectedIDs();
		int numToSolve= this->tobj->solveParents(ids);
		this->tobj->cleanTree();
		this->Rerender();
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		if (numToSolve ==0)
		{
			this->tobj->BranchPoints.clear();
		}
		this->statusBar()->showMessage(QString::number(numToSolve)+ " Remaining Branches");
		this->BranchesLabel->setText(QString::number(numToSolve));
	}
	else
	{
		std::vector<TraceLine*> traceList = this->TreeModel->GetSelectedTraces();
		for (unsigned int i = 0; i< traceList.size();i++)
		{
			this->FlipTree(traceList[i] );
		}
		this->tobj->BranchPoints.clear();
		this->ClearSelection();
		this->statusBar()->showMessage(" set roots");
	}
	//this->Rerender();
}
void View3D::AddNewBranches()
{
	unsigned int i=0;
	TraceLine* trunk;
	std::vector <TraceLine*> newChildren;
	//std::vector <TraceLine*> selected; 
	if (this->SelectedTraceIDs.size() > 2)
	{
		trunk =  reinterpret_cast<TraceLine*>(this->tobj->hashc[this->SelectedTraceIDs[0]]);
		for (i=1;i<this->SelectedTraceIDs.size(); i++)
		{
			TraceLine* child = reinterpret_cast<TraceLine*>(
				this->tobj->hashc[this->SelectedTraceIDs[i]]);
			if (child == trunk)
			{
				continue;
			}
			bool found = false;
			for (unsigned int j = 0; j < newChildren.size();j++)
			{
				if (newChildren[j]->GetId() == child->GetId())
				{
					found = true; 
				}
			}
			if (!found)
			{
				newChildren.push_back(child);
			}
		}
		if (newChildren.size() >1)
		{
			if (trunk->isFree())
			{
				if (trunk->Orient(newChildren[0])&&trunk->Orient(newChildren[1]))
				{
					this->tobj->ReverseSegment(trunk);
				}
			}
			else if(!trunk->isLeaf())
			{
				return;
			}
			for ( i = 0; i < newChildren.size(); i ++)
			{
				if (!newChildren[i]->Orient(trunk))
				{
					if (!newChildren[i]->isFree())
					{
						this->FlipTree(newChildren[i]);
					}
					else
					{
						this->tobj->ReverseSegment(newChildren[i]);
					}
				}
			}
			this->AddChildren(trunk, newChildren);
			this->ClearSelection();
			this->statusBar()->showMessage(tr("Update Tree Plots"));
			this->TreeModel->SetTraces(this->tobj->GetTraceLines());
			this->statusBar()->showMessage(tr("Branching complete"));
		}
	}
}
void View3D::ExplodeTree()
{
	std::vector<TraceLine*> roots = this->TreeModel->GetSelectedRoots();
	this->tobj->BranchPoints.clear();
	for (unsigned int i = 0; i < roots.size(); i++)
	{
		this->tobj->explode(roots.at(i));
	}
	//this->tobj->cleanTree();
	this->Rerender();
	this->TreeModel->SetTraces(this->tobj->GetTraceLines());
}
void View3D::BreakBranch()
{
	this->SelectedTraceIDs.clear();
	std::vector<TraceLine*> traces = this->TreeModel->GetSelectedTraces();
	int count = 0;
	QString breakText;
	for (unsigned int i = 0; i < traces.size(); i++)
	{
		int id = traces.at(i)->GetId();
		int parentID = traces.at(i)->GetParentID();
		int dparent = (int) traces.at(i)->GetDistToParent();
		if (this->tobj->BreakOffBranch(traces.at(i),true))
		{
			count++;
			breakText.append(QString("\n%1\t%2\t%3").arg(id).arg(parentID).arg(dparent));
			//this->EditLogDisplay->append(QString("Freed ")+ QString::number(id) + QString(" From Parent"));
		}
	}
	//this->tobj->cleanTree();
	this->EditLogDisplay->append(QString("Removed ")+ QString::number(count) + QString(" Traces From Parents"));
	this->EditLogDisplay->append(QString("Trace\tFrom\tDistance Of\t"));
	this->EditLogDisplay->append(breakText);
	this->Rerender();
	this->TreeModel->SetTraces(this->tobj->GetTraceLines());
}
void View3D::AddChildren(TraceLine *trunk, std::vector<TraceLine*> childTraces)
{
	for (unsigned int i = 0; i < childTraces.size(); i++)
	{
		this->EditLogDisplay->append(QString("Parent to append to: ")+QString::number(trunk->GetId()));
		if (childTraces[i]->GetParentID() == -1)
		{
			childTraces[i]->SetParent(trunk);
			trunk->AddBranch(childTraces[i]);
			this->EditLogDisplay->append(QString("child Branches ")+QString::number(childTraces[i]->GetId()));
		}//end check/make connection 
	}//end child trace size loop
}
/*  merging functions */
void View3D::MergeTraces()
{
	if (this->tobj->Gaps.size() > 1)
	{
		if(this->GapsPlotView)
		{
			this->GapsPlotView->close();
		}
		if (this->GapsTableView)
		{
			this->GapsTableView->close();
		}
		this->MergeSelectedTraces();  	
	}
	else
	{
		int conflict = 0;
		std::vector<TraceLine*> traceList = this->TreeModel->GetSelectedTraces();
		if (traceList.size() >=1)
		{
			conflict = this->tobj->createGapLists(traceList);
		}
		else
		{
			conflict = this->tobj->createGapLists(this->tobj->GetTraceLines());
		}       
		if(conflict == -1)
		{
			//user aborted
			this->tobj->Gaps.clear();
			return;
		}
		unsigned int i; 
		QMessageBox MergeInfo;
		if (this->tobj->Gaps.size() > 1)
		{
			for (i=0;i<this->tobj->Gaps.size(); i++)
			{
				this->tobj->Gaps[i]->compID = i;
			}
			MergeInfo.setText("\nNumber of computed distances:\t" 
				+ QString::number(this->tobj->Gaps.size())
				+"\nConflicts resolved:\t" + QString::number(conflict)
				+"\nEdit selection or press merge again");
			QPushButton *mergeAll = MergeInfo.addButton("Merge All", QMessageBox::YesRole);
			QPushButton *EditSelection = MergeInfo.addButton("Edit Selection", QMessageBox::ActionRole);
			QPushButton *abortButton = MergeInfo.addButton(QMessageBox::Abort);
			MergeInfo.exec();   
			if(MergeInfo.clickedButton()==EditSelection)
			{
				this->ShowMergeStats();
			}
			else if (MergeInfo.clickedButton()==mergeAll)
			{
				unsigned int num = this->tobj->Gaps.size();
				this->numMerged += num;
				this->EditLogDisplay->append("Merged " + 
					QString::number(num) + " traces");
				this->EditLogDisplay->append(QString(this->tobj->Gaps[0]->GapStatHeaders().c_str()));
				for (unsigned int i = 0; i < num; i++)
				{			  
					this->EditLogDisplay->append( QString(this->tobj->Gaps[i]->stats().c_str()));
					tobj->mergeTraces(this->tobj->Gaps[i]->endPT1,this->tobj->Gaps[i]->endPT2);
				}
				this->ClearSelection();
				this->statusBar()->showMessage(tr("Update Tree Plots"));
				this->TreeModel->SetTraces(this->tobj->GetTraceLines());
				this->statusBar()->showMessage(tr(" Done With Merge"));
			}    
			else if (MergeInfo.clickedButton()==abortButton)
			{
				this->tobj->Gaps.clear();
				this->candidateGaps.clear();
				this->statusBar()->showMessage("Merge Canceled");
			}
		}//end if this->tobj->Gaps size > 1
		else 
		{
			if (this->tobj->Gaps.size() ==1)
			{   
				tobj->mergeTraces(this->tobj->Gaps[0]->endPT1,this->tobj->Gaps[0]->endPT2);
				this->EditLogDisplay->append("Merged Trace:\t"  + QString(this->tobj->Gaps[0]->GapStatHeaders().c_str()) 
					+ QString(this->tobj->Gaps[0]->stats().c_str()));
				/*	+ QString::number(this->tobj->Gaps[0]->Trace1->GetId()) + "\tto\t" 
				+ QString::number(this->tobj->Gaps[0]->Trace2->GetId()));*/
				this->numMerged++;
				this->ClearSelection();
				this->statusBar()->showMessage(tr("Update Tree Plots"));
				this->TreeModel->SetTraces(this->tobj->GetTraceLines());
				this->statusBar()->showMessage(tr("One Trace merged; Done With Merge"));
			}
			else
			{
				this->Rerender();
				MergeInfo.setText("\nNo merges possible, set higher tolerances\n"); 
			} 
		}   
		this->myText.clear();
		this->poly_line_data->Modified();
		this->QVTK->GetRenderWindow()->Render();
	}//end else size
}

void View3D::ShowMergeStats()
{
	this->MergeGaps->SetTraceGaps(this->tobj->Gaps);
	this->GapsTableView = new TableWindow(); 
	this->GapsTableView->setModels(this->MergeGaps->getDataTable(), this->MergeGaps->GetObjectSelection());
	this->GapsTableView->setWindowTitle("Computed Features for Merge");
	this->GapsTableView->show();
	this->GapsPlotView = new PlotWindow();
	this->GapsPlotView->setModels(this->MergeGaps->getDataTable(), this->MergeGaps->GetObjectSelection());
	this->GapsPlotView->setWindowTitle("Computed Features for Merge");
	this->GapsPlotView->show();
}
void View3D::ShowCellAnalysis()
{
	//this->HideCellAnalysis();
	std::vector<CellTrace*> NewCells = this->tobj->CalculateCellFeatures();
	if (NewCells.size() > 0)
	{
		this->CellModel->setCells(NewCells);
		if(this->FL_MeasurePlot)
		{
			this->FL_MeasurePlot->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
			this->FL_MeasurePlot->update();
		}
		else
		{
			this->FL_MeasurePlot = new PlotWindow();
			this->FL_MeasurePlot->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
			this->FL_MeasurePlot->setWindowTitle("Computed Features for Cells");
			this->FL_MeasurePlot->move(this->TraceEditSettings.value("FLMeasurePlot/pos",QPoint(32, 561)).toPoint());
			this->FL_MeasurePlot->show();
		}
		if (this->FL_MeasureTable)
		{
			this->FL_MeasureTable->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
			this->FL_MeasureTable->update();
		}
		else
		{
			this->FL_MeasureTable = new TableWindow();
			this->FL_MeasureTable->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
			this->FL_MeasureTable->setWindowTitle("Computed Features for Cells");
			this->FL_MeasureTable->move(this->TraceEditSettings.value("FLMeasureTable/pos",QPoint(32, 561)).toPoint());
			this->FL_MeasureTable->resize(this->TraceEditSettings.value("FLMeasureTable/size",QSize(600, 480)).toSize());
			this->FL_MeasureTable->show();
		}
		/*if (this->FL_histo)
		{
			this->FL_histo->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
			this->FL_histo->update();
		}
		else
		{
			this->FL_histo = new HistoWindow();
			this->FL_histo->setModels(this->CellModel->getDataTable(), this->CellModel->GetObjectSelection());
			this->FL_histo->show();
		}*/
	}//end if new cells size > 0
}
void View3D::HideCellAnalysis()
{
	if (this->FL_MeasurePlot)
	{
		this->TraceEditSettings.setValue("FLMeasurePlot/pos", this->FL_MeasurePlot->pos());
		this->FL_MeasurePlot->close();
	}
	if(this->FL_MeasureTable)
	{
		this->TraceEditSettings.setValue("FLMeasureTable/pos", this->FL_MeasureTable->pos());
		this->TraceEditSettings.setValue("FLMeasureTable/size", this->FL_MeasureTable->size());
		this->FL_MeasureTable->close();
	}
	if (this->FL_histo)
	{
		this->FL_histo->close();
	}
	this->TraceEditSettings.sync();
}


void View3D::StartActiveLearning()
{
	vtkSmartPointer<vtkTable> featureTable;
	double confidence_thresh = 0.5;
	int cellCount= this->CellModel->getCellCount();
	if (cellCount < 1)
	{
		return;
	}
	//vtkSmartPointer<vtkTable> myDataTable;
	//myDataTable = this->CellModel->getDataTable();
	featureTable = this->CellModel->getDataTable();
	featureTable->RemoveColumnByName("Trace File");
	if(!featureTable) return;
//run training dialoge for sample selection
	TrainingDialog *Training = new TrainingDialog(featureTable, "train","active",featureTable->GetNumberOfRows() ,this);
	Training->exec();

	std::vector< std::pair<int,int> > id_time;	
	// Remove the training examples from the list of ids.
	//Get the list of ids
	for(int i=0;i<featureTable->GetNumberOfRows(); ++i)
	{
		if(featureTable->GetValueByName(i,"train_default1").ToDouble()==-1) 
		{
			std::pair<int,int> temp_pair;
			temp_pair.first = featureTable->GetValue(i,0).ToInt();
			temp_pair.second = 0;
			id_time.push_back(temp_pair);
		}
	}

	// If the user did not hit cancel 
	if(Training->result())
	{
		PatternAnalysisWizard *pWizard = new PatternAnalysisWizard( featureTable, PatternAnalysisWizard::_ACTIVE,"","", this);
		pWizard->setWindowTitle(tr("Pattern Analysis Wizard"));
		pWizard->exec();

		//new_table does not have the id column 
		vtkSmartPointer<vtkTable> new_table = pWizard->getExtractedTable();
		// If the user did not hit cancel 	
		if(pWizard->result())
		{
			//// Delete the prediction column if it exists
			std::vector< std::string > prediction_names = ftk::GetColumsWithString( "prediction_active" , new_table);
			if(prediction_names.size()>0)
				new_table->RemoveColumnByName("prediction_active");

			vnl_vector<double> class_list(new_table->GetNumberOfRows()); 

			for(int row = 0; (int)row < new_table->GetNumberOfRows(); ++row)  
			{
				class_list.put(row,vtkVariant(featureTable->GetValueByName(row,"train_default1")).ToDouble());
			}

			mclr = new MCLR_SM();
			double sparsity = 1;
			int active_query = 1;
			double max_info = -1e9;

			vnl_matrix<double> Feats = this->mclr->Normalize_Feature_Matrix(mclr->tableToMatrix(new_table, id_time));
			mclr->Initialize(Feats,sparsity,class_list,"",new_table);
			mclr->Get_Training_Model();

			// Get the active query based on information gain
			active_query = this->mclr->Active_Query();

			bool user_stop_dialog_flag = false;
			bool loop_termination_condition = true;

			

			/////////////////////////////////////////////////////////////////////////
			// Querying starts now
			/////////////////////////////////////////////////////////////////////////
			while(loop_termination_condition)
			{	//select the appropriate classes 				
				if (this->viewIn2D)
				{
					double test [6];
					this->Renderer->ComputeVisiblePropBounds(test);
					this->setRenderFocus(test, 6);
				}// end of reset renderer when in 2d mode 
				int zoomID = this->mclr->id_time_val.at(active_query).first;
				for(int row=0; row<(int)this->CellModel->getDataTable()->GetNumberOfRows(); ++row)
				{
					if(this->CellModel->getDataTable()->GetValue(row,0) == zoomID)
					{
						zoomID = row;
						break;
					}
				}
				CellTrace* currCell = this->CellModel->GetCellAt(zoomID);
				this->FocusOnCell(currCell);
									
				ALDialog =  new GenericALDialog(mclr->test_table, this->mclr->no_of_classes, active_query, this->mclr->top_features);
				ALDialog->setWindowTitle(QString("Active Learning Window: Specify Class for Cell %1").arg(mclr->id_time_val.at(active_query).first));
				ALDialog->exec();	 

				//if(dialog->rejectFlag)
				//	return;

				loop_termination_condition = ALDialog->finish &&ALDialog->result();

				while(ALDialog->class_selected == -1)
				{	
					QMessageBox::critical(this, tr("Oops"), tr("Please select a class"));
					this->show();
					ALDialog =  new GenericALDialog(mclr->test_table, this->mclr->no_of_classes, active_query, this->mclr->top_features);	
					ALDialog->exec();
					//i=0;
					//if(dialog->rejectFlag)
					//	return;
				}

				// Update the data & refresh the training model and refresh the Training ALDialog 		
				mclr->Update_Train_Data(active_query, ALDialog->class_selected);
				
				if(ALDialog->class_selected == 0)
				{
					mclr->Get_Training_Model();
					active_query = this->mclr->Active_Query();
					continue;
				}
				if(mclr->stop_training !=0)
				{
					QMessageBox msgBox;
					msgBox.setText("I understand the classification problem.");
					msgBox.setInformativeText("Do you want to stop training and classify ? ");
					msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
					msgBox.setDefaultButton(QMessageBox::Ok);
					int ret = msgBox.exec();

					switch (ret) 
					{
					case QMessageBox::Ok:
						// Save was clicked
						user_stop_dialog_flag = true;
						break;
					case QMessageBox::Cancel:
						mclr->stop_training = false;
						break;
					default:
						// should never be reached
						break;
					}
				}

				if(user_stop_dialog_flag)
					break;

				mclr->Get_Training_Model();
				active_query = this->mclr->Active_Query();
			}// while !loop_termination_condition
			// Querying is done


			/////////////////////////////////////////
			vtkSmartPointer<vtkTable> test_table  = vtkSmartPointer<vtkTable>::New();
			test_table->Initialize();

			test_table->SetNumberOfRows(this->CellModel->getDataTable()->GetNumberOfRows());
			for(int col=0; col<new_table->GetNumberOfColumns(); ++col)
			{
				vtkSmartPointer<vtkDoubleArray> column = vtkSmartPointer<vtkDoubleArray>::New();
				column->SetName(new_table->GetColumnName(col));
				test_table->AddColumn(column);	
			}
			for(int row = 0; row < (int)this->CellModel->getDataTable()->GetNumberOfRows(); ++row)
			{		
				vtkSmartPointer<vtkVariantArray> model_data1 = vtkSmartPointer<vtkVariantArray>::New();
				for(int c =0;c<(int)test_table->GetNumberOfColumns();++c)
					model_data1->InsertNextValue(this->CellModel->getDataTable()->GetValueByName(row,test_table->GetColumnName(c)));
				test_table->InsertNextRow(model_data1);
			}	

			////// Final Data  to classify after the active training
			vnl_matrix<double> data_classify;
			data_classify =  this->mclr->Normalize_Feature_Matrix(mclr->tableToMatrix(test_table, this->mclr->id_time_val));
			data_classify = data_classify.transpose();

			vnl_matrix<double> currprob;
			currprob = this->mclr->Test_Current_Model(data_classify);
			
			int predictionIndex = this->CellModel->AddNewFeatureHeader("Prediction");
			int confIndex = this->CellModel->AddNewFeatureHeader("Confidence");
			//std::cout << "debug prediction: "<< predictionIndex << "confidence" << confIndex << std::endl;
			for(unsigned int row = 0; (int)row < this->CellModel->getDataTable()->GetNumberOfRows(); ++row)  
			{
				vnl_vector<double> curr_col = currprob.get_column(row);
				CellTrace* currCell = this->CellModel->GetCellAtNoSelection(row);
				//myDataTable->SetValueByName(row, confidence_col_name.c_str(), vtkVariant(curr_col(curr_col.arg_max())));
				if(curr_col(curr_col.arg_max()) > confidence_thresh) 
				{
					currCell->SetClassifcation(predictionIndex, curr_col.arg_max()+1, confIndex, curr_col(curr_col.arg_max()));
				}
				else
				{
					currCell->SetClassifcation(predictionIndex, 0, confIndex, curr_col(curr_col.arg_max()));
				}
			}
			this->ShowCellAnalysis();
		}//pwizzard 
	}// Training->result after training dialog
}//end of active learning

QImage View3D::Get_AL_Snapshot(CellTrace* currentCell)
{
	double cellBounds[6];
	currentCell->getCellBounds(cellBounds);
	//this->FocusOnCell(currCell);
	double test [6];
	this->Renderer->ComputeVisiblePropBounds(test);
	this->setRenderFocus(test, 6);
	this->WindowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	this->WindowToImage->SetInput(this->QVTK->GetRenderWindow());
	this->WindowToImage->Update();

	QImage qimage = vtkImageDataToQImage(this->WindowToImage->GetOutput());
	qimage.save("traceEdit_screenshot.tif");

	return qimage;

}

QImage View3D::vtkImageDataToQImage(vtkImageData * imageData)
{
    int dim[3];
    imageData->GetDimensions(dim);
    if(dim[0]*dim[1]*dim[2] == 0)
        return QImage();

	int x_min=dim[0], x_max=0, y_min=dim[1], y_max=0;
	
    vtkUnsignedCharArray* scalars 
        = vtkUnsignedCharArray::SafeDownCast(imageData->GetPointData()->GetScalars());
    if(!scalars)
        return QImage();

    QImage qImage(dim[0], dim[1], QImage::Format_ARGB32);
    vtkIdType tupleIndex=0;
   
    for(int j=0; j<dim[1]; j++)
    {
        for(int i=0; i<dim[0]; i++)
        {
			unsigned char tuple[] = {0, 0, 0, 0};
            int r=0, g=0, b=0, a=0;
            scalars->GetTupleValue(tupleIndex+(j*dim[0])+i, tuple);

            switch(scalars->GetNumberOfComponents())
            {
            case 1: 
                r = g = b = tuple[0];
                a = 255;
                break;
            case 2:
                r = g = b = tuple[0];
                a = tuple[1];
                break;
            case 3:
                r = tuple[0];
                g = tuple[1];
                b = tuple[2];
                a = 255;
                break;
            case 4:
                r = tuple[0];
                g = tuple[1];
                b = tuple[2];
                a = tuple[3];
                break;
            }

			//to get the bounds of the orange traces
			if(r>100 && g>80 && g<100 && b<10)
			{
				r = 0;
                g = 255;
                b = 255;
				if(i < x_min)
					x_min = i;
				if(i > x_max)
					x_max = i;
				if(j < y_min)
					y_min = j;
				if(j > y_max)
					y_max = j;

			}
			////////////////////////////////////////
            QRgb color = qRgba(r, g, b, a);
            qImage.setPixel(i, j, color);
        }
    }
	if((x_max - x_min) > (y_max - y_min))
	{
		y_min = y_min - (((x_max - x_min)-(y_max - y_min))/2);
		y_max = y_max + (((x_max - x_min)-(y_max - y_min))/2);
	}
	if((y_max - y_min) > (x_max - x_min))
	{
		x_min = x_min - (((y_max - y_min)-(x_max - x_min))/2);
		x_max = x_max + (((y_max - y_min)-(x_max - x_min))/2);
	}
	QImage q_Image;
	//if((x_max - x_min) > 256)
		q_Image = qImage.copy(x_min - 20, y_min - 20, x_max - x_min + 40, y_max - y_min + 40).scaledToHeight(256);	
	//else
	//{
	//	x_min = x_min - ((256 - (x_max - x_min))/2);
	//	x_max = x_max + ((256 - (x_max - x_min))/2);
	//	y_min = y_min - ((256 - (y_max - y_min))/2);
	//	y_max = y_max + ((256 - (y_max - y_min))/2);
	//	q_Image = qImage.copy(x_min - 10, y_min - 10, x_max - x_min + 20, y_max - y_min + 20);
	//}
 
	return q_Image;
}

void View3D::updateSelectionHighlights()
{
	bool selected = false;
	int curID;
	std::vector<int> GapIDs = this->MergeGaps->GetSelectedGapIDs();
	for (unsigned int i = 0; i < this->tobj->Gaps.size(); i++)
	{
		curID = this->tobj->Gaps[i]->compID;
		selected = false;
		unsigned int j =0;
		while(!selected && j < GapIDs.size())
		{
			if ( curID == GapIDs[j])
			{
				selected = true;
			}
			else
			{
				j++;
			}
		}//end while
		if (selected == true)
		{
			this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .05);
			this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .05);
		}//end true
		else
		{
			this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .3);
			this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .3);
		}//end else true
	}//end for i
	this->poly_line_data->Modified();
	this->QVTK->GetRenderWindow()->Render();
	this->statusBar()->showMessage(tr("Done"));
}
void View3D::MergeSelectedTraces()
{
	this->statusBar()->showMessage(tr("Merging"));
	this->updateSelectionHighlights(); //if it didnt
	std::vector<int> GapIDs = this->MergeGaps->GetSelectedGapIDs();
	bool selected = false;
	int curID;
	QMessageBox MergeInfo;
	double aveCost=0;
	QPushButton *mergeAll;
	QPushButton *mergeNone;
	unsigned int i=0, j=0;
	MergeInfo.setText("Merge Function");
	if (GapIDs.size() > 1)
	{
		for (i = 0; i < this->tobj->Gaps.size(); i++)
		{
			curID = this->tobj->Gaps[i]->compID;
			selected = false; 
			j =0;
			while(!selected && j < GapIDs.size())
			{
				if ( curID == GapIDs[j])
				{
					selected = true;
				}
				else
				{
					j++;
				}
			}//end gapids size j
			if (selected == true)
			{
				aveCost += this->tobj->Gaps[i]->cost;
				this->tobj->mergeTraces(this->tobj->Gaps[i]->endPT1,this->tobj->Gaps[i]->endPT2);
				this->dtext += this->tobj->Gaps[i]->stats().c_str();
			}
		} 
		MergeInfo.setText("merged " + QString::number(GapIDs.size()) + " traces.");  
		this->EditLogDisplay->append("merged " + QString::number(GapIDs.size()) + " traces.");  
		this->numMerged += (int)GapIDs.size();
		this->dtext+="\nAverage Cost of Merge:\t" 
			+ QString::number( aveCost / GapIDs.size() );
		this->dtext+= "\nSum of Merge costs " + QString::number(aveCost);
		this->EditLogDisplay->append(this->dtext);
		MergeInfo.setDetailedText(this->dtext);
		MergeInfo.exec();
	}
	else
	{
		for (i=0;i<this->tobj->Gaps.size(); i++)
		{
			if  (this->tobj->Gaps[i]->cost <=5)
			{
				this->dtext+= this->tobj->Gaps[i]->stats().c_str();/*"\nTrace " + QString::number(this->tobj->Gaps[i]->Trace1->GetId());
																   this->dtext+= " and "+ QString::number(this->tobj->Gaps[i]->Trace2->GetId() );
																   this->dtext+="\tcost of:" + QString::number(this->tobj->Gaps[i]->cost); */
				this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .125);
				this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .125);
				this->candidateGaps.push_back( this->tobj->Gaps[i]);
			} //end of cost<5
		}//end of for merge    
		if (this->candidateGaps.size()>=1)
		{      
			myText+="\nNumber of possible lines:\t" + QString::number(this->candidateGaps.size());
			mergeAll = MergeInfo.addButton("Merge All", QMessageBox::YesRole);
			mergeNone = MergeInfo.addButton("Merge None", QMessageBox::NoRole);
			MergeInfo.setText(myText); 
			MergeInfo.setDetailedText(this->dtext);
			this->poly_line_data->Modified();
			this->QVTK->GetRenderWindow()->Render();
			MergeInfo.exec();    
			if(MergeInfo.clickedButton()==mergeAll)
			{
				this->EditLogDisplay->append("Merged " + 
					QString::number(this->candidateGaps.size()) + " traces");
				this->EditLogDisplay->append(this->dtext);
				this->numMerged += (int)this->candidateGaps.size();
				for (j=0; j<this->candidateGaps.size();j++)
				{
					tobj->mergeTraces(this->candidateGaps[j]->endPT1,this->candidateGaps[j]->endPT2);
				}
			}
			else if(MergeInfo.clickedButton()==mergeNone)
			{
				this->candidateGaps.clear();
				this->statusBar()->showMessage("Merge Canceled");
			}
		}   //end if graylist size
		else
		{
			this->statusBar()->showMessage("nothing to merge");
		}   //end else   
	}
	this->ClearSelection();
	this->statusBar()->showMessage(tr("Update Tree Plots"));
	this->TreeModel->SetTraces(this->tobj->GetTraceLines());
	this->statusBar()->showMessage(tr("Done With Merge"));
}
/*  other trace modifiers */
void View3D::SplitTraces()
{
	if(this->SelectedTraceIDs.size()>=1)
	{
		std::unique(this->SelectedTraceIDs.begin(), this->SelectedTraceIDs.end());
		for(unsigned int i = 0; i < this->SelectedTraceIDs.size(); i++)
		{
			this->tobj->splitTrace(this->SelectedTraceIDs[i]);
		}

		//scanf("%*d\n");
		//printf("About to finish up\n");
		this->numSplit += (int) this->SelectedTraceIDs.size();
		//printf("about to add things to edit log\n");
		this->EditLogDisplay->append("split " + QString::number(this->SelectedTraceIDs.size()) +" Traces");
		//printf("About to clear selection\n");
		this->ClearSelection();
		//printf("about to update status bar\n");
		this->statusBar()->showMessage(tr("Update Tree Plots"));
		//printf("about to set tree model\n");
		//this->TreeModel->SetTraces(this->tobj->GetTraceLines());
	}
	else
	{
		this->statusBar()->showMessage(tr("Nothing to split"), 1000); 
	}
}

void View3D::FlipTraces()
{
	int numFlipped =0;
	std::vector<TraceLine*> traceList = this->TreeModel->GetSelectedTraces();
	if (traceList.size() >=1)
	{
		for (unsigned int i = 0; i < traceList.size(); i++)
		{
			TraceLine* thisLine = traceList[i];
			if (thisLine->isFree())
			{
				//this->EditLogDisplay->append("\tTrace\t" + QString::number(thisLine->GetId()));
				this->tobj->ReverseSegment(thisLine);
				this->dtext += QString("\n") + QString(thisLine->stats().c_str() );
				numFlipped++;
			}
			else if (thisLine->isLeaf())
			{
				this->FlipTree( thisLine);
				numFlipped++;
			}
		}		
		this->ClearSelection();
		this->EditLogDisplay->append("Fipped " +  QString::number(numFlipped) + " Traces");
		this->statusBar()->showMessage(tr("Reversed Selected"));
	}
	else
	{
		this->statusBar()->showMessage(tr("Nothing Selected"));
	}
}
void View3D::FlipTree(TraceLine *thisLine)
{
	if (thisLine->isLeaf())
	{
		this->tobj->BranchPoints.clear();
		this->tobj->explode(this->tobj->findTraceByID( thisLine->GetRootID()));
		this->tobj->isParent(thisLine->GetId());
		this->tobj->cleanTree();
		this->Rerender();
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		this->EditLogDisplay->append(QString("Reversed Tree New Root is Trace: ")+QString::number(thisLine->GetId()));
	}
}
void View3D::SetTraceType(int newType)
{
	std::vector<TraceLine*> traceList = this->TreeModel->GetSelectedTraces();
	if (traceList.size() >=1)
	{
		this->EditLogDisplay->append( QString::number(traceList.size())
			+ " Traces set to type: " +  QString::number( newType));
		for (unsigned int i = 0; i < traceList.size(); i++)
		{
			this->EditLogDisplay->append( "\tTrace\t" + QString::number(traceList[i]->GetId()));
			traceList[i]->SetType((unsigned char) newType);
			traceList[i]->setTraceColor(this->tobj->GetTraceLUT(traceList[i]));
		}
		this->ClearSelection();
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		this->statusBar()->showMessage(QString::number(traceList.size())
			+ tr(" traces set to type")+ QString::number(newType));
	}
	else
	{
		this->statusBar()->showMessage(tr("Nothing Selected"));
	}
}
/* output */
void View3D::SaveToFile()
{
	//display a save file dialog

	QString traceDir = this->TraceEditSettings.value("traceDir", ".").toString();
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("Save File"),
		traceDir,
		tr("SWC Images (*.swc);;VTK files (*.vtk)"));

	//if the user pressed cancel, bail out now.
	if(fileName.isNull())
	{
		return;
	}
	//this->tempTraceFile.clear();// will make this a buffered state for reload
	this->tempTraceFile.removeAll(fileName);
	this->tempTraceFile.append( fileName);
	this->TraceEditSettings.setValue("lastOpen/Temp", this->tempTraceFile);
	//make sure the user supplied an appropriate output file format
	if(!fileName.endsWith(".vtk") && !fileName.endsWith(".swc"))
	{
		QMessageBox::critical(this, tr("Unsupported file format"),
			tr("Trace editor only supports output to .swc and .vtk file formats"));
		this->SaveToFile();
		return;
	}

	if(fileName.endsWith(".swc"))
	{
		this->tobj->WriteToSWCFile(fileName.toStdString().c_str()); 
	}
	else if(fileName.endsWith(".vtk"))
	{
		this->tobj->WriteToVTKFile(fileName.toStdString().c_str()); 
	}
	this->statusBar()->showMessage("File saved as:\t" + fileName.section('/',-2));
	this->EditLogDisplay->append(QString("File saved as: %1  at time: %2").arg(fileName) 
		.arg(this->Time.currentTime().toString( "h:m:s ap" )));
	//Edit Log written to file
	QString logFileName = this->tempTraceFile.last().section('.',0,-1);
	logFileName.append("_log.txt");
	QFile logFile(logFileName);
	if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		return;
	}
	else
	{
		this->TraceEditSettings.setValue("lastOpen/Log", logFileName);
		QTextStream out(&logFile);
		out << this->EditLogDisplay->toPlainText();
	}
	if (!this->ProjectName.isEmpty())
	{
		this->SaveProjectFile();
	}
	this->TraceEditSettings.setValue("lastOpen/Project",this->ProjectName);
	this->TraceEditSettings.sync();
}
void View3D::SaveProjectFile()
{
	QString projectDir = this->TraceEditSettings.value("projectDir", ".").toString();
	QString newProject = QFileDialog::getSaveFileName(
		this,
		tr("Save Project File"),
		projectDir,
		tr("Project File (*.xml)"));
	if (!newProject.isEmpty())
	{
		ftk::ProjectManager * project = new ftk::ProjectManager();
		for (unsigned int i = 0; i < this->ImageActors->NumberOfImages(); i++)
		{
			std::string name = this->ImageActors->FileNameOf(i);
			std::string type;
			if (this->ImageActors->isRayCast(i))
			{
				type = "Image";
			}else
			{
				type = "Soma";
			}
			std::vector<double> coord = this->ImageActors->GetShiftImage(i);
			project->addFile(name, type, coord[0], coord[1], coord[2]);
		}//end adding images
		if (!this->tempTraceFile.isEmpty())
		{
			project->addFile(this->tempTraceFile.last().toStdString(), "Trace", 0,0,0);
		}
		this->ProjectName = newProject;
		project->writeProject((char*)newProject.toStdString().c_str());
		this->TraceEditSettings.setValue("lastOpen/Project",this->ProjectName);
		this->TraceEditSettings.sync();
	}
}
void View3D::SaveSelected()
{
	QString traceDir = this->TraceEditSettings.value("traceDir", ".").toString();
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("Save Selected Trees to File"),
		traceDir,
		tr("SWC Images (*.swc)"));
	if(!fileName.isEmpty())
	{
		//this->tempTraceFile.append( fileName);
		std::vector<TraceLine*> roots = this->TreeModel->GetSelectedRoots();
		if (roots.size() > 0)
		{
			this->tobj->WriteToSWCFile(roots, fileName.toStdString().c_str()); 
			this->EditLogDisplay->append(QString("Selected traces file saved as: %1  at time: %2")
				.arg(fileName) 
				.arg(this->Time.currentTime().toString( "h:m:s ap" )));
		}
	}
}
void View3D::saveRenderWindow(const char *filename)
{
	this->WindowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	this->WindowToImage->SetInput(this->QVTK->GetRenderWindow());
	
	if (savescreenshotDialog) //only if savescreenshotDialog is constructed
		this->WindowToImage->SetMagnification(this->savescreenshotDialog->getMagnification()); //save this for presentations
	else
		this->WindowToImage->SetMagnification(1);
	
	this->JPEGWriter = vtkSmartPointer<vtkJPEGWriter>::New();
	this->JPEGWriter->SetInput(this->WindowToImage->GetOutput());
	this->JPEGWriter->SetFileName(filename);
	this->JPEGWriter->Write();
}
void View3D::SaveScreenShot()
{
	QString fileName, filePath;
	QString imageDir = this->TraceEditSettings.value("ScreenShotDir", ".").toString();
	this->savescreenshotDialog = new ScreenShotDialog(this, fileName, imageDir);
	savescreenshotDialog->exec();
	filePath = savescreenshotDialog->getDir();
	fileName = savescreenshotDialog->getfileName();
	QString fullFileName = filePath % "/" % fileName % QString(".jpg");
	if (!fullFileName.isEmpty())
	{
		this->saveRenderWindow(fullFileName.toStdString().c_str());
		this->TraceEditSettings.setValue("ScreenShotDir", filePath);
	}
	delete savescreenshotDialog;
	savescreenshotDialog = NULL;
}
void View3D::AutoCellExport()
{
	bool reNameCell= false;
	int cellCount= this->CellModel->getCellCount();
	if (cellCount >= 1)
	{
		QString curdirectoryswc = this->TraceEditSettings.value("swcDir", ".").toString();
		QString curdirectoryjpg = this->TraceEditSettings.value("jpgDir", ".").toString();
		QString swcfileName, jpgfileName;
		bool changeswcfileName = false;
		bool changejpgfileName = false;

		SaveCellExportDialog *cellexportDialog = new SaveCellExportDialog(this, curdirectoryswc, curdirectoryjpg, swcfileName, jpgfileName, changeswcfileName, changejpgfileName);
		cellexportDialog->exec();

		curdirectoryswc = cellexportDialog->getSWCDir();
		curdirectoryjpg = cellexportDialog->getJPGDir();
		swcfileName = cellexportDialog->getSWCfileName();
		jpgfileName = cellexportDialog->getJPGfileName();
		changeswcfileName = cellexportDialog->differentSWCfileName();
		changejpgfileName = cellexportDialog->differentJPGfileName();
		
		if (cellexportDialog->getSave())
		{
			QProgressDialog progress("Finding Cells", "Abort", 0, cellCount, this);
			progress.setWindowModality(Qt::WindowModal);
			QString traceDir = this->TraceEditSettings.value("traceDir", ".").toString();		
			QString coordFileName = traceDir % "/coord.txt";
			this->CellModel->WriteCellCoordsToFile(coordFileName.toStdString().c_str());

			for (int i = 0; i < cellCount; i++)
			{
				progress.setValue(i);
				if (progress.wasCanceled())
				{
					break;
				}
				CellTrace* currCell = this->CellModel->GetCellAt( i);
				this->FocusOnCell(currCell);
				QString cellName = QString(currCell->GetFileName().c_str());

				std::vector<TraceLine*> roots;
				roots.push_back(currCell->getRootTrace());
				int cellNum = i+1;
				// if change swc filename and assign a number
				if (changeswcfileName)
				{
					if (!swcfileName.isEmpty())
					{
						cellName = swcfileName + QString("%1").arg(cellNum);;
					}
					else
					{
						cellName = tr("cell_") + QString("%1").arg(cellNum);
					}
				}
				QString swcFileName = curdirectoryswc % "/" % cellName % QString(".swc");
				this->TraceEditSettings.setValue("swcDir", curdirectoryswc);
				this->tobj->WriteToSWCFile(roots, swcFileName.toStdString().c_str());
				// if change jpg filename and assign a number
				if (changejpgfileName)
				{
					if (!jpgfileName.isEmpty())
					{
						cellName = jpgfileName + QString("%1").arg(cellNum);
					}
					else
					{
						cellName = tr("cell_") + QString("%1").arg(cellNum);
					}
				}
				QString ScreenShotFileName = curdirectoryjpg % "/" % cellName % QString(".jpg");
				this->TraceEditSettings.setValue("jpgDir", curdirectoryjpg);
				this->saveRenderWindow(ScreenShotFileName.toStdString().c_str());
				if (this->viewIn2D)
				{
					double test [6];
					this->Renderer->ComputeVisiblePropBounds(test);
					this->setRenderFocus(test, 6);
				}// end of reset renderer when in 2d mode 
			}//end of cell export loop
			progress.setValue(cellCount);
		}
	}
	else
	{
		QMessageBox AutoExportInfo;
		AutoExportInfo.setText("No Cells Identified\nRun Cell Analysis");
		AutoExportInfo.exec();
	}
}
void View3D::closeEvent(QCloseEvent *event)
{	
	this->TraceEditSettings.setValue("mainWin/size", this->size());
	this->TraceEditSettings.setValue("mainWin/pos",	this->pos());
	this->TraceEditSettings.setValue("mainWin/use2d", this->viewIn2D);
	this->TraceEditSettings.setValue("lastOpen/Project",this->ProjectName);
	this->TraceEditSettings.setValue("lastOpen/Image", this->Image);
	this->TraceEditSettings.setValue("lastOpen/Trace",this->TraceFiles);
	this->TraceEditSettings.setValue("lastOpen/Soma", this->SomaFile);
	this->TraceEditSettings.setValue("lastOpen/Temp", this->tempTraceFile);
	this->CloseTreePlots();
	this->HideCellAnalysis();
	this->TraceEditSettings.sync();
	if(this->GapsPlotView)
	{
		this->GapsPlotView->close();
	}
	if(this->GapsTableView)
	{
		this->GapsTableView->close();
	}
	event->accept();
}

void View3D::CropBorderCells()
{
	std::vector<TraceLine*> roots = tobj->GetAllRoots();

	if (roots.size() == 0)
		return;	//No roots, so do nothing

	if (CellModel->getCellCount() == 0)	//Need to calculate cell features before we can select them!
	{
		std::vector<CellTrace*> NewCells = this->tobj->CalculateCellFeatures();
		if (NewCells.size() > 0)
		{
			this->CellModel->setCells(NewCells);
		}
	}

	CellModel->SelectByRootTrace(roots);
	std::vector<CellTrace*> cells_list = CellModel->GetSelectedCells();
	std::vector<CellTrace*>::iterator cells_list_iter;

	float sum_of_skewness_X = 0;
	float sum_of_skewness_Y = 0; 
	float sum_of_skewness_Z = 0;
	float sum_of_height = 0;
	float sum_of_width = 0; 
	float sum_of_depth = 0;
	float max_skewness_X = 0;
	float max_skewness_Y = 0;
	float max_skewness_Z = 0;

	unsigned int num_cells = 0;

	std::vector<int> bad_cell_roots;
	for (cells_list_iter = cells_list.begin(); cells_list_iter != cells_list.end(); cells_list_iter++)
	{
		CellTrace* cell = *cells_list_iter;
		if (cell->skewnessX == cell->skewnessX 
			&& cell->skewnessY == cell->skewnessY 
			&& cell->skewnessZ == cell->skewnessZ
			&& cell->maxX == cell->maxX
			&& cell->minX == cell->minX
			&& cell->maxY == cell->maxY
			&& cell->minY == cell->minY
			&& cell->maxZ == cell->maxZ
			&& cell->minZ == cell->minZ) //necessary to check for indeterminate floating point numbers
		{
			//calculate a bunch of statistical values
			sum_of_skewness_X += abs(cell->skewnessX);
			sum_of_skewness_Y += abs(cell->skewnessY);
			sum_of_skewness_Z += abs(cell->skewnessZ);
			sum_of_height += (cell->maxY - cell->minY);
			sum_of_width += (cell->maxX - cell->minX);
			sum_of_depth += (cell->maxZ - cell->minZ);
			if (abs(cell->skewnessX) > max_skewness_X)
				max_skewness_X = abs(cell->skewnessX);
			if (abs(cell->skewnessY) > max_skewness_Y)
				max_skewness_Y = abs(cell->skewnessY);
			if (abs(cell->skewnessZ) > max_skewness_Z)
				max_skewness_Z = abs(cell->skewnessZ);
			
			num_cells++;
		}
		else
		{
			std::cout << "Bad cell (indeterminate number), ID: " << cell->rootID() << std::endl;
			bad_cell_roots.push_back(cell->rootID());
		}
	}

	//Select the bad cells and delete them
	CellModel->SelectByIDs(bad_cell_roots);
	DeleteTraces();
	CellModel->SelectByRootTrace(roots);
	cells_list = CellModel->GetSelectedCells();

	double average_skewness_X = sum_of_skewness_X / num_cells;
	double average_skewness_Y = sum_of_skewness_Y / num_cells;
	double average_skewness_Z = sum_of_skewness_Z / num_cells;

	double average_width = sum_of_width / num_cells;
	double average_height = sum_of_height / num_cells;
	double average_depth = sum_of_depth / num_cells;

	std::cout << "Number of cells: " << num_cells << std::endl;

	std::cout << "Average Skewness X: " << average_skewness_X << std::endl;
	std::cout << "Average Skewness Y: " << average_skewness_Y << std::endl;
	std::cout << "Average Skewness Z: " << average_skewness_Z << std::endl;

	std::cout << "Average Cell Width: " << average_width << std::endl;
	std::cout << "Average Cell Height: " << average_height << std::endl;
	std::cout << "Average Cell Depth: " << average_depth << std::endl;

	std::cout << "Max Skewness X: " << max_skewness_X << std::endl;
	std::cout << "Max Skewness Y: " << max_skewness_Y << std::endl;
	std::cout << "Max Skewness Z: " << max_skewness_Z << std::endl;
	std::cout << std::endl;

	//Go through cells list again and only calculate average dimensions on cells with within 80% of the max skewness in each dimension (x, y, z)
	
	double sum_of_pruned_skewness_X = 0;
	double sum_of_pruned_skewness_Y = 0;
	double sum_of_pruned_skewness_Z = 0;
	double sum_of_pruned_width = 0;
	double sum_of_pruned_height = 0;
	double sum_of_pruned_depth = 0;
	size_t num_cells_after_pruning = 0;

	std::vector<int> pruned_cells_roots;
	for (cells_list_iter = cells_list.begin(); cells_list_iter != cells_list.end(); cells_list_iter++)
	{
		CellTrace* cell = *cells_list_iter;
		if (	abs(cell->skewnessX) <= 0.8 * max_skewness_X
			&&	abs(cell->skewnessY) <= 0.8 * max_skewness_Y
			&&	abs(cell->skewnessZ) <= 0.8 * max_skewness_Z)
		{

			sum_of_pruned_skewness_X += abs(cell->skewnessX);
			sum_of_pruned_skewness_Y += abs(cell->skewnessY);
			sum_of_pruned_skewness_Z += abs(cell->skewnessX);
			sum_of_pruned_width += (cell->maxX - cell->minX);
			sum_of_pruned_height += (cell->maxY - cell->minY);
			sum_of_pruned_depth += (cell->maxZ - cell->minZ);
			num_cells_after_pruning++;
		}
	}

	double average_pruned_skewness_X = sum_of_pruned_skewness_X / num_cells_after_pruning;
	double average_pruned_skewness_Y = sum_of_pruned_skewness_Y / num_cells_after_pruning;
	double average_pruned_skewness_Z = sum_of_pruned_skewness_Z / num_cells_after_pruning;

	double average_pruned_height = sum_of_pruned_height / num_cells_after_pruning;
	double average_pruned_width = sum_of_pruned_width / num_cells_after_pruning;
	double average_pruned_depth = sum_of_pruned_depth / num_cells_after_pruning;

	std::cout << "Average Pruned Skewness X: " << average_pruned_skewness_X << std::endl;
	std::cout << "Average Pruned Skewness Y: " << average_pruned_skewness_Y << std::endl;
	std::cout << "Average Pruned Skewness Z: " << average_pruned_skewness_Z << std::endl;
	
	std::cout << "Average Pruned Width: " << average_pruned_width << std::endl;
	std::cout << "Average Pruned Height: " << average_pruned_height << std::endl;
	std::cout << "Average Pruned Depth: " << average_pruned_depth << std::endl;

	std::cout << std::endl;

	//Remove the cells which are within 1/2 of the average pruned width/height/depth from the edge
	double bounds[6];
	ImageActors->getImageBounds(bounds);
	double image_min_x_bounds = bounds[0];
	double image_max_x_bounds = bounds[1];
	double image_min_y_bounds = bounds[2];
	double image_max_y_bounds = bounds[3];
	double image_min_z_bounds = bounds[4];
	double image_max_z_bounds = bounds[5];

	//std::cout << "New image bounds: XMin = " << image_min_x_bounds + average_pruned_width/2 << " XMax = " << image_max_x_bounds - average_pruned_width/2 << std::endl;
	std::vector<int> cropped_cells_root;
	for (cells_list_iter = cells_list.begin(); cells_list_iter != cells_list.end(); cells_list_iter++)
	{
		CellTrace* cell = *cells_list_iter;
		if (	cell->somaX < image_min_x_bounds + average_pruned_width/2  
			|| 	cell->somaX > image_max_x_bounds - average_pruned_width/2  
			|| 	cell->somaY < image_min_y_bounds + average_pruned_height/2
			|| 	cell->somaY > image_max_y_bounds - average_pruned_height/2
			|| 	cell->somaZ < image_min_z_bounds + average_pruned_depth/2
			|| 	cell->somaZ > image_max_z_bounds - average_pruned_depth/2
			)
		{
			//std::cout << "Cropping cell ID = " << cell->rootID() << "\tsomaX = " << cell->somaX << "\tsomaY = " << cell->somaY << std::endl;
			cropped_cells_root.push_back(cell->rootID());
		}
	}
	
	CellModel->SelectByIDs(cropped_cells_root);
	DeleteTraces();
	CellModel->SelectByRootTrace(roots);
	cells_list = CellModel->GetSelectedCells();
}                                                                                                                                                                                                                                      



void View3D::SaveComputedCellFeaturesTable()
{
	if (CellModel->getCellCount() == 0)	//Need to calculate cell features before we can write them!
	{
		std::vector<CellTrace*> NewCells = this->tobj->CalculateCellFeatures();
		if (NewCells.size() > 0)
		{
			this->CellModel->setCells(NewCells);
		}
	}
	
	ofstream myfile;
	myfile.open("L-Measures.txt");
	
	vtkSmartPointer<vtkTable> table = CellModel->getDataTable();
	
	//table->Dump(1);
	
	//Dump out headers
	for(vtkIdType columnIndex = 0; columnIndex < table->GetNumberOfColumns(); columnIndex++ )
	{	
		myfile << table->GetColumnName(columnIndex) << "\t";
	}
	myfile << std::endl;

	//Dump out data
	for(vtkIdType rowIndex = 0; rowIndex < table->GetNumberOfRows(); rowIndex++ )
	{
		for(vtkIdType columnIndex = 0; columnIndex < table->GetNumberOfColumns(); columnIndex++ )
		{
			vtkVariant value = table->GetValue(rowIndex, columnIndex);
			myfile << value << "\t";
		}
		myfile << endl;
	}

	myfile.close();
}