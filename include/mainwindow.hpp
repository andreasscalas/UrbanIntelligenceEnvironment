#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVTKOpenGLNativeWidget.h>
#include <annotation.hpp>
#include <annotation.hpp>
#include <annotationdialog.hpp>
#include <annotationrelationshipdialog.hpp>
#include <semanticattributedialog.hpp>
#include <annotationselectioninteractorstyle.hpp>
#include <drawabletrianglemesh.hpp>
#include <lineselectionstyle.hpp>
#include <measurestyle.hpp>
#include <relationship.hpp>
#include <triangleselectionstyle.hpp>
#include <verticesselectionstyle.hpp>
#include <vtkPropAssembly.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void draw();

    const std::shared_ptr<Drawables::DrawableTriangleMesh> &getCurrentMesh() const;
    void setCurrentMesh(const std::shared_ptr<Drawables::DrawableTriangleMesh> &newCurrentMesh);

private slots:
    void on_clearCanvasButton_clicked();

    void openContextualMenu(vtkObject *obj, unsigned long, void *, void *);

    void on_actionOpenMesh_triggered();

    void on_actionOpenAnnotations_triggered();

    void on_actionSaveAnnotations_triggered();

    void on_actionVisibleSelection_triggered(bool checked);

    void on_actionRemoveSelected_triggered(bool checked);

    void on_actionVerticesSelection_triggered(bool checked);

    void on_actionTrianglesRectangleSelection_triggered(bool checked);

    void on_actionTrianglesLassoSelection_triggered(bool checked);

    void slotFinalization(std::string, uchar*);

    void on_actionLinesSelection_triggered(bool checked);

    void on_actionSelectAnnotations_triggered(bool checked);

    void on_actionEditAnnotations_triggered(bool checked);

    void on_actionRulerMeasure_triggered(bool checked);

    void on_actionMeasureTape_triggered(bool checked);

    void on_actionCaliperMeasure_triggered(bool checked);

    void on_actionAnnotateSelection_triggered();

    void on_actionAddMeasure_triggered();

    void on_actionAnnotationRelation_triggered();

    void slotUpdate();

    void slotUpdateView();

    void slotAddAnnotationsRelationship(std::string, double, double, double, unsigned int, unsigned int, bool);

    void slotAddSemanticAttribute(std::string, std::string);

    void on_actionSave_relationships_triggered();

    void on_actionOpen_relationships_triggered();

    void on_actionComputeAccessibility_triggered();

    void on_actionclearSelection_triggered();

    void on_actionAddSemanticAttribute_triggered();

    void on_pushButton_clicked();

    void slotSelectAnnotation(std::string id, bool selected);

private:
    Ui::MainWindow *ui;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkPropAssembly> canvas;
    vtkSmartPointer<VerticesSelectionStyle> verticesSelectionStyle;
    vtkSmartPointer<LineSelectionStyle> linesSelectionStyle;
    vtkSmartPointer<TriangleSelectionStyle> trianglesSelectionStyle;
    vtkSmartPointer<AnnotationSelectionInteractorStyle> annotationsSelectionStyle;
    vtkSmartPointer<MeasureStyle> measureStyle;

    std::shared_ptr<AnnotationDialog> annotationDialog;
    std::shared_ptr<AnnotationsRelationshipDialog> relationshipDialog;
    std::shared_ptr<SemanticAttributeDialog> semanticAttributeDialog;
    std::shared_ptr<Drawables::DrawableTriangleMesh> currentMesh;
    std::shared_ptr<SemantisedTriangleMesh::Annotation> annotationBeingModified;
    std::vector<std::shared_ptr<SemantisedTriangleMesh::Relationship> > annotationsRelationships;
    std::string currentPath;
    uint lod;
    unsigned int reachedId;

    bool selectOnlyVisible;
    bool eraseSelected;
    bool selectVertices;
    bool selectEdges;
    bool selectAnnotations;
    bool selectTrianglesWithLasso;
    bool isAnnotationBeingModified;

    void drawMesh();
};
#endif // MAINWINDOW_H
