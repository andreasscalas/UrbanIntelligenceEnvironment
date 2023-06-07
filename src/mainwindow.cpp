#include "mainwindow.hpp"

#include "./ui_mainwindow.h"
#include <vtkDataSetMapper.h>
#include <vtkPoints.h>
#include <vtkPropAssembly.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkNamedColors.h>
#include <vtkGenericOpenGLRenderWindow.h>


#include <drawablesurfaceannotation.hpp>
#include <drawablepointannotation.hpp>
#include <annotationfilemanager.hpp>
#include <QFileDialog>
#include <vtkAreaPicker.h>
#include <vtkIdFilter.h>
#include <annotationdialog.hpp>
#include <qmessagebox.h>
#include <QInputDialog>
#include <drawableboundingmeasure.hpp>

#include "annotationselectioninteractorstyle.hpp"
#include "lineselectionstyle.hpp"
#include "measurestyle.hpp"
#include "triangleselectionstyle.hpp"
#include "verticesselectionstyle.hpp"

vtkStandardNewMacro(MeasureStyle)
vtkStandardNewMacro(AnnotationSelectionInteractorStyle)
vtkStandardNewMacro(VerticesSelectionStyle)
vtkStandardNewMacro(TriangleSelectionStyle)
vtkStandardNewMacro(LineSelectionStyle)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    annotationDialog = std::make_shared<AnnotationDialog>(this);
    relationshipDialog = std::make_shared<AnnotationsRelationshipDialog>(this);
    renderer = vtkSmartPointer<vtkRenderer>::New();
    canvas = vtkSmartPointer<vtkPropAssembly>::New();

    verticesSelectionStyle = vtkSmartPointer<VerticesSelectionStyle>::New();
    linesSelectionStyle = vtkSmartPointer<LineSelectionStyle>::New();
    trianglesSelectionStyle = vtkSmartPointer<TriangleSelectionStyle>::New();
    annotationsSelectionStyle = vtkSmartPointer<AnnotationSelectionInteractorStyle>::New();
    measureStyle = vtkSmartPointer<MeasureStyle>::New();

    currentPath = "";
    selectOnlyVisible = false;
    eraseSelected = false;
    selectVertices = false;
    selectEdges = false;
    selectTrianglesWithLasso = false;
    selectAnnotations = false;
    this->setWindowTitle("CityViewer");
    reachedId = 0;

    connect(verticesSelectionStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(linesSelectionStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(trianglesSelectionStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(annotationsSelectionStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(measureStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(annotationDialog.get(), SIGNAL(finalizationCalled(std::string, uchar*)), this, SLOT(slotFinalization(std::string, uchar*)));
    connect(relationshipDialog.get(), SIGNAL(addSemanticRelationship(std::string, double, double, double, unsigned int, unsigned int, bool)), this, SLOT(slotAddAnnotationsRelationship(std::string, double, double, double, unsigned int, unsigned int, bool)));
    connect(ui->measuresListWidget, SIGNAL(updateSignal()), this, SLOT(slotUpdate()));
    connect(ui->measuresListWidget, SIGNAL(updateViewSignal()), this, SLOT(slotUpdateView()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::draw()
{
    renderer = vtkSmartPointer<vtkRenderer>::New();
    canvas = vtkSmartPointer<vtkPropAssembly>::NewInstance(canvas);
    linesSelectionStyle->setRen(renderer);
    trianglesSelectionStyle->setRen(renderer);
    drawMesh();
    slotUpdateView();
}

void MainWindow::drawMesh()
{

    if(currentMesh != nullptr)
    {
        currentMesh->setDrawAnnotations(true);
        currentMesh->setDrawWireframe(false);
        currentMesh->draw(canvas);
    } else
    {

        vtkNew<vtkNamedColors> colors;
        vtkNew<vtkPoints> points;
        points->InsertNextPoint(1, 0, 0);
        points->InsertNextPoint(0, 1, 0);
        points->InsertNextPoint(0, 0, 1);
        points->InsertNextPoint(-1, -1, 0);
        vtkNew<vtkUnstructuredGrid> unstructuredGrid1;
        unstructuredGrid1->SetPoints(points);

        vtkIdType ptIds[] = {0, 1, 2, 3};
        unstructuredGrid1->InsertNextCell(VTK_TETRA, 4, ptIds);
        vtkNew<vtkDataSetMapper> mapper1;
        mapper1->SetInputData(unstructuredGrid1);

        vtkNew<vtkActor> actor1;
        actor1->SetMapper(mapper1);
        actor1->GetProperty()->SetColor(colors->GetColor3d("Cyan").GetData());
        canvas->AddPart(actor1);
    }


}

void MainWindow::on_clearCanvasButton_clicked()
{
    currentMesh.reset();
    update();
    draw();
}

const std::shared_ptr<DrawableTriangleMesh> &MainWindow::getCurrentMesh() const
{
    return currentMesh;
}

void MainWindow::setCurrentMesh(const std::shared_ptr<DrawableTriangleMesh> &newCurrentMesh)
{
    currentMesh = newCurrentMesh;
}

void MainWindow::openContextualMenu(vtkObject *obj, unsigned long, void *, void *)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("Remove Data Point", this);
    contextMenu.addAction(&action1);

    contextMenu.exec();
}

void MainWindow::on_actionOpenMesh_triggered()
{
    QString filename = QFileDialog::getOpenFileName(nullptr,
                       "Choose an annotation file",
                       QString::fromStdString(currentPath),
                       "PLY(*.ply);;All(*.*)");

    if (!filename.isEmpty()){
        currentMesh.reset();
        currentMesh = std::make_shared<DrawableTriangleMesh>();
        currentMesh->load(filename.toStdString());

        vtkSmartPointer<vtkIdFilter> verticesIdFilter = vtkSmartPointer<vtkIdFilter>::New();
        verticesIdFilter->SetInputData(currentMesh->getPointsActor()->GetMapper()->GetInputAsDataSet());
        verticesIdFilter->PointIdsOn();
        verticesIdFilter->SetPointIdsArrayName("OriginalMeshIds");
        verticesIdFilter->Update();

        vtkSmartPointer<vtkPolyData> inputPoints = static_cast<vtkPolyData*>(verticesIdFilter->GetOutput());
        verticesSelectionStyle->setVisiblePointsOnly(selectOnlyVisible);
        verticesSelectionStyle->setSelectionMode(!eraseSelected);
        verticesSelectionStyle->setMesh(currentMesh);
        verticesSelectionStyle->setAssembly(canvas);
        verticesSelectionStyle->setPoints(inputPoints);
        verticesSelectionStyle->setQvtkwidget(ui->meshViewer);
        verticesSelectionStyle->setRenderer(renderer);

        vtkSmartPointer<vtkIdFilter> linesIdFilter = vtkSmartPointer<vtkIdFilter>::New();
        linesIdFilter->SetInputData(currentMesh->getSurfaceActor()->GetMapper()->GetInputAsDataSet());
        linesIdFilter->PointIdsOn();
        linesIdFilter->SetPointIdsArrayName("OriginalMeshIds");
        linesIdFilter->Update();

        vtkSmartPointer<vtkPolyData> inputEdges = static_cast<vtkPolyData*>(linesIdFilter->GetOutput());
        linesSelectionStyle->setVisiblePointsOnly(selectOnlyVisible);
        linesSelectionStyle->setSelectionMode(!eraseSelected);
        linesSelectionStyle->setMesh(currentMesh);
        linesSelectionStyle->setAssembly(canvas);
        linesSelectionStyle->setPoints(inputEdges);
        linesSelectionStyle->setQvtkwidget(ui->meshViewer);
        linesSelectionStyle->setRen(renderer);

        vtkSmartPointer<vtkIdFilter> trianglesIdFilter = vtkSmartPointer<vtkIdFilter>::New();
        verticesIdFilter->SetInputData(currentMesh->getSurfaceActor()->GetMapper()->GetInputAsDataSet());
        verticesIdFilter->CellIdsOn();
        verticesIdFilter->SetPointIdsArrayName("OriginalMeshIds");
        verticesIdFilter->Update();

        vtkSmartPointer<vtkPolyData> inputTriangles = static_cast<vtkPolyData*>(verticesIdFilter->GetOutput());
        trianglesSelectionStyle->setMesh(currentMesh);
        trianglesSelectionStyle->setAssembly(canvas);
        trianglesSelectionStyle->SetTriangles(inputTriangles);
        trianglesSelectionStyle->setQvtkWidget(ui->meshViewer);
        trianglesSelectionStyle->setRen(renderer);

        annotationsSelectionStyle->setMesh(currentMesh);
        annotationsSelectionStyle->setAssembly(canvas);
        annotationsSelectionStyle->setQvtkWidget(ui->meshViewer);
        annotationsSelectionStyle->setRen(renderer);

        measureStyle->setMeasureAssembly(canvas);
        measureStyle->setMesh(currentMesh);
        measureStyle->setMeshRenderer(renderer);
        measureStyle->setQvtkwidget(this->ui->meshViewer);

        linesSelectionStyle->resetSelection();
        trianglesSelectionStyle->resetSelection();
        annotationsSelectionStyle->resetSelection();
        measureStyle->reset();

        update();
        draw();
    }
}


void MainWindow::on_actionOpenAnnotations_triggered()
{
    QString filename = QFileDialog::getOpenFileName(nullptr,
         "Choose an annotation file",
         QString::fromStdString(currentPath),
         "ANT(*.ant);;FCT(*.fct);;TRIANT(*.triant);;All(*.*)");
    if(!filename.isEmpty() && currentMesh != nullptr)
    {
        SemantisedTriangleMesh::AnnotationFileManager manager;
        manager.setMesh(currentMesh);
        auto annotations = manager.readAndStoreAnnotations(filename.toStdString());
        currentMesh->setAnnotations(annotations);
        reachedId = annotations.size();

        this->ui->measuresListWidget->setMesh(currentMesh);
        this->ui->measuresListWidget->update();
        update();
        draw();
    }
}


void MainWindow::on_actionSaveAnnotations_triggered()
{
    QString filename = QFileDialog::getSaveFileName(nullptr,
                     "Save the annotations on the mesh",
                     QString::fromStdString(currentPath),
                     "ANT(*.ant);;FCT(*.fct);;TRIANT(*.triant);;M(*.m)");

    if (!filename.isEmpty()){

      QFileInfo info(filename);
      currentPath = info.absolutePath().toStdString();
      SemantisedTriangleMesh::AnnotationFileManager manager;
      manager.setMesh(currentMesh);
      if(!manager.writeAnnotations(filename.toStdString()))
          std::cout << "Something went wrong during annotation file writing." << std::endl << std::flush;
    }
}

void MainWindow::on_actionVisibleSelection_triggered(bool checked)
{
    this->selectOnlyVisible = checked;
    verticesSelectionStyle->setVisiblePointsOnly(checked);
    linesSelectionStyle->setVisiblePointsOnly(checked);
    trianglesSelectionStyle->setVisibleTrianglesOnly(checked);

}

void MainWindow::on_actionRemoveSelected_triggered(bool checked)
{
    this->eraseSelected = checked;
    verticesSelectionStyle->setSelectionMode(!checked);
    linesSelectionStyle->setSelectionMode(!checked);
    trianglesSelectionStyle->setSelectionMode(!checked);
}

void MainWindow::on_actionVerticesSelection_triggered(bool checked)
{
    this->selectVertices = checked;
    if(checked)
    {
        this->selectEdges = false;
        this->selectAnnotations = false;
        vtkSmartPointer<vtkIdFilter> idFilter = vtkSmartPointer<vtkIdFilter>::New();
        idFilter->SetInputData(currentMesh->getPointsActor()->GetMapper()->GetInputAsDataSet());
        idFilter->PointIdsOn();
        idFilter->SetPointIdsArrayName("OriginalMeshIds");
        idFilter->Update();

        vtkSmartPointer<vtkPolyData> inputPoints = static_cast<vtkPolyData*>(idFilter->GetOutput());
        verticesSelectionStyle->setVisiblePointsOnly(selectOnlyVisible);
        verticesSelectionStyle->setSelectionMode(!eraseSelected);
        verticesSelectionStyle->setMesh(currentMesh);
        verticesSelectionStyle->setAssembly(canvas);
        verticesSelectionStyle->setPoints(inputPoints);
        verticesSelectionStyle->setQvtkwidget(ui->meshViewer);
        verticesSelectionStyle->setRenderer(renderer);
        vtkSmartPointer<vtkAreaPicker> picker = vtkSmartPointer<vtkAreaPicker>::New();
        picker->SetRenderer(renderer);
        ui->meshViewer->interactor()->SetPicker(picker);
        ui->meshViewer->interactor()->SetInteractorStyle(verticesSelectionStyle);
        this->ui->actionLinesSelection->setChecked(false);
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
        this->ui->actionSelectAnnotations->setChecked(false);
        this->ui->actionRulerMeasure->setChecked(false);
        this->ui->actionMeasureTape->setChecked(false);
        this->ui->actionCaliperMeasure->setChecked(false);
        linesSelectionStyle->resetSelection();
        trianglesSelectionStyle->resetSelection();
        annotationsSelectionStyle->resetSelection();
    }
}

void MainWindow::on_actionLinesSelection_triggered(bool checked)
{
    selectEdges = checked;
    if(checked)
    {
        this->selectVertices = false;
        this->selectAnnotations = false;

        vtkSmartPointer<vtkIdFilter> idFilter = vtkSmartPointer<vtkIdFilter>::New();
        idFilter->SetInputData(currentMesh->getSurfaceActor()->GetMapper()->GetInputAsDataSet());
        idFilter->PointIdsOn();
        idFilter->SetPointIdsArrayName("OriginalMeshIds");
        idFilter->Update();

        vtkSmartPointer<vtkPolyData> inputEdges = static_cast<vtkPolyData*>(idFilter->GetOutput());
        linesSelectionStyle->setVisiblePointsOnly(selectOnlyVisible);
        linesSelectionStyle->setSelectionMode(!eraseSelected);
        linesSelectionStyle->setMesh(currentMesh);
        linesSelectionStyle->setAssembly(canvas);
        linesSelectionStyle->setPoints(inputEdges);
        linesSelectionStyle->setQvtkwidget(ui->meshViewer);
        linesSelectionStyle->setRen(renderer);
        vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
        ui->meshViewer->interactor()->SetPicker(picker);
        ui->meshViewer->interactor()->SetInteractorStyle(linesSelectionStyle);
        this->ui->actionVerticesSelection->setChecked(false);
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
        this->ui->actionSelectAnnotations->setChecked(false);
        this->ui->actionRulerMeasure->setChecked(false);
        this->ui->actionMeasureTape->setChecked(false);
        this->ui->actionCaliperMeasure->setChecked(false);
        verticesSelectionStyle->resetSelection();
        trianglesSelectionStyle->resetSelection();
        annotationsSelectionStyle->resetSelection();
    }
}

void MainWindow::on_actionTrianglesRectangleSelection_triggered(bool checked)
{
    selectTrianglesWithLasso = !checked;
    if(checked)
    {
        bool selectTriangles = !selectVertices && !selectEdges && !selectAnnotations;

        this->selectVertices = false;
        this->selectEdges = false;
        this->selectAnnotations = false;

        vtkSmartPointer<vtkIdFilter> idFilter = vtkSmartPointer<vtkIdFilter>::New();
        idFilter->SetInputData(currentMesh->getSurfaceActor()->GetMapper()->GetInputAsDataSet());
        idFilter->CellIdsOn();
        idFilter->SetPointIdsArrayName("OriginalMeshIds");
        idFilter->Update();

        vtkSmartPointer<vtkPolyData> inputTriangles = static_cast<vtkPolyData*>(idFilter->GetOutput());
        trianglesSelectionStyle->setVisibleTrianglesOnly(selectOnlyVisible);
        trianglesSelectionStyle->setSelectionMode(!eraseSelected);
        trianglesSelectionStyle->setSelectionType(TriangleSelectionStyle::SelectionType::RECTANGLE_AREA);
        trianglesSelectionStyle->setMesh(currentMesh);
        trianglesSelectionStyle->setAssembly(canvas);
        trianglesSelectionStyle->SetTriangles(inputTriangles);
        trianglesSelectionStyle->setQvtkWidget(ui->meshViewer);
        trianglesSelectionStyle->setRen(renderer);
        vtkSmartPointer<vtkAreaPicker> picker = vtkSmartPointer<vtkAreaPicker>::New();
        picker->SetRenderer(renderer);
        ui->meshViewer->interactor()->SetPicker(picker);
        ui->meshViewer->interactor()->SetInteractorStyle(trianglesSelectionStyle);

        this->ui->actionVerticesSelection->setChecked(false);
        this->ui->actionLinesSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
        this->ui->actionSelectAnnotations->setChecked(false);
        this->ui->actionRulerMeasure->setChecked(false);
        this->ui->actionMeasureTape->setChecked(false);
        this->ui->actionCaliperMeasure->setChecked(false);
        verticesSelectionStyle->resetSelection();
        linesSelectionStyle->resetSelection();
        annotationsSelectionStyle->resetSelection();

    }
}

void MainWindow::on_actionTrianglesLassoSelection_triggered(bool checked)
{
    selectTrianglesWithLasso = checked;
    if(checked)
    {
        bool selectTriangles = !selectVertices && !selectEdges && !selectAnnotations;
        this->selectVertices = false;
        this->selectEdges = false;
        this->selectAnnotations = false;

        vtkSmartPointer<vtkIdFilter> idFilter = vtkSmartPointer<vtkIdFilter>::New();
        idFilter->SetInputData(currentMesh->getSurfaceActor()->GetMapper()->GetInputAsDataSet());
        idFilter->CellIdsOn();
        idFilter->SetPointIdsArrayName("OriginalMeshIds");
        idFilter->Update();

        vtkSmartPointer<vtkPolyData> inputTriangles = static_cast<vtkPolyData*>(idFilter->GetOutput());
        trianglesSelectionStyle->setVisibleTrianglesOnly(selectOnlyVisible);
        trianglesSelectionStyle->setSelectionMode(!eraseSelected);
        trianglesSelectionStyle->setSelectionType(TriangleSelectionStyle::SelectionType::LASSO_AREA);
        trianglesSelectionStyle->SetTriangles(inputTriangles);
        trianglesSelectionStyle->setMesh(currentMesh);
        trianglesSelectionStyle->setAssembly(canvas);
        trianglesSelectionStyle->setQvtkWidget(ui->meshViewer);
        trianglesSelectionStyle->setRen(renderer);
        vtkSmartPointer<vtkAreaPicker> picker = vtkSmartPointer<vtkAreaPicker>::New();
        picker->SetRenderer(renderer);
        ui->meshViewer->interactor()->SetPicker(picker);
        ui->meshViewer->interactor()->SetInteractorStyle(trianglesSelectionStyle);
        this->ui->actionVerticesSelection->setChecked(false);
        verticesSelectionStyle->resetSelection();
        this->ui->actionLinesSelection->setChecked(false);
        linesSelectionStyle->resetSelection();
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
        this->ui->actionSelectAnnotations->setChecked(false);
        annotationsSelectionStyle->resetSelection();
        this->ui->actionRulerMeasure->setChecked(false);
        this->ui->actionMeasureTape->setChecked(false);
        this->ui->actionCaliperMeasure->setChecked(false);
    }
}



void MainWindow::on_actionSelectAnnotations_triggered(bool checked)
{
    this->selectAnnotations = checked;
    if(checked)
    {
        this->selectVertices = false;
        this->selectEdges = false;
        annotationsSelectionStyle->setMesh(currentMesh);
        annotationsSelectionStyle->setAssembly(canvas);
        annotationsSelectionStyle->setQvtkWidget(ui->meshViewer);
        annotationsSelectionStyle->setRen(renderer);
        annotationsSelectionStyle->resetSelection();
        ui->meshViewer->interactor()->SetInteractorStyle(annotationsSelectionStyle);
        this->ui->actionVerticesSelection->setChecked(false);
        this->ui->actionLinesSelection->setChecked(false);
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
        this->ui->actionRulerMeasure->setChecked(false);
        this->ui->actionMeasureTape->setChecked(false);
        this->ui->actionCaliperMeasure->setChecked(false);
        verticesSelectionStyle->resetSelection();
        linesSelectionStyle->resetSelection();
        trianglesSelectionStyle->resetSelection();
    }
    slotUpdateView();
}


void MainWindow::slotUpdateView()
{
    this->ui->meshViewer->renderWindow()->RemoveRenderer(renderer);
    if(currentMesh != nullptr)
    {
        auto collection = canvas->GetParts();
        collection->InitTraversal();
        uint partsNumber = collection->GetNumberOfItems();
        for (vtkIdType i = 0; i < partsNumber; i++)
        {
          canvas->RemovePart(collection->GetNextProp());
        }
        currentMesh->draw(canvas);
    }
    renderer->AddActor(canvas);
    canvas->Modified();
    renderer->SetBackground(255,255,255);
    ui->measuresListWidget->update();
    this->ui->meshViewer->renderWindow()->AddRenderer(renderer);
    this->ui->meshViewer->renderWindow()->Render();
    this->ui->meshViewer->update();
}

void MainWindow::slotAddAnnotationsRelationship(std::string type, double weight, double minValue, double maxValue, unsigned int measureId1, unsigned int measureId2, bool directed)
{
    auto relationship = std::make_shared<SemantisedTriangleMesh::Relationship>();

    relationship->setId(annotationsRelationships.size());
    relationship->setAnnotations(relationshipDialog->getSubjects());
    relationship->setType(type);
    relationship->setWeight(weight);
    relationship->setMinValue(minValue);
    relationship->setMaxValue(maxValue);

    for (unsigned int i = 0; i < relationship->getAnnotations().size(); i++) {
        for (unsigned int j = i; j < relationship->getAnnotations().size(); j++) {
            if( i == j && relationship->getAnnotations().size() != 1)
                continue;
            currentMesh->addAnnotationsRelationship(relationship->getAnnotations()[i], relationship->getAnnotations()[j], type, directed);
        }
    }


    slotUpdateView();
}

void MainWindow::slotFinalization(std::string tag, uchar * color)
{
    std::string id;
    if(isAnnotationBeingModified){
        id = annotationBeingModified->getId();
        isAnnotationBeingModified = false;
        annotationBeingModified = nullptr;
    }else
        id = std::to_string(reachedId++);

    if(selectVertices)
        verticesSelectionStyle->finalizeAnnotation(id, tag, color);
    else if(selectEdges)
        linesSelectionStyle->finalizeAnnotation(id, tag, color);
    else
        trianglesSelectionStyle->finalizeAnnotation(id, tag, color);


    this->ui->measuresListWidget->setMesh(currentMesh);
    this->ui->measuresListWidget->update();
}

void MainWindow::on_actionEditAnnotations_triggered(bool checked)
{
    auto selectedAnnotations = annotationsSelectionStyle->getSelectedAnnotations();
    if(selectedAnnotations.size() == 1){
        this->isAnnotationBeingModified = true;
        annotationBeingModified = selectedAnnotations[0];
        annotationsSelectionStyle->resetSelection();
        canvas->RemovePart(std::dynamic_pointer_cast<DrawableAnnotation>(annotationBeingModified)->getCanvas());
        currentMesh->removeAnnotation(std::stoi(annotationBeingModified->getId()));

        if(annotationBeingModified->getType() == SemantisedTriangleMesh::AnnotationType::Point){
            auto selectedPoints = std::dynamic_pointer_cast<DrawablePointAnnotation>(annotationBeingModified)->getPoints();
            verticesSelectionStyle->resetSelection();
            verticesSelectionStyle->setAssembly(canvas);
            verticesSelectionStyle->defineSelection(selectedPoints);
            this->ui->actionVerticesSelection->setChecked(true);
            this->ui->actionVerticesSelection->triggered(true);
            verticesSelectionStyle->draw();
        }
        else if(annotationBeingModified->getType() == SemantisedTriangleMesh::AnnotationType::Line){
            auto selectedLines = std::dynamic_pointer_cast<DrawableLineAnnotation>(annotationBeingModified)->getPolyLines();
            linesSelectionStyle->resetSelection();
            linesSelectionStyle->setAssembly(canvas);
            linesSelectionStyle->defineSelection(selectedLines);
            linesSelectionStyle->setLassoStarted(false);
            this->ui->actionLinesSelection->setChecked(true);
            this->ui->actionLinesSelection->triggered(true);
            linesSelectionStyle->draw();
        }
        else if(annotationBeingModified->getType() == SemantisedTriangleMesh::AnnotationType::Surface){
            auto selectedTriangles = std::dynamic_pointer_cast<DrawableSurfaceAnnotation>(annotationBeingModified)->getTrianglesIds();
            trianglesSelectionStyle->resetSelection();
            trianglesSelectionStyle->setAssembly(canvas);
            trianglesSelectionStyle->defineSelection(selectedTriangles);
            if(selectTrianglesWithLasso)
            {
                this->ui->actionTrianglesLassoSelection->setChecked(true);
                this->ui->actionTrianglesLassoSelection->triggered(true);
            }
            else
            {
                this->ui->actionTrianglesRectangleSelection->setChecked(true);
                this->ui->actionTrianglesRectangleSelection->triggered(true);
            }
        }

    } else {
        auto dialog = new QMessageBox(this);
        dialog->setWindowTitle("Error");
        dialog->setText("This action can be applied only if exactly one annotation is selected");
        dialog->show();
    }
}

void MainWindow::on_actionRulerMeasure_triggered(bool checked)
{
    auto selectedAnnotations = annotationsSelectionStyle->getSelectedAnnotations();
    if(selectedAnnotations.size() == 1){
        std::dynamic_pointer_cast<DrawableAnnotation>(selectedAnnotations[0])->setDrawAttributes(true);
        measureStyle->setDrawAttributes(true);
        measureStyle->setMeasureAssembly(canvas);
        measureStyle->setMesh(currentMesh);
        measureStyle->setMeshRenderer(renderer);
        measureStyle->setQvtkwidget(this->ui->meshViewer);
        ui->actionVerticesSelection->setChecked(false);
        ui->actionLinesSelection->setChecked(false);
        ui->actionTrianglesRectangleSelection->setChecked(false);
        ui->actionTrianglesLassoSelection->setChecked(false);
        ui->actionSelectAnnotations->setChecked(false);
        ui->actionMeasureTape->setChecked(false);
        ui->actionCaliperMeasure->setChecked(false);
        ui->meshViewer->interactor()->SetInteractorStyle(measureStyle);
        measureStyle->setMeasureType(MeasureStyle::MeasureType::RULER);
    }
}


void MainWindow::on_actionMeasureTape_triggered(bool checked)
{
    auto selectedAnnotations = annotationsSelectionStyle->getSelectedAnnotations();
    if(selectedAnnotations.size() == 1){
        measureStyle->setDrawAttributes(true);
        measureStyle->setMeasureAssembly(canvas);
        measureStyle->setMesh(currentMesh);
        measureStyle->setMeshRenderer(renderer);
        measureStyle->setQvtkwidget(this->ui->meshViewer);
        ui->actionVerticesSelection->setChecked(false);
        ui->actionLinesSelection->setChecked(false);
        ui->actionTrianglesRectangleSelection->setChecked(false);
        ui->actionTrianglesLassoSelection->setChecked(false);
        ui->actionSelectAnnotations->setChecked(false);
        ui->actionRulerMeasure->setChecked(false);
        ui->actionCaliperMeasure->setChecked(false);
        ui->meshViewer->interactor()->SetInteractorStyle(measureStyle);
        measureStyle->setMeasureType(MeasureStyle::MeasureType::TAPE);
    }

}


void MainWindow::on_actionCaliperMeasure_triggered(bool checked)
{
    auto selectedAnnotations = annotationsSelectionStyle->getSelectedAnnotations();
    if(selectedAnnotations.size() == 1){
        measureStyle->setDrawAttributes(true);
        measureStyle->setMeasureAssembly(canvas);
        measureStyle->setMesh(currentMesh);
        measureStyle->setMeshRenderer(renderer);
        measureStyle->setQvtkwidget(this->ui->meshViewer);
        ui->actionVerticesSelection->setChecked(false);
        ui->actionLinesSelection->setChecked(false);
        ui->actionTrianglesRectangleSelection->setChecked(false);
        ui->actionTrianglesLassoSelection->setChecked(false);
        ui->actionSelectAnnotations->setChecked(false);
        ui->actionRulerMeasure->setChecked(false);
        ui->actionMeasureTape->setChecked(false);
        ui->meshViewer->interactor()->SetInteractorStyle(measureStyle);
        measureStyle->setMeasureType(MeasureStyle::MeasureType::BOUNDING);
    }

}


void MainWindow::on_actionAnnotateSelection_triggered()
{
    annotationDialog->show();
}

void MainWindow::on_actionAddMeasure_triggered()
{
    auto selected = annotationsSelectionStyle->getSelectedAnnotations();

    if(selected.size() == 1){
        bool ok;
        QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                                 tr("Attribute name:"), QLineEdit::Normal,
                                                 QDir::home().dirName(), &ok);
        if (ok && !text.isEmpty()){
            auto attribute = measureStyle->finalizeAttribute(selected[0]->getAttributes().size(), text.toStdString());
            selected[0]->addAttribute(attribute);
        }

        this->ui->measuresListWidget->setMesh(currentMesh);
        this->ui->measuresListWidget->update();
        slotUpdateView();
    } else {
        QMessageBox* dialog = new QMessageBox(this);
        dialog->setWindowTitle("Error");
        dialog->setText("You need to select at least one annotation");
        dialog->show();
    }
}

void MainWindow::slotUpdate()
{
    currentMesh->update();
    slotUpdateView();
    update();
}


void MainWindow::on_actionAnnotationRelation_triggered()
{
    auto selected = annotationsSelectionStyle->getSelectedAnnotations();
    relationshipDialog->setSubjects(selected);
    relationshipDialog->show();

}


void MainWindow::on_actionSave_relationships_triggered()
{
    QString filename = QFileDialog::getSaveFileName(nullptr,
                     "Save the annotations' relationships",
                     QString::fromStdString(currentPath),
                     "REL(*.rel);;");

    if (!filename.isEmpty()){

      QFileInfo info(filename);
      currentPath = info.absolutePath().toStdString();
      SemantisedTriangleMesh::AnnotationFileManager manager;
      manager.setMesh(currentMesh);
      if(!manager.writeRelationships(filename.toStdString()))
          std::cout << "Something went wrong during relationships file writing." << std::endl << std::flush;
    }
}


void MainWindow::on_actionOpen_relationships_triggered()
{
    QString filename = QFileDialog::getOpenFileName(nullptr,
                     "Load the annotations' relationships",
                     QString::fromStdString(currentPath),
                     "REL(*.rel);;");

    if (!filename.isEmpty()){

      QFileInfo info(filename);
      currentPath = info.absolutePath().toStdString();
      SemantisedTriangleMesh::AnnotationFileManager manager;
      manager.setMesh(currentMesh);
      if(!manager.readRelationships(filename.toStdString()))
          std::cout << "Something went wrong during relationships file load." << std::endl << std::flush;
    }
}

