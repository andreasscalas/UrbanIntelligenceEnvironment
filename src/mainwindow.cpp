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
    renderer = vtkSmartPointer<vtkRenderer>::New();
    canvas = vtkSmartPointer<vtkPropAssembly>::New();

    verticesSelectionStyle = vtkSmartPointer<VerticesSelectionStyle>::New();
    linesSelectionStyle = vtkSmartPointer<LineSelectionStyle>::New();
    trianglesSelectionStyle = vtkSmartPointer<TriangleSelectionStyle>::New();
    annotationsSelectionStyle = vtkSmartPointer<AnnotationSelectionInteractorStyle>::New();

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
    connect(trianglesSelectionStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(annotationsSelectionStyle, SIGNAL(updateView()), this, SLOT(slotUpdateView()));
    connect(annotationDialog.get(), SIGNAL(finalizationCalled(std::string, uchar*)), this, SLOT(slotFinalization(std::string, uchar*)));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::draw()
{
    renderer = vtkSmartPointer<vtkRenderer>::New();
    canvas = vtkSmartPointer<vtkPropAssembly>::NewInstance(canvas);
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
    obj->Print(std::cout);
    std::cout << std::flush;
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
        update();
        draw();
    }
}


void MainWindow::on_actionSaveAnnotations_triggered()
{
;
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
        vtkSmartPointer<vtkAreaPicker> picker = vtkSmartPointer<vtkAreaPicker>::New();
        picker->SetRenderer(renderer);
        ui->meshViewer->interactor()->SetPicker(picker);
        verticesSelectionStyle->resetSelection();

        ui->meshViewer->interactor()->SetInteractorStyle(verticesSelectionStyle);
        this->ui->actionLinesSelection->setChecked(false);
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
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
        linesSelectionStyle->resetSelection();

        ui->meshViewer->interactor()->SetInteractorStyle(linesSelectionStyle);
        this->ui->actionVerticesSelection->setChecked(false);
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
    }
}

void MainWindow::on_actionTrianglesRectangleSelection_toggled(bool checked)
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
        if(!selectTriangles)
            trianglesSelectionStyle->resetSelection();
        ui->meshViewer->interactor()->SetInteractorStyle(trianglesSelectionStyle);

        this->ui->actionVerticesSelection->setChecked(false);
        this->ui->actionLinesSelection->setChecked(false);
        this->ui->actionTrianglesLassoSelection->setChecked(false);
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
        trianglesSelectionStyle->setMesh(currentMesh);
        trianglesSelectionStyle->setAssembly(canvas);
        trianglesSelectionStyle->SetTriangles(inputTriangles);
        trianglesSelectionStyle->setQvtkWidget(ui->meshViewer);
        trianglesSelectionStyle->setRen(renderer);
        vtkSmartPointer<vtkAreaPicker> picker = vtkSmartPointer<vtkAreaPicker>::New();
        picker->SetRenderer(renderer);
        ui->meshViewer->interactor()->SetPicker(picker);
        if(!selectTriangles)
            trianglesSelectionStyle->resetSelection();
        ui->meshViewer->interactor()->SetInteractorStyle(trianglesSelectionStyle);
        this->ui->actionVerticesSelection->setChecked(false);
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
    }
}



void MainWindow::on_actionselectAnnotations_triggered(bool checked)
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
        this->ui->actionTrianglesRectangleSelection->setChecked(false);
    }
}


void MainWindow::on_actionSelectionAnnotation_triggered()
{
    annotationDialog->show();
}


void MainWindow::slotUpdateView()
{
    this->ui->meshViewer->renderWindow()->RemoveRenderer(renderer);
    renderer->AddActor(canvas);
    renderer->SetBackground(255,255,255);
    this->ui->meshViewer->renderWindow()->AddRenderer(renderer);
    this->ui->meshViewer->renderWindow()->Render();
    this->ui->meshViewer->update();
}

void MainWindow::slotFinalization(std::string tag, uchar * color)
{
    std::string id;
    if(isAnnotationBeingModified){
        id = annotationBeingModified->getId();
        isAnnotationBeingModified = false;
        annotationBeingModified = nullptr;
    }else
        id = reachedId++;

    if(selectVertices)
        verticesSelectionStyle->finalizeAnnotation(id, tag, color);
    else if(selectEdges)
        linesSelectionStyle->finalizeAnnotation(id, tag, color);
    else
        trianglesSelectionStyle->finalizeAnnotation(id, tag, color);


}
