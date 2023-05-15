#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVTKOpenGLNativeWidget.h>
#include <annotation.hpp>
#include <annotation.hpp>
#include <annotationdialog.hpp>
#include <annotationselectioninteractorstyle.hpp>
#include <drawabletrianglemesh.hpp>
#include <lineselectionstyle.hpp>
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

    const std::shared_ptr<DrawableTriangleMesh> &getCurrentMesh() const;
    void setCurrentMesh(const std::shared_ptr<DrawableTriangleMesh> &newCurrentMesh);

private slots:
    void on_clearCanvasButton_clicked();

    void openContextualMenu(vtkObject *obj, unsigned long, void *, void *);

    void on_actionOpenMesh_triggered();

    void on_actionOpenAnnotations_triggered();

    void on_actionSaveAnnotations_triggered();

    void on_actionVisibleSelection_triggered(bool checked);

    void on_actionRemoveSelected_triggered(bool checked);

    void on_actionVerticesSelection_triggered(bool checked);

    void on_actionTrianglesRectangleSelection_toggled(bool checked);

    void on_actionTrianglesLassoSelection_triggered(bool checked);

    void on_actionSelectionAnnotation_triggered();

    void slotUpdateView();

    void slotFinalization(std::string, uchar*);

    void on_actionLinesSelection_triggered(bool checked);

    void on_actionselectAnnotations_triggered(bool checked);

private:
    Ui::MainWindow *ui;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkPropAssembly> canvas;
    vtkSmartPointer<VerticesSelectionStyle> verticesSelectionStyle;
    vtkSmartPointer<LineSelectionStyle> linesSelectionStyle;
    vtkSmartPointer<TriangleSelectionStyle> trianglesSelectionStyle;
    vtkSmartPointer<AnnotationSelectionInteractorStyle> annotationsSelectionStyle;

    std::shared_ptr<AnnotationDialog> annotationDialog;
    std::shared_ptr<DrawableTriangleMesh> currentMesh;
    std::shared_ptr<SemantisedTriangleMesh::Annotation> annotationBeingModified;
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